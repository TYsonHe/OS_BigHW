#pragma once
#ifndef HEADER_H
#define HEADER_H

#define _CRT_SECURE_NO_WARNINGS

// ����Ψһ�û�ROOT����Ϣ
#define ROOT_ID 0
#define ROOT_GID 0
#define ROOT_DIR_INUMBER 1

// ������ɫ���
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

// �ļ�������
static const string DISK_PATH = "myDisk.img";
// �ļ��߼����С: 512�ֽ�
static const int SIZE_BLOCK = 512;
// �ļ����С
static const int SIZE_DISK = SIZE_BLOCK * 8192;
// �ļ�����Block����
static const int NUM_BLOCK_ALL = SIZE_DISK / SIZE_BLOCK;

// DiskInode����
static const int NUM_DISKINODE = 256;
// DiskInode�п���ʹ�õ�������������
static const int NUM_I_ADDR = 10;
// DiskInode��С�����ֽ�Ϊ��λ��
static const int SIZE_DISKINODE = 64;
// DiskInode��ʼ��λ�ã���blockΪ��λ��
static const unsigned int POSITION_DISKINODE = 2;
// ÿ��Block��DiskInode������
static const int NUM_INODE_PER_BLOCK = SIZE_BLOCK / SIZE_DISKINODE;

// SuperBlock���ܹ������������Inode����������̿������
static const int NUM_FREE_INODE = 100;
// SuperBlock��ʼ��λ�ã���blockΪ��λ��
static const unsigned int POSITION_SUPERBLOCK = 0;
// SuperBlockһ����������̿������
static const int NUM_FREE_BLOCK_GROUP = 100;
// SuperBlock�����еĴ�С
static const int SIZE_SUPERBLOCK = 816;
// SuperBlock��Ҫ�����ֽ���
static const int SIZE_PADDING = POSITION_DISKINODE * SIZE_BLOCK - SIZE_SUPERBLOCK;

// ���ݿ�Block����
static const int NUM_BLOCK = NUM_BLOCK_ALL - POSITION_DISKINODE - NUM_DISKINODE / NUM_INODE_PER_BLOCK;
// ���ݿ�Block��ʼ��λ�ã���blockΪ��λ��
static const unsigned int POSITION_BLOCK = int(POSITION_DISKINODE + SIZE_DISKINODE * NUM_DISKINODE / SIZE_BLOCK);

// �涨��User�������ռһ��BLOCK����:NUM_USER*(NUM_USER_NAME+NUM_USER_PASSWORD)<=BLOCK_SIZE
// User������û���
static const int NUM_USER = 8;
// User�û����Ƶ���󳤶�
static const unsigned int NUM_USER_NAME = (SIZE_BLOCK / NUM_USER - sizeof(short) * 2) / 2;
// User�û��������󳤶�
static const unsigned int NUM_USER_PASSWORD = (SIZE_BLOCK / NUM_USER - sizeof(short) * 2) / 2;
// User��ʼ��λ�ã���blockΪ��λ��
static const unsigned int POSITION_USER = POSITION_BLOCK + 1;

// BufferManager������ƿ顢������������
static const int NUM_BUF = 15;
// BufferManager��������С�� ���ֽ�Ϊ��λ
static const int SIZE_BUFFER = SIZE_BLOCK;

// �涨����Ŀ¼���������ĵ�һ��Block��
// Directory��һ��Ŀ¼����Ŀ¼�ļ�����󳤶�
static const int NUM_FILE_NAME = 28;
// Directory��һ��Ŀ¼�������Ŀ¼����
static const int NUM_SUB_DIR = SIZE_BLOCK / (NUM_FILE_NAME + sizeof(int));
// Directory��ʼ��λ�ã���blockΪ��λ��
static const unsigned int POSITION_DIRECTORY = POSITION_BLOCK;

// FileSystem:OpenFileTable�������ļ���
static const int NUM_FILE = 100;
// FileSystem:IndoeTable�����Inode��
static const int NUM_INODE = 100;

// i_addr�������
// �����ļ�ʹ�õ������̿�����
static const int NUM_FILE_INDEX = SIZE_BLOCK / sizeof(int);
static const int NUM_BLOCK_IFILE = 5; // ���ļ��߼�������ʼ�ĺţ�����i_addr[0]-[4]Ϊֱ��������i_addr[5-10]Ϊ�������
static const int NUM_BLOCK_ILARG = NUM_FILE_INDEX * (NUM_I_ADDR - NUM_BLOCK_IFILE) + NUM_BLOCK_IFILE;

