#include "header.h"
#include "utils.h"


/**************************************************************
* fformat��ʼ���ļ�ϵͳ
* ������
* ����ֵ��
***************************************************************/
void FileSystem::fformat() {
	fstream fd(DISK_PATH, ios::out);
	fd.close();
	fd.open(DISK_PATH, ios::out | ios::in | ios::binary);
	// ���û�д��ļ��������ʾ��Ϣ��throw����
	if (!fd.is_open())
	{
		cout << "�޷���һ���ļ�myDisk.img" << endl;
		throw(errno);
	}
	fd.close();

	// �ȶ��û����г�ʼ����ֻ��Root�û�
	this->userTable = new UserTable();
	this->userTable->AddRoot(); // ���root�û�
	this->curId = ROOT_ID;
	this->curName = "root";

	// ��ʼ������
	this->bufManager = new BufferManager();
	this->spb = new SuperBlock();
	
	// ��ʽ��ʼ��SuperBlock
	this->spb->Init();
	// ��superblockд�ش���
	this->WriteSpb();

	// ���ڶ�Ŀ¼���г�ʼ��
	// ����һ�����е����Inode��������Ŀ¼
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

	// ������Ŀ¼
	Directory rootDir; // ����һ�������̿��Ÿ�Ŀ¼����
	rootDir.mkdir(".", this->rootDirInode->i_number);  // �����Լ�
	rootDir.mkdir("..", this->rootDirInode->i_number); // �������ף���Ŀ¼�ĸ���Ҳ���Լ�
	this->curDir = "/";

	// ����Ŀ¼���������̿��
	Buf* newBuf = this->Alloc();
	memcpy(newBuf->b_addr, Directory_to_Char(&rootDir), sizeof(Directory));
	
	// ����д�ش�����������
	this->bufManager->Bwrite(newBuf);
	// ����Ŀ¼��Inodeд��������
	this->rootDirInode->i_size = sizeof(Directory) / NUM_SUB_DIR * 2;
	this->rootDirInode->i_addr[0] = newBuf->b_blkno;

	// ����Ҫ�����Ŀ¼
	this->mkdir("/bin");
	this->mkdir("/etc");
	this->mkdir("/home");
	this->mkdir("/dev");
	// ��rootInodeд�ش�����
	this->rootDirInode->WriteI();

	// ������д���û���
	// ����������ʱֻ��һ��root�û�
	this->fcreate("/etc/userTable.txt");
	int filoc = fopen("/etc/userTable.txt");
	File* userTableFile = &this->openFileTable[filoc];
	this->fwrite(userTable2Char(this->userTable), sizeof(UserTable), userTableFile); // ��Ҫȫ��д��
	this->fclose(userTableFile);
}

void FileSystem::exit() {

}

void FileSystem::init() {

}

void FileSystem::run() {

}