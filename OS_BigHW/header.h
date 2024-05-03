#pragma once
#ifndef HEADER_H
#define HEADER_H

#define _CRT_SECURE_NO_WARNINGS

// 设置唯一用户ROOT的信息
#define ROOT_ID 0
#define ROOT_GID 0
#define ROOT_DIR_INUMBER 1

// 设置颜色输出
#define RESET   "\033[0m"
#define RED     "\033[31m"      /* Red */
#define BLUE    "\033[34m"      /* Blue */
#define YELLOW  "\033[33m"      /* YELLOW */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */

#include <iostream>
#include <conio.h>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <map>
using namespace std;

// 文件卷名称
static const string DISK_PATH = "myDisk.img";
// 文件逻辑块大小: 512字节
static const int SIZE_BLOCK = 512;
// 文件卷大小
static const int SIZE_DISK = SIZE_BLOCK * 8192;
// 文件所有Block数量
static const int NUM_BLOCK_ALL = SIZE_DISK / SIZE_BLOCK;

// DiskInode数量
static const int NUM_DISKINODE = 256;
// DiskInode中可以使用的最大物理块数量
static const int NUM_I_ADDR = 10;
// DiskInode大小（以字节为单位）
static const int SIZE_DISKINODE = 64;
// DiskInode开始的位置（以block为单位）
static const unsigned int POSITION_DISKINODE = 2;
// 每个Block中DiskInode的数量
static const int NUM_INODE_PER_BLOCK = SIZE_BLOCK / SIZE_DISKINODE;

// SuperBlock中能够管理的最大空闲Inode与空闲数据盘块的数量
static const int NUM_FREE_INODE = 100;
// SuperBlock开始的位置（以block为单位）
static const unsigned int POSITION_SUPERBLOCK = 0;
// SuperBlock一组空闲数据盘块的数量
static const int NUM_FREE_BLOCK_GROUP = 100;
// SuperBlock本身有的大小
static const int SIZE_SUPERBLOCK = 816;
// SuperBlock需要填充的字节数
static const int SIZE_PADDING = POSITION_DISKINODE * SIZE_BLOCK - SIZE_SUPERBLOCK;

// 数据块Block数量
static const int NUM_BLOCK = NUM_BLOCK_ALL - POSITION_DISKINODE - NUM_DISKINODE / NUM_INODE_PER_BLOCK;
// 数据块Block开始的位置（以block为单位）
static const unsigned int POSITION_BLOCK = int(POSITION_DISKINODE + SIZE_DISKINODE * NUM_DISKINODE / SIZE_BLOCK);

// 规定：User内容最多占一个BLOCK，即:NUM_USER*(NUM_USER_NAME+NUM_USER_PASSWORD)<=BLOCK_SIZE
// User中最多用户数
static const int NUM_USER = 8;
// User用户名称的最大长度
static const unsigned int NUM_USER_NAME = (SIZE_BLOCK / NUM_USER - sizeof(short) * 2) / 2;
// User用户密码的最大长度
static const unsigned int NUM_USER_PASSWORD = (SIZE_BLOCK / NUM_USER - sizeof(short) * 2) / 2;
// User开始的位置（以block为单位）
static const unsigned int POSITION_USER = POSITION_BLOCK + 1;

// BufferManager缓存控制块、缓冲区的数量
static const int NUM_BUF = 15;
// BufferManager缓冲区大小。 以字节为单位
static const int SIZE_BUFFER = SIZE_BLOCK;

// 规定：根目录在数据区的第一个Block中
// Directory中一个目录下子目录文件名最大长度
static const int NUM_FILE_NAME = 28;
// Directory中一个目录下最多子目录个数
static const int NUM_SUB_DIR = SIZE_BLOCK / (NUM_FILE_NAME + sizeof(int));
// Directory开始的位置（以block为单位）
static const unsigned int POSITION_DIRECTORY = POSITION_BLOCK;

// FileSystem:OpenFileTable中最多打开文件数
static const int NUM_FILE = 100;
// FileSystem:IndoeTable中最多Inode数
static const int NUM_INODE = 100;

