#include "header.h"
#include "utils.h"

// FileSystem类的构造函数
FileSystem::FileSystem()
{
	// 暂时不需要内容
}

// FileSystem类析构函数
FileSystem::~FileSystem()
{
	/*
	this->exit();
	delete this->bufManager;
	delete this->spb;
	delete this->userTable;
	if (this->curDirInode != this->rootDirInode)
		delete this->rootDirInode;
	delete this->curDirInode;*/
}

/**************************************************************
* GetAbsolutionPath 获取绝对路径，假设路径正确
* 参数：path 相对路径或绝对路径
* 返回值：string 绝对路径
***************************************************************/
string FileSystem::GetAbsolutionPath(string path)
{
	if (path[0] == '/')
		return path;
	else
		return this->curDir + path;
}

/**************************************************************
* GetBufferManager 得到文件系统的缓存管理类
* 参数：
* 返回值：BufferManager* 指针
***************************************************************/
BufferManager* FileSystem::GetBufferManager()
{
    return this->bufManager;
}

/**************************************************************
* WriteSpb将SuperBlock写回磁盘
* 参数：
* 返回值：
***************************************************************/
void FileSystem::WriteSpb()
{
	char* p = SuperBlk_to_Char(this->spb);
	Buf* bp = this->bufManager->Bread(POSITION_SUPERBLOCK);
	memcpy(bp->b_addr, p, SIZE_BUFFER);
	this->bufManager->Bwrite(bp);
	bp = this->bufManager->Bread(POSITION_SUPERBLOCK + 1);
	// 这里之前有内存泄漏,没有填充的话这里一直会有问题
	memcpy(bp->b_addr, p + SIZE_BLOCK, SIZE_BLOCK);
	this->bufManager->Bwrite(bp);
}

/**************************************************************
* WriteUserTable 将用户表写回磁盘
* 参数：
* 返回值：
***************************************************************/
void FileSystem::WriteUserTable()
{
	int filoc = fopen("/etc/userTable.txt");
	File* userTableFile = &this->openFileTable[filoc];
	this->fwrite(UserTable_to_Char(this->userTable), sizeof(UserTable), userTableFile);
	this->fclose(userTableFile);
}

/**************************************************************
* IAlloc 分配一个空闲的外存Inode
* 参数：
* 返回值：Inode* 返回分配到的内存Inode，如果分配失败，返回NULL
***************************************************************/
Inode* FileSystem::IAlloc()
{
    Buf* pBuf;
    Inode* pNode;
    int ino = 0; // 分配到的空闲外存Inode编号

    // SuperBlock直接管理的空闲Inode索引表已空
    // 注入新的空闲Inode索引表
    if (this->spb->s_ninode <= 0)
    {
        // 依次读入磁盘Inode区中的磁盘块，搜索其中空闲外存Inode，记入空闲Inode索引表
        for (unsigned int i = 0; i < this->spb->s_isize; i++)
        {
            pBuf = this->bufManager->Bread(POSITION_DISKINODE + i / NUM_INODE_PER_BLOCK);

            // 获取缓冲区首址
            int* p = (int*)pBuf->b_addr;

            // 检查该缓冲区中每个外存Inode的i_mode != 0，表示已经被占用
            for (int j = 0; j < NUM_INODE_PER_BLOCK; j++)
            {
                ino++;
                int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

                // 该外存Inode已被占用，不能记入空闲Inode索引表
                if (mode != 0)
                {
                    continue;
                }

                /*
                 * 如果外存inode的i_mode==0，此时并不能确定
                 * 该inode是空闲的，因为有可能是内存inode没有写到
                 * 磁盘上,所以要继续搜索内存inode中是否有相应的项
                 */
                if (this->IsLoaded(ino) == -1)
                {
                    // 该外存Inode没有对应的内存拷贝，将其记入空闲Inode索引表
                    this->spb->s_inode[this->spb->s_ninode++] = ino;

                    // 如果空闲索引表已经装满，则不继续搜索
                    if (this->spb->s_ninode >= 100)
                        break;
                }
            }

            // 如果空闲索引表已经装满，则不继续搜索
            if (this->spb->s_ninode >= 100)
                break;
        }
    }

    // 如果这样了还没有可用外存Inode，返回NULL
    if (this->spb->s_ninode <= 0)
    {
        cout << "磁盘上外存Inode区已满!" << endl;
        throw(ENOSPC);
        return NULL;
    }

    // 现在从外存分配内存Inode
    int inumber = this->spb->s_inode[--this->spb->s_ninode];
    pNode = IGet(inumber);
    if (NULL == pNode) // 不做修改操作
        return NULL;

    if (0 == pNode->i_mode)
    {
        pNode->Clean();
        return pNode;
    }
    return pNode;
}