/**************************************************************
* Buf �����Buffer�Ķ��� ��Ҳ�л�����ƿ飩
* ��;����Ϊ�����洢��Ϣ
* ���⣺�ж�Ӧ��Flag��¼�����ʹ����Ϣ
*		ֻ��һ���豸
***************************************************************/
class Buf
{
public:
	enum BufFlag
	{
		B_NONE = 0x0,	// ��ʼ����δ����ʹ��ʱ��
		B_WRITE = 0x1,	// д������ǡ�
		B_READ = 0x2,	// ��������ǡ�
		B_DONE = 0x4,	// I/O��������
		B_DELWRI = 0x80 // �ӳ�д���
	};

public:
	unsigned int b_blkno;  // ������Ӧ���ڴ��߼����
	char* b_addr;		   // ָ��û������׵�ַ
	unsigned int b_flags;  // ������ƿ��־λ,�����enum BufFlag
	Buf* b_forw;		   // ��ǰ������ǰ���ڵ�,��Buf����NODEV���л�ĳһ�豸����
	Buf* b_back;		   // ��ǰ�����ĺ�̽ڵ�,ͬ��
	Buf* av_forw;		   // ��һ�����л�����ָ��,��Buf�������ɶ��л�ĳһI/O�������
	Buf* av_back;		   // ��һ�����л�����ָ��,ͬ��
	unsigned int b_wcount; // �贫�͵��ֽ���

	Buf();
	~Buf();
};

/**************************************************************
* BufferManager ����أ�������ƿ�����ඨ��
* ��;���������е�Buf
* ���⣺�ж�Ӧ��Flag��¼�����ʹ����Ϣ
*		ֻ��һ���豸
***************************************************************/
class BufferManager
{
private:
	Buf bFreeList;					   // ���ɻ�����У�˫������
	Buf devtab;						   // ����ֻ��һ���豸������ֻ��һ���豸��
	Buf m_Buf[NUM_BUF];				   // ������ƿ�����
	char Buffer[NUM_BUF][SIZE_BUFFER]; // ����������
public:
	// ���캯��
	BufferManager();
	~BufferManager();

	// ���������豸��Ŷ�ȡ����
	Buf* GetBlk(int blkno);

	// �������bpд��������
	void Bwrite(Buf* bp);

	// ��������ӳ�д
	void Bdwrite(Buf* bp);

	// ���������豸��Ŷ�ȡ����
	Buf* Bread(int blkno);

	// ����Buf
	void CleanBuf(Buf* bp);

	// ȫ�����棬д�����
	void Flush();
};

/**************************************************************
* Directory Ŀ¼��Ϣ��
* ��;����Ϊ�ļ�ϵͳ��Ŀ¼��Ϣ��¼
* ���⣺ʵ������״Ŀ¼�ṹ
*		һ��Directory���һ��BLOCK��С
***************************************************************/
class Directory
{
public:
	int d_inodenumber[NUM_SUB_DIR];				 // ��Ŀ¼Inode��
	char d_filename[NUM_SUB_DIR][NUM_FILE_NAME]; // ��Ŀ¼�ļ���

	Directory();
	~Directory();

	// ����Ŀ¼��name��Inode��inumber����ǰĿ¼����һ����Ŀ¼
	int mkdir(const char* name, const int inumber);

	// ���Ŀ¼�еĵ�iloc����Ŀ¼
	void rmi(int iloc);
};

/**************************************************************
* SuperBlock ������
* ��;����Ϊ�ļ�ϵͳ�洢��Դ�����
* ���⣺��Ҫ�����ļ�ϵͳ�������Դ��Ϣ
*		Inode��
*		�����̿�
*		�������Inode
***************************************************************/
class SuperBlock //1024�ֽ� ռ2���������飩
{
public:
	unsigned int s_isize;			  // Inode���ߴ磬ռ�õ����ݿ�����
	unsigned int s_fsize;			  // �������ݿ�����
	int s_ninode;					  // ֱ�ӹ���Ŀ������Inode����
	int s_inode[NUM_FREE_INODE];	  // ֱ�ӹ���Ŀ������Inode������
	int s_nfree;					  // ֱ�ӹ���Ŀ����̿�����
	int s_free[NUM_FREE_BLOCK_GROUP]; // ֱ�ӹ���Ŀ����̿�������
	char padding[SIZE_PADDING];		  // ����ֽ�

	// ����&����
	SuperBlock();
	~SuperBlock();

	// ��ʼ��SuperBlock
	void Init();
};

