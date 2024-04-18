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
		cout << "无法打开一级文件myDisk.img" << endl;
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
	int filoc = fopen("/etc/userTable.txt");
	File* userTableFile = &this->openFileTable[filoc];
	this->fwrite(userTable2Char(this->userTable), sizeof(UserTable), userTableFile); // 需要全部写入
	this->fclose(userTableFile);
}

void FileSystem::exit() {

}

void FileSystem::init() {

}

void FileSystem::run() {

}