/**************************************************************
* Free 释放指定的数据盘块
* 参数：blkno 要释放的数据盘块号
* 返回值：
***************************************************************/
void FileSystem::Free(int blkno)
{
	Buf* pBuf;
	if (blkno < POSITION_BLOCK)
	{
		cout << "不能释放系统盘块" << endl;
		return;
	}

	// 如果先前系统中已经没有空闲盘块，现在释放的是系统中第1块空闲盘块
	if (this->spb->s_nfree <= 0)
	{
		this->spb->s_nfree = 1;
		this->spb->s_free[0] = 0;
	}

	// 如果SuperBlock中直接管理空闲磁盘块号的栈已满
	if (this->spb->s_nfree >= NUM_FREE_BLOCK_GROUP)
	{
		// 分配一个新缓存块，用于存放新的空闲磁盘块号
		pBuf = this->bufManager->GetBlk(blkno);

		// 将s_nfree和s_free[100]写入回收盘块的前101个字节
		// s_free[0]=回收的盘块号
		// s_nfree=1
		int* stack = new int[NUM_FREE_BLOCK_GROUP + 1] {0};		// 第一位是链接的上一组的盘块个数
		stack[0] = this->spb->s_nfree;                          // 第一位是链接的上一组的盘块个数
		for (int i = 0; i < NUM_FREE_BLOCK_GROUP; i++)
			stack[i + 1] = this->spb->s_free[i];
		memcpy(pBuf->b_addr, IntArray_to_Char(stack, NUM_FREE_BLOCK_GROUP + 1), sizeof(int) * NUM_FREE_BLOCK_GROUP + 1);
		bufManager->Bwrite(pBuf);

		this->spb->s_free[0] = blkno;
		this->spb->s_nfree = 1;
	}

	// 释放数据盘块号
	this->spb->s_free[this->spb->s_nfree++] = blkno;
}

/**************************************************************
* IsLoaded 判断指定外存Inode是否已经加载到内存中
* 参数：inumber 外存Inode编号
* 返回值：如果已经加载，返回内存Inode在inodeTable的编号，否则返回-1
***************************************************************/
int FileSystem::IsLoaded(int inumber)
{
    // 寻找指定外存Inode的内存inode拷贝
    for (int i = 0; i < NUM_INODE; i++)
        if (this->inodeTable[i].i_count != 0 && this->inodeTable[i].i_number == inumber)
            return i;

    return -1;
}

/**************************************************************
* IGet 从外存读取指定外存Inode到内存中
* 参数：inumber 外存Inode编号
* 返回值：Inode* 内存Inode拷贝
***************************************************************/
Inode* FileSystem::IGet(int inumber)
{
    Inode* pInode = NULL;
    // 在inodeTable中查找指定外存Inode的内存inode拷贝
    int isInTable = this->IsLoaded(inumber);

    // 如果找到了，直接返回内存inode拷贝，引用计数加1
    if (isInTable != -1)
    {
        pInode = &this->inodeTable[isInTable];
        pInode->i_count++;
        pInode->i_atime = unsigned int(time(NULL));
        return pInode;
    }

    // 没有找到，先从内存Inode节点表中分配一个Inode,再从外存读取
    for (int i = 0; i < NUM_INODE; i++)
        // 如果该内存Inode引用计数为零，则该Inode表示空闲，可以使用
        if (this->inodeTable[i].i_count == 0)
        {
            pInode = &(this->inodeTable[i]);
            break;
        }

    // 如果内存InodeTable已满，抛出异常
    if (pInode == NULL)
    {
        cout << "内存InodeTable已满" << endl;
        throw(ENFILE);
        return pInode;
    }

    // 如果内存InodeTabl没满，从外存读取指定外存Inode到内存中
    pInode->i_number = inumber;
    pInode->i_count++;
    pInode->i_atime = unsigned int(time(NULL));

    // 将该外存Inode读入缓冲区
    Buf* pBuf = this->bufManager->Bread(POSITION_DISKINODE + (inumber - 1) / NUM_INODE_PER_BLOCK);
    // 将缓冲区中的外存Inode信息拷贝到新分配的内存Inode中
    pInode->ICopy(pBuf, inumber);
    return pInode;
}

