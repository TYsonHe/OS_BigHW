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
		cout << "�޷��򿪴����ļ�myDisk.img" << endl;
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
	//int filoc = fopen("/etc/userTable.txt");
	//File* userTableFile = &this->openFileTable[filoc];
	//this->fwrite(UserTable_to_Char(this->userTable), sizeof(UserTable), userTableFile); // ��Ҫȫ��д��
	//this->fclose(userTableFile);
	this->WriteUserTable();
}

/**************************************************************
* exit �˳��ļ�ϵͳ
* ������
* ����ֵ��
***************************************************************/
void FileSystem::exit() 
{
	this->curId = ROOT_ID; // ��֤Ȩ��
	// ��superblockд�ش���
	this->WriteSpb();
	// ��userTableд�ش��̣�
	this->WriteUserTable();

	// �������ļ����ر�
	for (const auto& pair : this->openFileMap)
	{
		this->fclose(&(this->openFileTable[pair.second - 1]));
	}
	this->openFileMap.clear();

    // ��ȫ�˳�
	// �����е����ݶ�д�ش���,���ӳ�д��Buf
	this->bufManager->Flush();
}

/**************************************************************
* init ��ʼ���ļ�ϵͳ����ȡ�Ѿ����ڵ�myDisk.img
* ������
* ����ֵ��
***************************************************************/
void FileSystem::init()
{
	fstream fd(DISK_PATH, ios::out | ios::in | ios::binary);
	// ���û�д��ļ��������ʾ��Ϣ��throw����
	if (!fd.is_open())
	{
		cout << "�޷��򿪴����ļ�myDisk.img" << endl;
		throw(errno);
	}
	fd.close();

	// �Ի���������ݽ��г�ʼ��
	this->bufManager = new BufferManager();

	// ��ȡ�����飬����������������
	char* ch = new char[sizeof(SuperBlock)];
	Buf* buf = this->bufManager->Bread(POSITION_SUPERBLOCK);
	memcpy(ch, buf->b_addr, SIZE_BLOCK);
	buf = this->bufManager->Bread(POSITION_SUPERBLOCK + 1);
	memcpy(ch + SIZE_BLOCK, buf->b_addr, SIZE_BLOCK);
	this->spb = Char_to_SuperBlock(ch); // ����ɾ��ch

	// ��ȡ��Ŀ¼Inode
	buf = this->bufManager->Bread(POSITION_DISKINODE);
	this->rootDirInode = this->IAlloc();
	this->rootDirInode->ICopy(buf, ROOT_DIR_INUMBER);
	this->curDirInode = this->rootDirInode;
	this->curId = ROOT_ID; // ����ֻ��һ��root�û�
	this->curDir = "/";

	// ��ȡ�û���Ϣ��
	// ����ֱ�ӵ���this->fopen����ΪuserTable����û�г�ʼ��
	Inode* pinode = this->NameI("/etc/userTable.txt");
	// û���ҵ���Ӧ��Inode
	if (pinode == NULL)
	{
		cout << "û���ҵ�/etc/userTable.txt!" << endl;
		throw(ENOENT);
		return;
	}
	// ����ҵ����ж���Ҫ�ҵ��ļ��ǲ����ļ�����
	if (!(pinode->i_mode & Inode::INodeMode::IFILE))
	{
		cout << "����һ����ȷ��/etc/userTable.txt�ļ�!" << endl;
		throw(ENOTDIR);
		return;
	}
	Buf* bp = this->bufManager->Bread(pinode->Bmap(0)); // userTable.txt�ļ�����ֻռһ���̿��С
	this->userTable = Char_to_UserTable(bp->b_addr);
}

/**************************************************************
* run �����ļ�ϵͳ���ն˳���
* ������
* ����ֵ��
***************************************************************/
void FileSystem::run()
{
    this->login();

    cout << "������help�鿴ʹ��ָ��" << endl;
    vector<string> input;
    string strIn;
    bool needEndl = true;
    while (true)
    {
        input.clear();
        if(needEndl)
            cout << endl;
        cout<< this->curName<<"@FileSystem-2153698    " << this->curDir << ">"; // ���ֺ�Linux����̨һ��
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
                // Ŀ¼����
                if (input[0] == "ls") // �鿴��Ŀ¼
                {
                    if (input.size() > 1)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->ls();
                    needEndl = true;
                }
                else if (input[0] == "cd") // ������Ŀ¼
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->cd(input[1]);
                    needEndl = false;

                }
                else if (input[0] == "rmdir") // ɾ����Ŀ¼
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->rmdir(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "mkdir")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->mkdir_terminal(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "ll")
                {
                    if (input.size() < 1 || input.size() > 1)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->ll();
                    needEndl = false;
                }

                // �ļ�����
                else if (input[0] == "touch")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->createFile(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "rm")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->removeFile(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "open")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->openFile(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "close")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->closeFile(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "write")
                {
                    if (input.size() < 2 || input.size() > 3)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    int mode = input.size() == 3 ? atoi(input[2].c_str()) : 0;
                    this->writeFile(input[1], mode);
                }
                else if (input[0] == "cat")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->printFile(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "cpffs")
                {
                    if (input.size() < 4 || input.size() > 4)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->copy_from_fs(input[1], input[2], stoi(input[3]));
                    needEndl = false;
                }
                else if (input[0] == "cpfwin")
                {
                    if (input.size() < 2 || input.size() > 2)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->copy_from_win(input[1]);
                    needEndl = false;
                }
                else if (input[0] == "chmod")
                {
                    if (input.size() < 3 || input.size() > 3)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->chmod(input[1], input[2]);
                }
                else if (input[0] == "lsfs")
                {
                    if (input.size() < 1 || input.size() > 1)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->print0penFileList();
                    needEndl = false;
                }
                else if (input[0] == "fseek")
                {
                    if (input.size() < 3 || input.size() > 3)
                    {
                        cout << "����Ƿ�!" << endl;
                        continue;
                    }
                    this->change_fseek(input[1], stoi(input[2]));
                }
                else if (input[0] == "flseek")
                {
                    if (input.size() != 2)
                    {
                        cout << "����Ƿ�!" << endl;
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
                    cout << "ָ����ڣ���ͨ��help����ָ��Ͱ�����Ϣ";
                }
            }
        }
        catch (int& e)
        {
            cerr << "error code��" << e << endl;
            cout << "���Բ鿴Linux������" << endl;
        }
    }
}