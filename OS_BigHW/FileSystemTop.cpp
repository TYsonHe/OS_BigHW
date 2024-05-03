#include "header.h"
#include "utils.h"


/**************************************************************
* fformat初始化文件系统
* 参数：
* 返回值：
***************************************************************/
void FileSystem::fformat() {
	fstream fd(DISK_PATH, ios::out);
	fd.close();
	fd.open(DISK_PATH, ios::out | ios::in | ios::binary);
	// 如果没有打开文件则输出提示信息并throw错误
	if (!fd.is_open())
	{
		cout << "无法打开磁盘文件myDisk.img" << endl;
		throw(errno);
	}
	fd.close();

	// 先对用户进行初始化，只有Root用户
	this->userTable = new UserTable();
	this->userTable->AddRoot(); // 添加root用户
	this->curId = ROOT_ID;
	this->curName = "root";

	// 初始化缓存
	this->bufManager = new BufferManager();
	this->spb = new SuperBlock();
	
	// 正式初始化SuperBlock
	this->spb->Init();
	// 将superblock写回磁盘
	this->WriteSpb();

	// 现在对目录进行初始化
	// 分配一个空闲的外存Inode来索引根目录
	this->rootDirInode = this->IAlloc();
	this->rootDirInode->i_uid = ROOT_ID;
	this->rootDirInode->i_gid = this->userTable->GetGId(ROOT_ID);
	this->rootDirInode->i_mode = Inode::INodeMode::IDIR |
		Inode::INodeMode::OWNER_R | Inode::INodeMode::OWNER_W |
		Inode::INodeMode::GROUP_R |
		Inode::INodeMode::OTHER_R;
	this->rootDirInode->i_nlink = 1;
	this->rootDirInode->i_size = 0;
	this->rootDirInode->i_mtime = unsigned int(time(NULL));
	this->rootDirInode->i_atime = unsigned int(time(NULL));
	this->curDirInode = this->rootDirInode;

	// 构建根目录
	Directory rootDir; // 分配一个数据盘块存放根目录内容
	rootDir.mkdir(".", this->rootDirInode->i_number);  // 创建自己
	rootDir.mkdir("..", this->rootDirInode->i_number); // 创建父亲，根目录的父亲也是自己
	this->curDir = "/";

	// 给根目录分配数据盘块号
	Buf* newBuf = this->Alloc();
	memcpy(newBuf->b_addr, Directory_to_Char(&rootDir), sizeof(Directory));
	
	// 并且写回磁盘数据区中
	this->bufManager->Bwrite(newBuf);
	// 给根目录的Inode写回数据区
	this->rootDirInode->i_size = sizeof(Directory) / NUM_SUB_DIR * 2;
	this->rootDirInode->i_addr[0] = newBuf->b_blkno;

	// 根据要求添加目录
	this->mkdir("/bin");
	this->mkdir("/etc");
	this->mkdir("/home");
	this->mkdir("/dev");
	// 将rootInode写回磁盘中
	this->rootDirInode->WriteI();

	// 创建并写入用户表
	// 但是我们暂时只有一个root用户
	this->fcreate("/etc/userTable.txt");
	//int filoc = fopen("/etc/userTable.txt");
	//File* userTableFile = &this->openFileTable[filoc];
	//this->fwrite(UserTable_to_Char(this->userTable), sizeof(UserTable), userTableFile); // 需要全部写入
	//this->fclose(userTableFile);
	this->WriteUserTable();
}

/**************************************************************
* exit 退出文件系统
* 参数：
* 返回值：
***************************************************************/
void FileSystem::exit() 
{
	this->curId = ROOT_ID; // 保证权限
	// 将superblock写回磁盘
	this->WriteSpb();
	// 将userTable写回磁盘，
	this->WriteUserTable();

	// 把所有文件都关闭
	for (const auto& pair : this->openFileMap)
	{
		this->fclose(&(this->openFileTable[pair.second - 1]));
	}
	this->openFileMap.clear();

    // 安全退出
	// 将所有的内容都写回磁盘,即延迟写的Buf
	this->bufManager->Flush();
}