/**************************************************************
* IPut 释放InodeTable中的Inode节点
* 参数：pNode 内存Inode指针
* 返回值：
***************************************************************/
void FileSystem::IPut(Inode* pNode)
{
	// 当前进程为引用该内存Inode的唯一进程，且准备释放该内存Inode
	if (pNode->i_count == 1)
	{
		pNode->i_atime = unsigned int(time(NULL));
		pNode->WriteI();

		// 该文件已经没有目录路径指向它
		if (pNode->i_nlink <= 0)
		{
			// 释放该文件占据的数据盘块
			pNode->ITrunc();
			pNode->i_mode = 0;
			// 释放对应的外存Inode
			this->IFree(pNode->i_number);
		}
		pNode->Clean();
	}

	// 减少内存Inode的引用计数
	pNode->i_count--;
}

/**************************************************************
* IFree 释放外存Inode节点
* 参数：number 外存Inode编号
* 返回值：
***************************************************************/
void FileSystem::IFree(int number)
{
	// spb够用
	if (this->spb->s_ninode >= NUM_FREE_INODE)
		return;

	// 将该外存Inode的编号记入空闲Inode索引表，后来的会直接覆盖掉它的内容
	this->spb->s_inode[this->spb->s_ninode++] = number;
}

/**************************************************************
* NameI 根据文件路径查找对应的Inode
* 参数：path 文件路径
* 返回值：Inode* 返回对应的Inode，如果没有找到，返回NULL
***************************************************************/
Inode* FileSystem::NameI(string path)
{
	Inode* pInode;
	Buf* pbuf;
	vector<string> paths = stringSplit(path, '/'); // 所以要求文件夹和文件的名中不能出现"/"
	int ipaths = 0;
	bool isFind = false;

	// 第一个字符为/表示绝对路径
	if (path.size() != 0 && path[0] == '/') // 从根目录开始查找
		pInode = this->rootDirInode;
	else // 相对路径的查找
		pInode = this->curDirInode;
	int blkno = pInode->Bmap(0);
	// 读取磁盘的数据

	while (true)
	{
		isFind = false;
		// 包含path为空的情况
		if (ipaths == paths.size()) // 这种情况说明找到了对应的文件或目录
			break;
		else if (ipaths >= paths.size())
			return NULL;

		// 如果现有的Inode是目录文件才正确,因为在这里面的循环才会找到文件/目录
		// 一旦找到文件/目录不会进入这个循环
		if (pInode->i_mode & Inode::INodeMode::IDIR)
		{
			// 计算要读的物理盘块号
			// 由于目录文件只占一个盘块，所以只有一项不为空
			int blkno = pInode->Bmap(0);
			// 读取磁盘的数据
			pbuf = this->bufManager->Bread(blkno);

			// 将数据转为目录结构
			Directory* fatherDir = Char_to_Directory(pbuf->b_addr);

			// 循环查找目录中的每个元素
			for (int i = 0; i < NUM_SUB_DIR; i++)
			{
				// 如果找到对应子目录
				if (paths[ipaths] == fatherDir->d_filename[i])
				{
					ipaths++;
					isFind = true;
					pInode = this->IGet(fatherDir->d_inodenumber[i]);
					break;
				}
			}

			// 如果没有找到对应的文件或目录
			if (!isFind)
				return NULL;
		}
		else // 不是目录文件是错误的
			return NULL;
	}

	// 到这个部分说明找到了对应的文件或者目录
	return pInode;
}

/**************************************************************
* Alloc 分配空闲缓冲区
* 参数：
* 返回值：Buf* 返回分配到的缓冲区，如果分配失败，返回NULL
***************************************************************/
Buf* FileSystem::Alloc()
{
    int blkno; // 分配到的空闲磁盘块编号
    Buf* pBuf;

    // 从索引表“栈顶”获取空闲磁盘块编号
    blkno = this->spb->s_free[--this->spb->s_nfree];

    // 已分配尽所有的空闲磁盘块，直接返回
    if (0 == blkno)
    {
        this->spb->s_nfree = 0;
        cout << "磁盘已满!没有空余盘块" << endl;
        throw(ENOSPC);
        return NULL;
    }

    // 空闲磁盘块索引表已空，下一组空闲磁盘块的编号读入SuperBlock的s_free
    if (this->spb->s_nfree <= 0)
    {
        // 读入该空闲磁盘块
        pBuf = this->bufManager->Bread(blkno);

        int* p = (int*)pBuf->b_addr;

        // 首先读出空闲盘块数s_nfre
        this->spb->s_nfree = (int)p[0];

        // 根据空闲盘块数读取空闲盘块索引表
        for (int i = 0; i < this->spb->s_nfree; i++)
            this->spb->s_free[i] = (int)p[i + 1];
    }

    // 这样的话分配一空闲磁盘块，返回该磁盘块的缓存指针
    pBuf = this->bufManager->GetBlk(blkno); // 为该磁盘块申请缓存
    this->bufManager->CleanBuf(pBuf);         // 清空缓存中的数据

    return pBuf;
}