/**************************************************************
* DiskINode ��������ڵ�
* ��;�����Inodeλ���ļ��洢�豸�ϵ����Inode��
		ÿ���ļ���Ψһ��Ӧ�����Inode
* ���⣺��¼�˸��ļ���Ӧ�Ŀ�����Ϣ��
*		���INode���󳤶�Ϊ64�ֽ�
*		ÿ�����̿���Դ��512/64 = 8�����Inode
***************************************************************/
class DiskInode // 64�ֽ�
{
public:
	unsigned int d_mode;	// �ļ����ͺͷ��ʿ���λ
	int d_nlink;			// �ļ�Ӳ��������������ļ���Ŀ¼���в�ͬ·����������
	short d_uid;			// �ļ������ߵ��û�id
	short d_gid;			// �ļ������ߵ���id
	int d_size;				// �ļ���С���ֽ�Ϊ��λ
	int d_addr[NUM_I_ADDR]; // ��ַӳ����Ǽ��߼���������֮���ӳ���ϵ
	unsigned int d_atime;	// ������ʱ��
	unsigned int d_mtime;	// ����޸�ʱ��
};

/**************************************************************
* Inode �ڴ������ڵ�
* ��;��Ϊ�ļ�ϵͳ�е����е���Դ�����ڴ������
* ���⣺��¼����Դ��Ϣ
*		ÿһ���򿪵��ļ�����ǰ����Ŀ¼�����ص����ļ�ϵͳ
*		ֻ��һ���豸������Ҫi_number��ȷ��λ��
***************************************************************/
class Inode // 64�ֽ�
{
public:
	// i_mode�б�־λ
	enum INodeMode
	{
		IDIR = 0x4000,	// �ļ����ͣ�Ŀ¼�ļ�
		IFILE = 0x2000, // �ļ����ͣ��ļ�
		ILARG = 0x1000, // �ļ����ͣ����ļ�,ILARG��IFILEһ�����
		OWNER_R = 0x200,
		OWNER_W = 0x100,
		GROUP_R = 0x20,
		GROUP_W = 0x10,
		OTHER_R = 0x2,
		OTHER_W = 0x1,
	};

public:
	short i_uid;			// �ļ������ߵ��û�id
	short i_gid;			// �ļ������ߵ���id
	unsigned short i_mode;	// �ļ�������ʽ��Ϣ
	short i_nlink;			// �ļ�Ӳ��������������ļ���Ŀ¼���в�ͬ·����������
	int i_size;				// �ļ���С���ֽ�Ϊ��λ
	int i_addr[NUM_I_ADDR]; // ָ�����ݿ�������ַӳ��������ļ��߼���ź�������ת���Ļ���������
	unsigned int i_atime;	// ������ʱ��
	unsigned int i_mtime;	// ����޸�ʱ��
	short i_count;			// ���ü���
	short i_number;			// ��inode���еı��,���ڴ�Inodeת��Ϊ���Inode

	Inode();
	~Inode();

	// ���߼����lbnӳ�䵽�����̿��phyBlkno
	int Bmap(int lbn);

	// ���ݻ�������bp�����Inode��ȡ���ݵ��ڴ�Inode
	void ICopy(Buf* bp, int inumber);

	// ���ݹ�����ڴ�Inode�����ļ�Ȩ��
	int AssignMode(unsigned short mode);

	// ���Inode����
	void Clean();

	// ���ڴ�Inode���µ������
	void WriteI();

	// ��ȡ��Ŀ¼inodenumber
	int GetParentInumber();

	// ��ȡĿ¼����
	Directory* GetDir();

	// �ͷ������̿�
	void ITrunc();

	// ����id��gid��ȡ�ļ�Ȩ���ַ���
	string GetModeString(int id, int gid);

	// ͨ���ַ�����ȡmode
	unsigned short String_to_Mode(string mode);
};

/**************************************************************
* File ���ļ����ƿ�
* ��;����¼�ļ�ϵͳ���ļ�����Ϣ
* ���⣺�ļ�ʹ���˵�ids���ļ���дλ�õȵ�
***************************************************************/
class File
{
public:
	Inode* f_inode;		   // ָ����ļ����ڴ�Inodeָ��
	unsigned int f_offset; // �ļ���дλ��ָ��
	short f_uid;		   // �ļ������ߵ��û�id
	short f_gid;		   // �ļ������ߵ���id

	File();
	~File();

	void Clean();
};