// i_addr索引设计
// 大型文件使用的最多的盘块数量
static const int NUM_FILE_INDEX = SIZE_BLOCK / sizeof(int);
static const int NUM_BLOCK_IFILE = 5; // 大文件逻辑索引开始的号，表明i_addr[0]-[4]为直接索引，i_addr[5-10]为间接索引
static const int NUM_BLOCK_ILARG = NUM_FILE_INDEX * (NUM_I_ADDR - NUM_BLOCK_IFILE) + NUM_BLOCK_IFILE;

/**************************************************************
* Buf 缓存块Buffer的定义 （也叫缓存控制块）
* 用途：作为缓存块存储信息
* 特殊：有对应的Flag记录缓存块使用信息
*		只有一个设备
***************************************************************/
class Buf
{
public:
	enum BufFlag
	{
		B_NONE = 0x0,	// 初始化（未定义使用时）
		B_WRITE = 0x1,	// 写操作标记。
		B_READ = 0x2,	// 读操作标记。
		B_DONE = 0x4,	// I/O操作结束
		B_DELWRI = 0x80 // 延迟写标记
	};

public:
	unsigned int b_blkno;  // 缓存块对应的内存逻辑块号
	char* b_addr;		   // 指向该缓存块的首地址
	unsigned int b_flags;  // 缓存控制块标志位,定义见enum BufFlag
	Buf* b_forw;		   // 当前缓存块的前驱节点,将Buf插入NODEV队列或某一设备队列
	Buf* b_back;		   // 当前缓存块的后继节点,同上
	Buf* av_forw;		   // 上一个空闲缓存块的指针,将Buf插入自由队列或某一I/O请求队列
	Buf* av_back;		   // 下一个空闲缓存块的指针,同上
	unsigned int b_wcount; // 需传送的字节数

	Buf();
	~Buf();
};

/**************************************************************
* BufferManager 缓存池（缓存控制块管理）类定义
* 用途：管理所有的Buf
* 特殊：有对应的Flag记录缓存块使用信息
*		只有一个设备
***************************************************************/
class BufferManager
{
private:
	Buf bFreeList;					   // 自由缓存队列（双向链表）
	Buf devtab;						   // 由于只有一个设备，所以只有一个设备表
	Buf m_Buf[NUM_BUF];				   // 缓存控制块数组
	char Buffer[NUM_BUF][SIZE_BUFFER]; // 缓冲区数组
public:
	// 构造函数
	BufferManager();
	~BufferManager();

	// 根据物理设备块号读取缓存
	Buf* GetBlk(int blkno);

	// 将缓存块bp写到磁盘上
	void Bwrite(Buf* bp);

	// 将缓存块延迟写
	void Bdwrite(Buf* bp);

	// 根据物理设备块号读取缓存
	Buf* Bread(int blkno);

	// 清理Buf
	void CleanBuf(Buf* bp);

	// 全部保存，写入磁盘
	void Flush();
};

/**************************************************************
* Directory 目录信息类
* 用途：作为文件系统的目录信息记录
* 特殊：实现了树状目录结构
*		一个Directory类就一个BLOCK大小
***************************************************************/
class Directory
{
public:
	int d_inodenumber[NUM_SUB_DIR];				 // 子目录Inode号
	char d_filename[NUM_SUB_DIR][NUM_FILE_NAME]; // 子目录文件名

	Directory();
	~Directory();

	// 根据目录名name和Inode号inumber给当前目录创建一个子目录
	int mkdir(const char* name, const int inumber);

	// 清除目录中的第iloc个子目录
	void rmi(int iloc);
};

/**************************************************************
* SuperBlock 超级块
* 用途：作为文件系统存储资源管理块
* 特殊：需要管理文件系统的许多资源信息
*		Inode区
*		空闲盘块
*		空闲外存Inode
***************************************************************/
class SuperBlock //1024字节 占2个扇区（块）
{
public:
	unsigned int s_isize;			  // Inode区尺寸，占用的数据块数量
	unsigned int s_fsize;			  // 磁盘数据块总数
	int s_ninode;					  // 直接管理的空闲外存Inode数量
	int s_inode[NUM_FREE_INODE];	  // 直接管理的空闲外存Inode索引表
	int s_nfree;					  // 直接管理的空闲盘块数量
	int s_free[NUM_FREE_BLOCK_GROUP]; // 直接管理的空闲盘块索引表
	char padding[SIZE_PADDING];		  // 填充字节