/**************************************************************
* FAlloc 分配空闲打开文件控制块File结构
* 参数：
* 返回值：File* 返回分配到的打开文件控制块File结构，如果分配失败，返回NULL
***************************************************************/
File* FileSystem::FAlloc(int& iloc)
{
	for (int i = 0; i < NUM_FILE; i++)
		if (this->openFileTable[i].f_inode == NULL)
		{
			iloc = i;
			return &this->openFileTable[i];
		}
	iloc = -1;
	return NULL;
}

/**************************************************************
* mkdir 创建文件夹
* 参数：path 文件夹路径
* 返回值：int 创建成功为0，否则为-1
***************************************************************/
int FileSystem::mkdir(string path)
{
	vector<string> paths = stringSplit(path, '/');
	if (paths.size() == 0)
	{
		cout << "路径无效!" << endl;
		throw(EINVAL);
		return -1;
	}

	string name = paths[paths.size() - 1];
	if (name.size() > NUM_FILE_NAME)
	{
		cout << "新建目录名过长!" << endl;
		throw(ENAMETOOLONG);
		return -1;
	}
	else if (name.size() == 0)
	{
		cout << "新建目录名不能为空!" << endl;
		throw(EINVAL);
		return -1;
	}

	Inode* fatherInode;

	// 从路径中删除文件夹名
	path.erase(path.size() - name.size(), name.size());
	// 找到想要创建的文件夹名的父文件夹相应的Inode
	fatherInode = this->NameI(path);

	// 没有找到相应的Inode
	if (fatherInode == NULL)
	{
		cout << "没有找到对应的文件或目录!" << endl;
		throw(ENOENT);
		return -1;
	}

	// 如果找到，判断创建的文件夹的父文件夹是不是文件夹类型
	if (!(fatherInode->i_mode & Inode::INodeMode::IDIR))
	{
		cout << "不是一个正确的目录项!" << endl;
		throw(ENOTDIR);
		return -1;
	}

	if (this->Access(fatherInode, FileMode::WRITE) == 0) // 判断是否有权限写文件
	{
		cout << "没有权限写文件!" << endl;
		throw(EACCES);
		return -1;
	}

	bool isFull = true;
	// 当有权限写文件时，判断是否有重名文件而且查看是否有空闲的子目录可以写
	// 计算要读的物理盘块号
	// 由于目录文件只占一个盘块，所以只有一项不为空
	int blkno = fatherInode->Bmap(0);
	// 读取磁盘的数据
	Buf* fatherBuf = this->bufManager->Bread(blkno);
	// 将数据转为目录结构
	Directory fatherDir = *Char_to_Directory(fatherBuf->b_addr);
	// 循环查找目录中的每个元素
	for (int i = 0; i < NUM_SUB_DIR; i++)
	{
		// 如果找到对应子目录
		if (name == fatherDir.d_filename[i])
		{
			cout << "文件已存在!" << endl;
			throw(EEXIST);
			return -1;
		}
		if (isFull && fatherDir.d_inodenumber[i] == 0)
		{
			isFull = false;
		}
	}

	// 如果目录已满
	if (isFull)
	{
		cout << "目录已满!" << endl;
		throw(ENOSPC);
		return -1;
	}

	// 这才开始创建新的文件夹
	// 分配一个新的内存Inode
	Inode* newinode = this->IAlloc();
	newinode->i_mode = Inode::INodeMode::IDIR |
		Inode::INodeMode::OWNER_R | Inode::INodeMode::OWNER_W |
		Inode::INodeMode::GROUP_R |
		Inode::INodeMode::OTHER_R;
	newinode->i_nlink = 1;
	newinode->i_uid = this->curId;
	newinode->i_gid = this->userTable->GetGId(this->curId);
	newinode->i_size = 0;
	newinode->i_mtime = unsigned int(time(NULL));
	newinode->i_atime = unsigned int(time(NULL));
	// 给新文件夹添加两个目录项
	Directory newDir;
	newDir.mkdir(".", newinode->i_number);	   // 创建自己
	newDir.mkdir("..", fatherInode->i_number); // 创建父亲
	// 跟新文件夹分配数据盘块号
	Buf* newBuf = this->Alloc();
	memcpy(newBuf->b_addr, Directory_to_Char(&newDir), sizeof(Directory));
	newinode->i_size = sizeof(Directory) / NUM_SUB_DIR * 2; // 新文件夹大小是两个目录项
	newinode->i_addr[0] = newBuf->b_blkno;

	// 将新文件夹写入其父亲的目录项中
	fatherDir.mkdir(name.c_str(), newinode->i_number);
	fatherInode->i_size += sizeof(Directory) / NUM_SUB_DIR; // 父亲的大小增加一个目录项
	fatherInode->i_atime = unsigned int(time(NULL));
	fatherInode->i_mtime = unsigned int(time(NULL));
	memcpy(fatherBuf->b_addr, Directory_to_Char(&fatherDir), sizeof(Directory));

	// 统一写回：父目录inode，新目录inode，父目录数据块、新目录数据块
	fatherInode->WriteI();
	newinode->WriteI();

	this->bufManager->Bwrite(fatherBuf);
	this->bufManager->Bwrite(newBuf);
	// 除FS里的rootInode和curInode外释放所有Inode
	if (fatherInode != this->rootDirInode && fatherInode != this->curDirInode)
		this->IPut(fatherInode);
	this->IPut(newinode);

	return 0;
}