/**************************************************************
* UserTable �û���
* ��;���ļ�ϵͳ���û�����
*		��¼�û���������
* ���⣺ֻ��һ��root�û�
***************************************************************/
class UserTable
{
public:
	short u_id[NUM_USER];						  // �û�id
	short u_gid[NUM_USER];						  // �û�������id
	char u_name[NUM_USER][NUM_USER_NAME];		  // �û���
	char u_password[NUM_USER][NUM_USER_PASSWORD]; // �û�����

	// ������
	UserTable();
	~UserTable();

	// ���root�û�
	void AddRoot();

	// ������û�Id
	short GetGId(const short id);

	// �����û�����¼����
	short FindUser(const char* name, const char* password);
};

/**************************************************************
* FileSystem �ļ�ϵͳ����ģ��
* ��;������ļ�ϵͳ������������
*		��¼���ն˵���Ϣ
*		����Ĺ���ͨ�� bufManager������
*		�����ļ�ϵͳ��Դ�Ĺ���ͨ��SuperBlock������
*		�ڴ�ͨ��Inode��������
*		�ļ�ͨ�����ļ������
*		root�û����û���
* ���⣺ֻ��һ���豸��һ������
***************************************************************/
class FileSystem
{
private:
	short curId;				  // Ŀǰʹ�õ�userID
	string curName;				  // Ŀǰʹ�õ�userName
	string curDir;				  // Ŀǰ���ڵ�Ŀ¼
	BufferManager* bufManager;	  // ������ƿ������
	SuperBlock* spb;			  // ������
	UserTable* userTable;		  // �û���
	File openFileTable[NUM_FILE]; // ���ļ�������ֻ��һ����������û�н��̴��ļ���ֱ��д��������
	Inode inodeTable[NUM_INODE];  // �ڴ�Inode��
	Inode* curDirInode;			  // ָ��ǰĿ¼��Inodeָ��
	Inode* rootDirInode;		  // ��Ŀ¼�ڴ�Inode
	map<string, int> openFileMap; // ���ļ����map�����ڿ��ٲ���,�����ļ�������openFileTable��λ��+1

	// ����path��ȡInode
	Inode* NameI(string path);
	Inode* NameI(string path, bool findDir); // NameI�������أ�ֻѰ��IDIR���͵�Inode

	// �ж�ָ�����Inode�Ƿ��Ѿ����ص��ڴ���
	int IsLoaded(int inumber);

	// ������ȡָ�����Inode���ڴ���
	Inode* IGet(int inumber);

	// ����pInode�Ƿ��и���mode��Ȩ��
	int Access(Inode* pInode, unsigned int mode);

	// ����һ�����е����Inode
	Inode* IAlloc();

	// �ͷ��ڴ�Inode
	void IPut(Inode* pNode);

	// ������д��ļ����ƿ�File�ṹ
	File* FAlloc(int& iloc);

	// ��superblock���µ������
	void WriteSpb();

	// ��UserTable���µ������
	// �̶�Ѱ��/etc/userTable.txt�ļ�
	void WriteUserTable();

	// ����Inode���ͷ�Inode
	void IFree(int number);

	// ��ȡ����·��������·����ȷ
	string GetAbsolutionPath(string path);

public:
	enum FileMode
	{
		READ = 0x1, // ��
		WRITE = 0x2 // д
	};

	FileSystem();
	~FileSystem();

	BufferManager* GetBufferManager();

	// �ͷſ��������̿�
	void Free(int blkno);

	// ������������̿�
	Buf* Alloc();

	// �����ļ�
	int fcreate(string path);
	// �����ļ���
	int mkdir(string path);
	// �˳�ϵͳ
	void exit();
	// ��ʼ���ļ�ϵͳ
	void fformat();
	// ����path���ļ����ļ���
	int fopen(string path);
	// ����fd�ر��ļ�
	void fclose(File* fp);
	// ���ļ�
	void fread(File* fp, char*& buffer, int count);
	// д�ļ�
	void fwrite(const char* buffer, int count, File* fp);
	// �ƶ��ļ�ָ��
	int fseek(File* fp, int offset, int mode);
	// ɾ���ļ�
	int fdelete(string path);

	// �ļ�ϵͳ��ʼ��
	void init();
	
	// �ն˳�����Ҫ�õ�������
	// Ŀ¼����
	void ls();
	void cd(string subname);
	void rmdir(string subname);
	void mkdir_terminal(string subname);
	void ll();

	// �ļ���������
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

	// ��������
	void help();
	void pwd();
	void format();
	void login();
	void run();
};

#endif