	// 构造&析构
	SuperBlock();
	~SuperBlock();

	// 初始化SuperBlock
	void Init();
};

/**************************************************************
* DiskINode 外存索引节点
* 用途：外存Inode位于文件存储设备上的外存Inode区
		每个文件有唯一对应的外存Inode
* 特殊：记录了该文件对应的控制信息。
*		外存INode对象长度为64字节
*		每个磁盘块可以存放512/64 = 8个外存Inode
***************************************************************/
class DiskInode // 64字节
{
public:
	unsigned int d_mode;	// 文件类型和访问控制位
	int d_nlink;			// 文件硬联结计数，即该文件在目录树中不同路径名的数量
	short d_uid;			// 文件所有者的用户id
	short d_gid;			// 文件所有者的组id
	int d_size;				// 文件大小，字节为单位
	int d_addr[NUM_I_ADDR]; // 地址映射表，登记逻辑块和物理块之间的映射关系
	unsigned int d_atime;	// 最后访问时间
	unsigned int d_mtime;	// 最后修改时间
};

/**************************************************************
* Inode 内存索引节点
* 用途：为文件系统中的所有的资源分配内存的索引
* 特殊：记录了资源信息
*		每一个打开的文件、当前访问目录、挂载的子文件系统
*		只有一个设备，仅需要i_number来确定位置
***************************************************************/
class Inode // 64字节
{
public:
	// i_mode中标志位
	enum INodeMode
	{
		IDIR = 0x4000,	// 文件类型：目录文件
		IFILE = 0x2000, // 文件类型：文件
		ILARG = 0x1000, // 文件类型：大文件,ILARG与IFILE一起出现
		OWNER_R = 0x200,
		OWNER_W = 0x100,
		GROUP_R = 0x20,
		GROUP_W = 0x10,
		OTHER_R = 0x2,
		OTHER_W = 0x1,
	};

public:
	short i_uid;			// 文件所有者的用户id
	short i_gid;			// 文件所有者的组id
	unsigned short i_mode;	// 文件工作方式信息
	short i_nlink;			// 文件硬联结计数，即该文件在目录树中不同路径名的数量
	int i_size;				// 文件大小，字节为单位
	int i_addr[NUM_I_ADDR]; // 指向数据块区，地址映射表，用于文件逻辑块号和物理块号转换的基本索引表
	unsigned int i_atime;	// 最后访问时间
	unsigned int i_mtime;	// 最后修改时间
	short i_count;			// 引用计数
	short i_number;			// 在inode区中的编号,将内存Inode转换为外存Inode

	Inode();
	~Inode();

	// 将逻辑块号lbn映射到物理盘块号phyBlkno
	int Bmap(int lbn);

	// 根据缓存内容bp将外存Inode读取数据到内存Inode
	void ICopy(Buf* bp, int inumber);

	// 根据规则给内存Inode赋予文件权限
	int AssignMode(unsigned short mode);

	// 清空Inode内容
	void Clean();

	// 将内存Inode更新到外存中
	void WriteI();

	// 获取父目录inodenumber
	int GetParentInumber();

	// 获取目录内容
	Directory* GetDir();

	// 释放数据盘块
	void ITrunc();

	// 根据id和gid获取文件权限字符串
	string GetModeString(int id, int gid);

	// 通过字符串获取mode
	unsigned short String_to_Mode(string mode);
};

/**************************************************************
* File 打开文件控制块
* 用途：记录文件系统中文件的信息
* 特殊：文件使用人的ids、文件读写位置等等
***************************************************************/
class File
{
public:
	Inode* f_inode;		   // 指向打开文件的内存Inode指针
	unsigned int f_offset; // 文件读写位置指针
	short f_uid;		   // 文件所有者的用户id
	short f_gid;		   // 文件所有者的组id

	File();
	~File();

	void Clean();
};

/**************************************************************
* UserTable 用户表
* 用途：文件系统的用户管理
*		记录用户名和密码
* 特殊：只有一个root用户
***************************************************************/
class UserTable
{
public:
	short u_id[NUM_USER];						  // 用户id
	short u_gid[NUM_USER];						  // 用户所在组id
	char u_name[NUM_USER][NUM_USER_NAME];		  // 用户名
	char u_password[NUM_USER][NUM_USER_PASSWORD]; // 用户密码