/**************************************************************
* fcreate 创建文件
* 参数：path 文件路径
* 返回值：int 创建成功为0，否则为-1
***************************************************************/
int FileSystem::fcreate(string path)
{
	vector<string> paths = stringSplit(path, '/');
	if (paths.size() == 0)
	{
		cout << "路径无效!" << endl;
		throw(EINVAL);
		return -1;
	}
	string name = paths[paths.size() - 1];
	if (name.size() > NUM_FILE_NAME)
	{
		cout << "文件名过长!" << endl;
		throw(ENAMETOOLONG);
		return -1;
	}
	else if (name.size() == 0)
	{
		cout << "文件名不能为空!" << endl;
		throw(EINVAL);
		return -1;
	}

	Inode* fatherInode;

	// 从路径中删除文件名
	path.erase(path.size() - name.size(), name.size());
	// 找到想要创建的文件的父文件夹相应的Inode
	fatherInode = this->NameI(path);

	// 没有找到相应的Inode
	if (fatherInode == NULL)
	{
		cout << "没有找到对应的文件或目录!" << endl;
		throw(ENOENT);
		return -1;
	}

	// 如果找到，判断创建的文件的父文件夹是不是文件夹类型
	if (!(fatherInode->i_mode & Inode::INodeMode::IDIR))
	{
		cout << "不是一个正确的目录项!" << endl;
		throw(ENOTDIR);
		return -1;
	}

	if (this->Access(fatherInode, FileMode::WRITE) == 0) // 判断是否有权限写文件
	{
		cout << "没有权限写文件!" << endl;
		throw(EACCES);
		return -1;
	}

	bool isFull = true;
	int iinDir = 0;
	// 当有权限写文件时，判断是否有重名文件而且查看是否有空闲的子目录可以写
	// 计算要读的物理盘块号
	// 由于目录文件只占一个盘块，所以只有一项不为空
	int blkno = fatherInode->Bmap(0);
	// 读取磁盘的数据
	Buf* fatherBuf = this->bufManager->Bread(blkno);
	// 将数据转为目录结构
	Directory* fatherDir = Char_to_Directory(fatherBuf->b_addr);
	// 循环查找目录中的每个元素
	for (int i = 0; i < NUM_SUB_DIR; i++)
	{
		// 如果找到对应子目录
		if (name == fatherDir->d_filename[i])
		{
			cout << "文件已存在!" << endl;
			throw(EEXIST);
			return -1;
		}
		if (isFull && fatherDir->d_inodenumber[i] == 0)
		{
			isFull = false;
			iinDir = i;
		}
	}

	// 如果目录已满
	if (isFull)
	{
		cout << "目录已满!" << endl;
		throw(ENOSPC);
		return -1;
	}

	// 这才开始创建新的文件
	// 分配一个新的内存Inode
	Inode* newinode = this->IAlloc();
	newinode->i_mode = Inode::INodeMode::IFILE |
		Inode::INodeMode::OWNER_R | Inode::INodeMode::OWNER_W |
		Inode::INodeMode::GROUP_R |
		Inode::INodeMode::OTHER_R;
	newinode->i_nlink = 1;
	newinode->i_uid = this->curId;
	newinode->i_gid = this->userTable->GetGId(this->curId);
	newinode->i_size = 0;
	newinode->i_mtime = unsigned int(time(NULL));
	newinode->i_atime = unsigned int(time(NULL));

	// 将新的Inode写回磁盘中
	newinode->WriteI();

	// 将文件写入目录项中
	fatherDir->mkdir(name.c_str(), newinode->i_number);
	
	memcpy(fatherBuf->b_addr, Directory_to_Char(fatherDir), sizeof(Directory));
	this->bufManager->Bwrite(fatherBuf);

	fatherInode->i_size += sizeof(Directory) / NUM_SUB_DIR; // 父亲的大小增加一个目录项
	fatherInode->i_atime = unsigned int(time(NULL));
	fatherInode->i_mtime = unsigned int(time(NULL));

	fatherInode->WriteI();

	// 释放所有Inode
	if (fatherInode != this->rootDirInode && fatherInode != this->curDirInode)
		this->IPut(fatherInode);
	this->IPut(newinode);

	return 0;
}

