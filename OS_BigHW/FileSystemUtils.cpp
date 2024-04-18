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
        for (int i = 0; i < this->spb->s_isize; i++)
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
                 * 从源码中得到的注释
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

	// 如果找到，判断是否有权限写文件
	if (this->Access(fatherInode, FileMode::WRITE) == 0)
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
	Directory fatherDir = *char2Directory(fatherBuf->b_addr);
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

	// 如果找到，判断是否有权限写文件
	if (this->Access(fatherInode, FileMode::WRITE) == 0)
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
	Directory* fatherDir = char2Directory(fatherBuf->b_addr);
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