/**************************************************************
* init 初始化文件系统，读取已经存在的myDisk.img
* 参数：
* 返回值：
***************************************************************/
void FileSystem::init()
{
	fstream fd(DISK_PATH, ios::out | ios::in | ios::binary);
	// 如果没有打开文件则输出提示信息并throw错误
	if (!fd.is_open())
	{
		cout << "无法打开磁盘文件myDisk.img" << endl;
		throw(errno);
	}
	fd.close();

	// 对缓存相关内容进行初始化
	this->bufManager = new BufferManager();

	// 读取超级块，超级块有两个！！
	char* ch = new char[sizeof(SuperBlock)];
	Buf* buf = this->bufManager->Bread(POSITION_SUPERBLOCK);
	memcpy(ch, buf->b_addr, SIZE_BLOCK);
	buf = this->bufManager->Bread(POSITION_SUPERBLOCK + 1);
	memcpy(ch + SIZE_BLOCK, buf->b_addr, SIZE_BLOCK);
	this->spb = Char_to_SuperBlock(ch); // 不能删掉ch

	// 读取根目录Inode
	buf = this->bufManager->Bread(POSITION_DISKINODE);
	this->rootDirInode = this->IAlloc();
	this->rootDirInode->ICopy(buf, ROOT_DIR_INUMBER);
	this->curDirInode = this->rootDirInode;
	this->curId = ROOT_ID; // 我们只有一个root用户
	this->curDir = "/";

	// 读取用户信息表
	// 不能直接调用this->fopen，因为userTable本身还没有初始化
	Inode* pinode = this->NameI("/etc/userTable.txt");
	// 没有找到相应的Inode
	if (pinode == NULL)
	{
		cout << "没有找到/etc/userTable.txt!" << endl;
		throw(ENOENT);
		return;
	}
	// 如果找到，判断所要找的文件是不是文件类型
	if (!(pinode->i_mode & Inode::INodeMode::IFILE))
	{
		cout << "不是一个正确的/etc/userTable.txt文件!" << endl;
		throw(ENOTDIR);
		return;
	}
	Buf* bp = this->bufManager->Bread(pinode->Bmap(0)); // userTable.txt文件本身只占一个盘块大小
	this->userTable = Char_to_UserTable(bp->b_addr);
}

/**************************************************************
* run 运行文件系统的终端程序
* 参数：
* 返回值：
***************************************************************/
void FileSystem::run()
{
    this->login();

    cout << "请输入help查看使用指南" << endl;
    vector<string> input;
    string strIn;
    bool needEndl = true;
    while (true)
    {
        input.clear();
        if(needEndl)
            cout << endl;
        cout<< this->curName<<"@FileSystem-2153698    " << this->curDir << ">"; // 保持和Linux控制台一致
        getline(cin, strIn);
        input = stringSplit(strIn, ' ');
        if (input.size() == 0)
        {
            needEndl = false;
            continue;
        }

        try
        {
            if (input.size() < 1)
            {
                needEndl = false;
                continue;
            }
            else if (input[0] == "clear")
            {
                needEndl = false;
                system("cls");
            }
            else
            {
                // 目录管理
                if (input[0] == "ls") // 查看子目录
                {
                    if (input.size() > 1)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->ls();
                    needEndl = true;
                }
                else if (input[0] == "cd") // 进入子目录
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->cd(input[1]);
                    needEndl = false;

                }
                else if (input[0] == "rmdir") // 删除子目录
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->rmdir(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "mkdir")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->mkdir_terminal(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "ll")
                {
                    if (input.size() < 1 || input.size() > 1)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->ll();
                    needEndl = false;
                }

                // 文件管理
                else if (input[0] == "touch")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->createFile(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "rm")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->removeFile(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "open")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->openFile(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "close")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->closeFile(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "write")
                {
                    if (input.size() < 2 || input.size() > 3)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    int mode = input.size() == 3 ? atoi(input[2].c_str()) : 0;
                    this->writeFile(input[1], mode);
                }
                else if (input[0] == "cat")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->printFile(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "cpffs")
                {
                    if (input.size() < 4 || input.size() > 4)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->copy_from_fs(input[1], input[2], stoi(input[3]));
                    needEndl = false;
                }
                else if (input[0] == "cpfwin")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->copy_from_win(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "chmod")
                {
                    if (input.size() < 3 || input.size() > 3)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->chmod(input[1], input[2]);
                }
                else if (input[0] == "lsfs")
                {
                    if (input.size() < 1 || input.size() > 1)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->print0penFileList();
                    needEndl = false;
                }
                else if (input[0] == "fseek")
                {
                    if (input.size() < 3 || input.size() > 3)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->change_fseek(input[1], stoi(input[2]));
                }
                else if (input[0] == "flseek")
                {
                    if (input.size() != 2)
                    {
                        cout << "输入非法!" << endl;
                        continue;
                    }
                    this->flseek(input[1]);
                }
                else if (input[0] == "format")
                {
                    this->format();
                    this->exit();
                    this->login();
                }
                else if (input[0] == "help")
                    this->help();
                else if (input[0] == "pwd")
                {
                    this->pwd();
                    needEndl = false;
                }
                else if (input[0] == "exit")
                {
                    this->exit();
                    break;
                }
                else
                {
                    cout << "指令不存在，请通过help查阅指令和帮助信息";
                }
            }
        }
        catch (int& e)
        {
            cerr << "error code：" << e << endl;
            cout << "可以查看Linux错误码" << endl;
        }
    }
}