/**************************************************************
* fdelete 删除文件
* 参数：path 文件路径
* 返回值：int 删除成功为0，否则为-1
***************************************************************/
int FileSystem::fdelete(string path)
{
	vector<string> paths = stringSplit(path, '/');
	if (paths.size() == 0)
	{
		cout << "路径无效!" << endl;
		throw(EINVAL);
		return -1;
	}
	string name = paths[paths.size() - 1];
	if (name.size() > NUM_FILE_NAME)
	{
		cout << "文件名过长!" << endl;
		throw(ENAMETOOLONG);
		return -1;
	}
	else if (name.size() == 0)
	{
		cout << "文件名不能为空!" << endl;
		throw(EINVAL);
		return -1;
	}

	Inode* fatherInode;

	// 从路径中删除文件名
	path.erase(path.size() - name.size(), name.size());
	// 找到想要创建的文件的父文件夹相应的Inode
	fatherInode = this->NameI(path);

	// 没有找到相应的Inode
	if (fatherInode == NULL)
	{
		cout << "没有找到" << path << endl;
		throw(ENOENT);
		return -1;
	}
	// 如果找到，判断创建的文件的父文件夹是不是文件夹类型
	if (!(fatherInode->i_mode & Inode::INodeMode::IDIR))
	{
		cout << "不是一个正确的目录项!" << endl;
		throw(ENOTDIR);
		return -1;
	}

	if (this->Access(fatherInode, FileMode::WRITE) == 0)
	{
		cout << "没有权限删除(写)文件!" << endl;
		throw(EACCES);
		return -1;
	}

	// 普通情况，删除子文件
	// 当有权限写文件时，判断是否有重名文件而且查看是否有空闲的子目录可以写
	// 计算要读的物理盘块号
	// 由于目录文件只占一个盘块，所以只有一项不为空
	int blkno = fatherInode->Bmap(0);
	// 读取磁盘的数据
	Buf* fatherBuf = this->bufManager->Bread(blkno);
	// 将数据转为目录结构
	Directory* fatherDir = Char_to_Directory(fatherBuf->b_addr);
	bool isFind = false;
	int iinDir = 0;
	for (int i = 0; i < NUM_SUB_DIR; i++)
	{
		// 如果找到对应文件
		if (!isFind && name == fatherDir->d_filename[i])
		{
			isFind = true;
			iinDir = i;
		}
	}

	if (!iinDir)
	{
		cout << "未找到所要删除的文件!" << endl;
		return -1;
	}

	// 获取所要删除的文件的Inode
	Inode* pDeleteInode = this->IGet(fatherDir->d_inodenumber[iinDir]);
	if (NULL == pDeleteInode)
	{
		cout << "未找到所要删除的文件!" << endl;
		return -1;
	}
	if (pDeleteInode->i_mode & Inode::INodeMode::IDIR) // 如果是文件夹类型
	{
		cout << "请输入正确的文件名!" << endl;
		return -1;
	}

	if (this->Access(pDeleteInode, FileMode::WRITE) == 0)
	{
		cout << "没有权限删除(写)文件!" << endl;
		throw(EACCES);
		return -1;
	}

	// 删除父目录下的文件项
	fatherDir->rmi(iinDir);
	fatherInode->i_size -= sizeof(Directory) / NUM_SUB_DIR; // 父亲的大小减小一个目录项
	fatherInode->i_atime = unsigned int(time(NULL));
	fatherInode->i_mtime = unsigned int(time(NULL));
	memcpy(fatherBuf->b_addr, Directory_to_Char(fatherDir), sizeof(Directory));

	// 统一写回：父目录inode 父目录数据块
	fatherInode->WriteI();
	this->bufManager->Bwrite(fatherBuf);

	// 释放所有Inode
	if (fatherInode != this->rootDirInode && fatherInode != this->curDirInode)
		this->IPut(fatherInode);

	// 释放Inode
	pDeleteInode->i_nlink--;
	this->IPut(pDeleteInode); // 当i_nlink为0时，会释放Inode

	return 0;
}