	// 方法：
	UserTable();
	~UserTable();

	// 添加root用户
	void AddRoot();

	// 获得组用户Id
	short GetGId(const short id);

	// 查找用户，登录功能
	short FindUser(const char* name, const char* password);
};

/**************************************************************
* FileSystem 文件系统顶层模块
* 用途：完成文件系统的整个管理功能
*		记录了终端的信息
*		缓存的管理通过 bufManager来管理
*		外存和文件系统资源的管理通过SuperBlock来管理
*		内存通过Inode表来管理
*		文件通过打开文件表管理
*		root用户有用户表
* 特殊：只有一个设备，一个进程
***************************************************************/
class FileSystem
{
private:
	short curId;				  // 目前使用的userID
	string curName;				  // 目前使用的userName
	string curDir;				  // 目前所在的目录
	BufferManager* bufManager;	  // 缓存控制块管理类
	SuperBlock* spb;			  // 超级块
	UserTable* userTable;		  // 用户表
	File openFileTable[NUM_FILE]; // 打开文件表，由于只有一个进程所以没有进程打开文件表，直接写在了这里
	Inode inodeTable[NUM_INODE];  // 内存Inode表
	Inode* curDirInode;			  // 指向当前目录的Inode指针
	Inode* rootDirInode;		  // 根目录内存Inode
	map<string, int> openFileMap; // 打开文件表的map，用于快速查找,从是文件打开类在openFileTable的位置+1

	// 根据path获取Inode
	Inode* NameI(string path);
	Inode* NameI(string path, bool findDir); // NameI函数重载，只寻找IDIR类型的Inode

	// 判断指定外存Inode是否已经加载到内存中
	int IsLoaded(int inumber);

	// 从外存读取指定外存Inode到内存中
	Inode* IGet(int inumber);

	// 查找pInode是否有给定mode的权限
	int Access(Inode* pInode, unsigned int mode);

	// 分配一个空闲的外存Inode
	Inode* IAlloc();

	// 释放内存Inode
	void IPut(Inode* pNode);

	// 分配空闲打开文件控制块File结构
	File* FAlloc(int& iloc);

	// 将superblock更新到外存中
	void WriteSpb();

	// 将UserTable更新到外存中
	// 固定寻找/etc/userTable.txt文件
	void WriteUserTable();

	// 根据Inode号释放Inode
	void IFree(int number);

	// 获取绝对路径，假设路径正确
	string GetAbsolutionPath(string path);

public:
	enum FileMode
	{
		READ = 0x1, // 读
		WRITE = 0x2 // 写
	};

	FileSystem();
	~FileSystem();

	BufferManager* GetBufferManager();

	// 释放空闲数据盘块
	void Free(int blkno);

	// 分配空闲数据盘块
	Buf* Alloc();

	// 创建文件
	int fcreate(string path);
	// 创建文件夹
	int mkdir(string path);
	// 退出系统
	void exit();
	// 初始化文件系统
	void fformat();
	// 根据path打开文件或文件夹
	int fopen(string path);
	// 根据fd关闭文件
	void fclose(File* fp);
	// 读文件
	void fread(File* fp, char*& buffer, int count);
	// 写文件
	void fwrite(const char* buffer, int count, File* fp);
	// 移动文件指针
	int fseek(File* fp, int offset, int mode);
	// 删除文件
	int fdelete(string path);

	// 文件系统初始化
	void init();
	
	// 终端程序需要用到的命令
	// 目录命令
	void ls();
	void cd(string subname);
	void rmdir(string subname);
	void mkdir_terminal(string subname);
	void ll();

	// 文件操作命令
	void openFile(string path);
	void createFile(string path);
	void removeFile(string path);
	void closeFile(string path);
	void writeFile(string path, int mode);
	void printFile(string path);
	void change_fseek(string path, int offset);
	void flseek(string path);
	void copy_from_win(string path);
	void copy_from_fs(string filename, string winpath, int count);
	void print0penFileList();
	void chmod(string path, string mode);

	// 其它命令
	void help();
	void pwd();
	void format();
	void login();
	void run();
};

#endif