/**************************************************************
* fwrite 写文件
* 参数：buffer 写入的内容 count 写入的字节数 fp 文件指针
* 返回值：
***************************************************************/
void FileSystem::fwrite(const char* buffer, int count, File* fp)
{
	if (fp == NULL)
	{
		cout << "文件指针为空!" << endl;
		throw(EBADF);
		return;
	}

	// 如果找到，判断是否有权限打开文件
	if (this->Access(fp->f_inode, FileMode::WRITE) == 0)
	{
		cout << "没有权限写文件!" << endl;
		throw(EACCES);
		return;
	}

	if (count + fp->f_offset > SIZE_BLOCK * NUM_BLOCK_ILARG)
	{
		cout << "写入文件太大!" << endl;
		throw(EFBIG);
		return;
	}
	// 获取文件的Inode
	Inode* pInode = fp->f_inode;

	// 写文件的三种情况：
	// 1. 写入的起始位置为逻辑块的起始地址；写入字节数为512------异步写
	// 2. 写入的起始位置不是逻辑块的起始地址；写入字节数为512----先读后写
	//  2.1 写到缓存末尾-----------------------------------------异步写
	//	2.2 没有写到缓存末尾-------------------------------------延迟写

	int pos = 0; // 已经写入的字节数
	while (pos < count)
	{
		// 计算本次写入位置在文件中的位置
		int startpos = fp->f_offset;
		// 计算本次写入物理盘块号，如果文件大小不够的话会在里面新分配物理盘块
		int blkno = pInode->Bmap(startpos / SIZE_BLOCK);
		// 计算本次写入的大小
		int size = SIZE_BLOCK - startpos % SIZE_BLOCK;
		if (size > count - pos)
			size = count - pos; // 修正写入的大小

		cout << "[writing block no]:" << blkno << endl;
		// 如果写入的起始位置为逻辑块的起始地址；写入字节数为512-------异步写
		if (startpos % SIZE_BLOCK == 0 && size == SIZE_BLOCK)
		{
			// 申请缓存
			Buf* pBuf = this->bufManager->GetBlk(blkno);
			// 将数据写入缓存
			memcpy(pBuf->b_addr, buffer + pos, size);
			// 将数据立即写入磁盘
			this->bufManager->Bwrite(pBuf);
		}
		else
		{	// 写入的起始位置不是逻辑块的起始地址；写入字节数为512----先读后写
			// 申请缓存
			Buf* pBuf = this->bufManager->Bread(blkno);
			// 将数据写入缓存
			memcpy(pBuf->b_addr + startpos % SIZE_BLOCK, buffer + pos, size);

			// 写到缓存末尾---异步写
			if (startpos % SIZE_BLOCK + size == SIZE_BLOCK)
				this->bufManager->Bwrite(pBuf);
			else // 没有写到缓存末尾---延迟写
				this->bufManager->Bdwrite(pBuf);
		}

		pos += size;
		fp->f_offset += size; // 调整文件指针
	}

	if (unsigned int (pInode->i_size) < fp->f_offset)
		pInode->i_size = fp->f_offset; // 调整文件大小

	pInode->i_mtime = unsigned int(time(NULL));
	pInode->i_atime = unsigned int(time(NULL)); // 更改文件信息
}

/**************************************************************
* fopen 打开文件
* 参数：path 文件路径
* 返回值：File* 返回打开文件的指针
***************************************************************/
int FileSystem::fopen(string path)
{
	Inode* pinode = this->NameI(path);

	// 没有找到相应的Inode
	if (pinode == NULL)
	{
		cout << "没有找到对应的文件或目录!" << endl;
		throw(ENOENT);
		return -1;
	}

	// 如果找到，判断所要找的文件是不是文件类型
	if (!(pinode->i_mode & Inode::INodeMode::IFILE))
	{
		cout << "不是一个正确的文件!" << endl;
		throw(ENOTDIR);
		return -1;
	}

	// 判断是否有读权限打开文件
	if (this->Access(pinode, FileMode::READ) == 0)
	{
		cout << "没有权限读文件!" << endl;
		throw(EACCES);
		return -1;
	}

	// 分配打开文件控制块File结构
	int fileloc = 0;
	File* pFile = this->FAlloc(fileloc);
	if (NULL == pFile)
	{
		cout << "打开太多文件!" << endl;
		throw(ENFILE);
		return -1;
	}
	pFile->f_inode = pinode;
	pFile->f_offset = 0;
	pFile->f_uid = this->curId;
	pFile->f_gid = this->userTable->GetGId(this->curId);

	// 修改访问时间
	pinode->i_atime = unsigned int(time(NULL));

	return fileloc;
}

/**************************************************************
* fclose 关闭文件
* 参数：fp 文件指针
* 返回值：
***************************************************************/
// 根据fd关闭文件
void FileSystem::fclose(File* fp)
{
	// 释放内存结点
	this->IPut(fp->f_inode);
	fp->Clean();
}

/**************************************************************
* fread 读文件到字符串中
* 参数：fp 文件指针 buffer 读取内容索要存放的字符串 count  读取的字节数
* 返回值：
***************************************************************/
void FileSystem::fread(File* fp, char*& buffer, int count)
{
	if (fp == NULL)
	{
		cout << "文件指针为空!" << endl;
		throw(EBADF);
		return;
	}

	if (this->Access(fp->f_inode, FileMode::READ) == 0)
	{
		cout << "没有权限读文件!" << endl;
		throw(EACCES);
		return;
	}

	// 获取文件的Inode
	Inode* pInode = fp->f_inode;
	if (count > 0)
		buffer = new char[count + 1];
	else
	{
		buffer = NULL;
		return;
	}
	int pos = 0; // 已经读取的字节数
	while (pos < count)
	{
		// 计算本读取位置在文件中的位置
		int startpos = fp->f_offset;
		if (startpos >= pInode->i_size) // 读取位置超出文件大小
			break;
		// 计算本次读取物理盘块号，由于上一个判断,不会有读取位置超出文件大小的问题
		int blkno = pInode->Bmap(startpos / SIZE_BLOCK);
		// 计算本次读取的大小
		int size = SIZE_BLOCK - startpos % SIZE_BLOCK;
		if (size > count - pos)
			size = count - pos; // 修正读取的大小

		Buf* pBuf = this->bufManager->Bread(blkno);
		// TODO:如有大文件这里需要改
		memcpy(buffer + pos, pBuf->b_addr + startpos % SIZE_BLOCK, size);
		pos += size;
		fp->f_offset += size;
	}
	buffer[count] = '\0'; // 设置结束标记

	pInode->i_atime = unsigned int(time(NULL));
}

/**************************************************************
* fseek 移动文件指针
* 参数：fp 文件指针 offset 偏移量 mode 移动方式,SEEK_SET,SEEK_CUR,SEEK_END
* 返回值：int 移动成功为0，否则为-1
***************************************************************/
int FileSystem::fseek(File* fp, int offset, int mode)
{
	if (SEEK_SET == mode) // 从文件头开始
	{
		if (offset >= 0 && offset <= fp->f_inode->i_size)
			fp->f_offset = offset;
		else
			return -1;
	}
	else if (SEEK_CUR == mode) // 从当前位置开始
	{
		if (offset >= 0 && (fp->f_offset + offset) >= 0)
			fp->f_offset += offset;
		else
			return -1;
	}
	else if (SEEK_END == mode) // 从文件尾开始
	{
		if (offset >= 0 && (fp->f_inode->i_size - 1 + offset) >= 0)
			fp->f_offset = fp->f_inode->i_size - 1 + offset;
		else
			return -1;
	}
	else
		return -1;
	return 0;
}

/**************************************************************
* Access 查找pInode是否有给定mode的权限
* 参数：pInode 要查找的Inode指针 mode   要查找的权限
* 返回值：如果有权限，返回1，否则返回0
***************************************************************/
int FileSystem::Access(Inode* pInode, unsigned int mode)
{
	// 如果是文件所有者 或者是 ROOT
	if (this->curId == pInode->i_uid || this->curId == ROOT_ID)
	{
		if (mode == FileMode::WRITE) // 写权力前提是有读权力
			return (pInode->i_mode & Inode::INodeMode::OWNER_R) && (pInode->i_mode & Inode::INodeMode::OWNER_W);
		else if (mode == FileMode::READ)
			return pInode->i_mode & Inode::INodeMode::OWNER_R;
		else
			return 0;
	}

	// 组和其他人暂时不考虑，只有一个用户
	return 0;
}
