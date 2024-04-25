#include "header.h"
#include "utils.h"

// FileSystem��Ĺ��캯��
FileSystem::FileSystem()
{
	// ��ʱ����Ҫ����
}

// FileSystem����������
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
* GetAbsolutionPath ��ȡ����·��������·����ȷ
* ������path ���·�������·��
* ����ֵ��string ����·��
***************************************************************/
string FileSystem::GetAbsolutionPath(string path)
{
	if (path[0] == '/')
		return path;
	else
		return this->curDir + path;
}

/**************************************************************
* GetBufferManager �õ��ļ�ϵͳ�Ļ��������
* ������
* ����ֵ��BufferManager* ָ��
***************************************************************/
BufferManager* FileSystem::GetBufferManager()
{
    return this->bufManager;
}

/**************************************************************
* WriteSpb��SuperBlockд�ش���
* ������
* ����ֵ��
***************************************************************/
void FileSystem::WriteSpb()
{
	char* p = SuperBlk_to_Char(this->spb);
	Buf* bp = this->bufManager->Bread(POSITION_SUPERBLOCK);
	memcpy(bp->b_addr, p, SIZE_BUFFER);
	this->bufManager->Bwrite(bp);
	bp = this->bufManager->Bread(POSITION_SUPERBLOCK + 1);
	// ����֮ǰ���ڴ�й©,û�����Ļ�����һֱ��������
	memcpy(bp->b_addr, p + SIZE_BLOCK, SIZE_BLOCK);
	this->bufManager->Bwrite(bp);
}

/**************************************************************
* WriteUserTable ���û���д�ش���
* ������
* ����ֵ��
***************************************************************/
void FileSystem::WriteUserTable()
{
	int filoc = fopen("/etc/userTable.txt");
	File* userTableFile = &this->openFileTable[filoc];
	this->fwrite(UserTable_to_Char(this->userTable), sizeof(UserTable), userTableFile);
	this->fclose(userTableFile);
}

/**************************************************************
* IAlloc ����һ�����е����Inode
* ������
* ����ֵ��Inode* ���ط��䵽���ڴ�Inode���������ʧ�ܣ�����NULL
***************************************************************/
Inode* FileSystem::IAlloc()
{
    Buf* pBuf;
    Inode* pNode;
    int ino = 0; // ���䵽�Ŀ������Inode���

    // SuperBlockֱ�ӹ���Ŀ���Inode�������ѿ�
    // ע���µĿ���Inode������
    if (this->spb->s_ninode <= 0)
    {
        // ���ζ������Inode���еĴ��̿飬�������п������Inode���������Inode������
        for (unsigned int i = 0; i < this->spb->s_isize; i++)
        {
            pBuf = this->bufManager->Bread(POSITION_DISKINODE + i / NUM_INODE_PER_BLOCK);

            // ��ȡ��������ַ
            int* p = (int*)pBuf->b_addr;

            // ���û�������ÿ�����Inode��i_mode != 0����ʾ�Ѿ���ռ��
            for (int j = 0; j < NUM_INODE_PER_BLOCK; j++)
            {
                ino++;
                int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

                // �����Inode�ѱ�ռ�ã����ܼ������Inode������
                if (mode != 0)
                {
                    continue;
                }

                /*
                 * ������inode��i_mode==0����ʱ������ȷ��
                 * ��inode�ǿ��еģ���Ϊ�п������ڴ�inodeû��д��
                 * ������,����Ҫ���������ڴ�inode���Ƿ�����Ӧ����
                 */
                if (this->IsLoaded(ino) == -1)
                {
                    // �����Inodeû�ж�Ӧ���ڴ濽��������������Inode������
                    this->spb->s_inode[this->spb->s_ninode++] = ino;

                    // ��������������Ѿ�װ�����򲻼�������
                    if (this->spb->s_ninode >= 100)
                        break;
                }
            }

            // ��������������Ѿ�װ�����򲻼�������
            if (this->spb->s_ninode >= 100)
                break;
        }
    }

    // ��������˻�û�п������Inode������NULL
    if (this->spb->s_ninode <= 0)
    {
        cout << "���������Inode������!" << endl;
        throw(ENOSPC);
        return NULL;
    }

    // ���ڴ��������ڴ�Inode
    int inumber = this->spb->s_inode[--this->spb->s_ninode];
    pNode = IGet(inumber);
    if (NULL == pNode) // �����޸Ĳ���
        return NULL;

    if (0 == pNode->i_mode)
    {
        pNode->Clean();
        return pNode;
    }
    return pNode;
}

/**************************************************************
* Free �ͷ�ָ���������̿�
* ������blkno Ҫ�ͷŵ������̿��
* ����ֵ��
***************************************************************/
void FileSystem::Free(int blkno)
{
	Buf* pBuf;
	if (blkno < POSITION_BLOCK)
	{
		cout << "�����ͷ�ϵͳ�̿�" << endl;
		return;
	}

	// �����ǰϵͳ���Ѿ�û�п����̿飬�����ͷŵ���ϵͳ�е�1������̿�
	if (this->spb->s_nfree <= 0)
	{
		this->spb->s_nfree = 1;
		this->spb->s_free[0] = 0;
	}

	// ���SuperBlock��ֱ�ӹ�����д��̿�ŵ�ջ����
	if (this->spb->s_nfree >= NUM_FREE_BLOCK_GROUP)
	{
		// ����һ���»���飬���ڴ���µĿ��д��̿��
		pBuf = this->bufManager->GetBlk(blkno);

		// ��s_nfree��s_free[100]д������̿��ǰ101���ֽ�
		// s_free[0]=���յ��̿��
		// s_nfree=1
		int* stack = new int[NUM_FREE_BLOCK_GROUP + 1] {0};		// ��һλ�����ӵ���һ����̿����
		stack[0] = this->spb->s_nfree;                          // ��һλ�����ӵ���һ����̿����
		for (int i = 0; i < NUM_FREE_BLOCK_GROUP; i++)
			stack[i + 1] = this->spb->s_free[i];
		memcpy(pBuf->b_addr, IntArray_to_Char(stack, NUM_FREE_BLOCK_GROUP + 1), sizeof(int) * NUM_FREE_BLOCK_GROUP + 1);
		bufManager->Bwrite(pBuf);

		this->spb->s_free[0] = blkno;
		this->spb->s_nfree = 1;
	}

	// �ͷ������̿��
	this->spb->s_free[this->spb->s_nfree++] = blkno;
}

/**************************************************************
* IsLoaded �ж�ָ�����Inode�Ƿ��Ѿ����ص��ڴ���
* ������inumber ���Inode���
* ����ֵ������Ѿ����أ������ڴ�Inode��inodeTable�ı�ţ����򷵻�-1
***************************************************************/
int FileSystem::IsLoaded(int inumber)
{
    // Ѱ��ָ�����Inode���ڴ�inode����
    for (int i = 0; i < NUM_INODE; i++)
        if (this->inodeTable[i].i_count != 0 && this->inodeTable[i].i_number == inumber)
            return i;

    return -1;
}

/**************************************************************
* IGet ������ȡָ�����Inode���ڴ���
* ������inumber ���Inode���
* ����ֵ��Inode* �ڴ�Inode����
***************************************************************/
Inode* FileSystem::IGet(int inumber)
{
    Inode* pInode = NULL;
    // ��inodeTable�в���ָ�����Inode���ڴ�inode����
    int isInTable = this->IsLoaded(inumber);

    // ����ҵ��ˣ�ֱ�ӷ����ڴ�inode���������ü�����1
    if (isInTable != -1)
    {
        pInode = &this->inodeTable[isInTable];
        pInode->i_count++;
        pInode->i_atime = unsigned int(time(NULL));
        return pInode;
    }

    // û���ҵ����ȴ��ڴ�Inode�ڵ���з���һ��Inode,�ٴ�����ȡ
    for (int i = 0; i < NUM_INODE; i++)
        // ������ڴ�Inode���ü���Ϊ�㣬���Inode��ʾ���У�����ʹ��
        if (this->inodeTable[i].i_count == 0)
        {
            pInode = &(this->inodeTable[i]);
            break;
        }

    // ����ڴ�InodeTable�������׳��쳣
    if (pInode == NULL)
    {
        cout << "�ڴ�InodeTable����" << endl;
        throw(ENFILE);
        return pInode;
    }

    // ����ڴ�InodeTablû����������ȡָ�����Inode���ڴ���
    pInode->i_number = inumber;
    pInode->i_count++;
    pInode->i_atime = unsigned int(time(NULL));

    // �������Inode���뻺����
    Buf* pBuf = this->bufManager->Bread(POSITION_DISKINODE + (inumber - 1) / NUM_INODE_PER_BLOCK);
    // ���������е����Inode��Ϣ�������·�����ڴ�Inode��
    pInode->ICopy(pBuf, inumber);
    return pInode;
}

/**************************************************************
* IPut �ͷ�InodeTable�е�Inode�ڵ�
* ������pNode �ڴ�Inodeָ��
* ����ֵ��
***************************************************************/
void FileSystem::IPut(Inode* pNode)
{
	// ��ǰ����Ϊ���ø��ڴ�Inode��Ψһ���̣���׼���ͷŸ��ڴ�Inode
	if (pNode->i_count == 1)
	{
		pNode->i_atime = unsigned int(time(NULL));
		pNode->WriteI();

		// ���ļ��Ѿ�û��Ŀ¼·��ָ����
		if (pNode->i_nlink <= 0)
		{
			// �ͷŸ��ļ�ռ�ݵ������̿�
			pNode->ITrunc();
			pNode->i_mode = 0;
			// �ͷŶ�Ӧ�����Inode
			this->IFree(pNode->i_number);
		}
		pNode->Clean();
	}

	// �����ڴ�Inode�����ü���
	pNode->i_count--;
}

/**************************************************************
* IFree �ͷ����Inode�ڵ�
* ������number ���Inode���
* ����ֵ��
***************************************************************/
void FileSystem::IFree(int number)
{
	// spb����
	if (this->spb->s_ninode >= NUM_FREE_INODE)
		return;

	// �������Inode�ı�ż������Inode�����������Ļ�ֱ�Ӹ��ǵ���������
	this->spb->s_inode[this->spb->s_ninode++] = number;
}

/**************************************************************
* NameI �����ļ�·�����Ҷ�Ӧ��Inode
* ������path �ļ�·��
* ����ֵ��Inode* ���ض�Ӧ��Inode�����û���ҵ�������NULL
***************************************************************/
Inode* FileSystem::NameI(string path)
{
	Inode* pInode;
	Buf* pbuf;
	vector<string> paths = stringSplit(path, '/'); // ����Ҫ���ļ��к��ļ������в��ܳ���"/"
	int ipaths = 0;
	bool isFind = false;

	// ��һ���ַ�Ϊ/��ʾ����·��
	if (path.size() != 0 && path[0] == '/') // �Ӹ�Ŀ¼��ʼ����
		pInode = this->rootDirInode;
	else // ���·���Ĳ���
		pInode = this->curDirInode;
	int blkno = pInode->Bmap(0);
	// ��ȡ���̵�����

	while (true)
	{
		isFind = false;
		// ����pathΪ�յ����
		if (ipaths == paths.size()) // �������˵���ҵ��˶�Ӧ���ļ���Ŀ¼
			break;
		else if (ipaths >= paths.size())
			return NULL;

		// ������е�Inode��Ŀ¼�ļ�����ȷ,��Ϊ���������ѭ���Ż��ҵ��ļ�/Ŀ¼
		// һ���ҵ��ļ�/Ŀ¼����������ѭ��
		if (pInode->i_mode & Inode::INodeMode::IDIR)
		{
			// ����Ҫ���������̿��
			// ����Ŀ¼�ļ�ֻռһ���̿飬����ֻ��һ�Ϊ��
			int blkno = pInode->Bmap(0);
			// ��ȡ���̵�����
			pbuf = this->bufManager->Bread(blkno);

			// ������תΪĿ¼�ṹ
			Directory* fatherDir = Char_to_Directory(pbuf->b_addr);

			// ѭ������Ŀ¼�е�ÿ��Ԫ��
			for (int i = 0; i < NUM_SUB_DIR; i++)
			{
				// ����ҵ���Ӧ��Ŀ¼
				if (paths[ipaths] == fatherDir->d_filename[i])
				{
					ipaths++;
					isFind = true;
					pInode = this->IGet(fatherDir->d_inodenumber[i]);
					break;
				}
			}

			// ���û���ҵ���Ӧ���ļ���Ŀ¼
			if (!isFind)
				return NULL;
		}
		else // ����Ŀ¼�ļ��Ǵ����
			return NULL;
	}

	// ���������˵���ҵ��˶�Ӧ���ļ�����Ŀ¼
	return pInode;
}

/**************************************************************
* Alloc ������л�����
* ������
* ����ֵ��Buf* ���ط��䵽�Ļ��������������ʧ�ܣ�����NULL
***************************************************************/
Buf* FileSystem::Alloc()
{
    int blkno; // ���䵽�Ŀ��д��̿���
    Buf* pBuf;

    // ��������ջ������ȡ���д��̿���
    blkno = this->spb->s_free[--this->spb->s_nfree];

    // �ѷ��価���еĿ��д��̿飬ֱ�ӷ���
    if (0 == blkno)
    {
        this->spb->s_nfree = 0;
        cout << "��������!û�п����̿�" << endl;
        throw(ENOSPC);
        return NULL;
    }

    // ���д��̿��������ѿգ���һ����д��̿�ı�Ŷ���SuperBlock��s_free
    if (this->spb->s_nfree <= 0)
    {
        // ����ÿ��д��̿�
        pBuf = this->bufManager->Bread(blkno);

        int* p = (int*)pBuf->b_addr;

        // ���ȶ��������̿���s_nfre
        this->spb->s_nfree = (int)p[0];

        // ���ݿ����̿�����ȡ�����̿�������
        for (int i = 0; i < this->spb->s_nfree; i++)
            this->spb->s_free[i] = (int)p[i + 1];
    }

    // �����Ļ�����һ���д��̿飬���ظô��̿�Ļ���ָ��
    pBuf = this->bufManager->GetBlk(blkno); // Ϊ�ô��̿����뻺��
    this->bufManager->CleanBuf(pBuf);         // ��ջ����е�����

    return pBuf;
}

/**************************************************************
* FAlloc ������д��ļ����ƿ�File�ṹ
* ������
* ����ֵ��File* ���ط��䵽�Ĵ��ļ����ƿ�File�ṹ���������ʧ�ܣ�����NULL
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
* mkdir �����ļ���
* ������path �ļ���·��
* ����ֵ��int �����ɹ�Ϊ0������Ϊ-1
***************************************************************/
int FileSystem::mkdir(string path)
{
	vector<string> paths = stringSplit(path, '/');
	if (paths.size() == 0)
	{
		cout << "·����Ч!" << endl;
		throw(EINVAL);
		return -1;
	}

	string name = paths[paths.size() - 1];
	if (name.size() > NUM_FILE_NAME)
	{
		cout << "�½�Ŀ¼������!" << endl;
		throw(ENAMETOOLONG);
		return -1;
	}
	else if (name.size() == 0)
	{
		cout << "�½�Ŀ¼������Ϊ��!" << endl;
		throw(EINVAL);
		return -1;
	}

	Inode* fatherInode;

	// ��·����ɾ���ļ�����
	path.erase(path.size() - name.size(), name.size());
	// �ҵ���Ҫ�������ļ������ĸ��ļ�����Ӧ��Inode
	fatherInode = this->NameI(path);

	// û���ҵ���Ӧ��Inode
	if (fatherInode == NULL)
	{
		cout << "û���ҵ���Ӧ���ļ���Ŀ¼!" << endl;
		throw(ENOENT);
		return -1;
	}

	// ����ҵ����жϴ������ļ��еĸ��ļ����ǲ����ļ�������
	if (!(fatherInode->i_mode & Inode::INodeMode::IDIR))
	{
		cout << "����һ����ȷ��Ŀ¼��!" << endl;
		throw(ENOTDIR);
		return -1;
	}

	if (this->Access(fatherInode, FileMode::WRITE) == 0) // �ж��Ƿ���Ȩ��д�ļ�
	{
		cout << "û��Ȩ��д�ļ�!" << endl;
		throw(EACCES);
		return -1;
	}

	bool isFull = true;
	// ����Ȩ��д�ļ�ʱ���ж��Ƿ��������ļ����Ҳ鿴�Ƿ��п��е���Ŀ¼����д
	// ����Ҫ���������̿��
	// ����Ŀ¼�ļ�ֻռһ���̿飬����ֻ��һ�Ϊ��
	int blkno = fatherInode->Bmap(0);
	// ��ȡ���̵�����
	Buf* fatherBuf = this->bufManager->Bread(blkno);
	// ������תΪĿ¼�ṹ
	Directory fatherDir = *Char_to_Directory(fatherBuf->b_addr);
	// ѭ������Ŀ¼�е�ÿ��Ԫ��
	for (int i = 0; i < NUM_SUB_DIR; i++)
	{
		// ����ҵ���Ӧ��Ŀ¼
		if (name == fatherDir.d_filename[i])
		{
			cout << "�ļ��Ѵ���!" << endl;
			throw(EEXIST);
			return -1;
		}
		if (isFull && fatherDir.d_inodenumber[i] == 0)
		{
			isFull = false;
		}
	}

	// ���Ŀ¼����
	if (isFull)
	{
		cout << "Ŀ¼����!" << endl;
		throw(ENOSPC);
		return -1;
	}

	// ��ſ�ʼ�����µ��ļ���
	// ����һ���µ��ڴ�Inode
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
	// �����ļ����������Ŀ¼��
	Directory newDir;
	newDir.mkdir(".", newinode->i_number);	   // �����Լ�
	newDir.mkdir("..", fatherInode->i_number); // ��������
	// �����ļ��з��������̿��
	Buf* newBuf = this->Alloc();
	memcpy(newBuf->b_addr, Directory_to_Char(&newDir), sizeof(Directory));
	newinode->i_size = sizeof(Directory) / NUM_SUB_DIR * 2; // ���ļ��д�С������Ŀ¼��
	newinode->i_addr[0] = newBuf->b_blkno;

	// �����ļ���д���丸�׵�Ŀ¼����
	fatherDir.mkdir(name.c_str(), newinode->i_number);
	fatherInode->i_size += sizeof(Directory) / NUM_SUB_DIR; // ���׵Ĵ�С����һ��Ŀ¼��
	fatherInode->i_atime = unsigned int(time(NULL));
	fatherInode->i_mtime = unsigned int(time(NULL));
	memcpy(fatherBuf->b_addr, Directory_to_Char(&fatherDir), sizeof(Directory));

	// ͳһд�أ���Ŀ¼inode����Ŀ¼inode����Ŀ¼���ݿ顢��Ŀ¼���ݿ�
	fatherInode->WriteI();
	newinode->WriteI();

	this->bufManager->Bwrite(fatherBuf);
	this->bufManager->Bwrite(newBuf);
	// ��FS���rootInode��curInode���ͷ�����Inode
	if (fatherInode != this->rootDirInode && fatherInode != this->curDirInode)
		this->IPut(fatherInode);
	this->IPut(newinode);

	return 0;
}

/**************************************************************
* fcreate �����ļ�
* ������path �ļ�·��
* ����ֵ��int �����ɹ�Ϊ0������Ϊ-1
***************************************************************/
int FileSystem::fcreate(string path)
{
	vector<string> paths = stringSplit(path, '/');
	if (paths.size() == 0)
	{
		cout << "·����Ч!" << endl;
		throw(EINVAL);
		return -1;
	}
	string name = paths[paths.size() - 1];
	if (name.size() > NUM_FILE_NAME)
	{
		cout << "�ļ�������!" << endl;
		throw(ENAMETOOLONG);
		return -1;
	}
	else if (name.size() == 0)
	{
		cout << "�ļ�������Ϊ��!" << endl;
		throw(EINVAL);
		return -1;
	}

	Inode* fatherInode;

	// ��·����ɾ���ļ���
	path.erase(path.size() - name.size(), name.size());
	// �ҵ���Ҫ�������ļ��ĸ��ļ�����Ӧ��Inode
	fatherInode = this->NameI(path);

	// û���ҵ���Ӧ��Inode
	if (fatherInode == NULL)
	{
		cout << "û���ҵ���Ӧ���ļ���Ŀ¼!" << endl;
		throw(ENOENT);
		return -1;
	}

	// ����ҵ����жϴ������ļ��ĸ��ļ����ǲ����ļ�������
	if (!(fatherInode->i_mode & Inode::INodeMode::IDIR))
	{
		cout << "����һ����ȷ��Ŀ¼��!" << endl;
		throw(ENOTDIR);
		return -1;
	}

	if (this->Access(fatherInode, FileMode::WRITE) == 0) // �ж��Ƿ���Ȩ��д�ļ�
	{
		cout << "û��Ȩ��д�ļ�!" << endl;
		throw(EACCES);
		return -1;
	}

	bool isFull = true;
	int iinDir = 0;
	// ����Ȩ��д�ļ�ʱ���ж��Ƿ��������ļ����Ҳ鿴�Ƿ��п��е���Ŀ¼����д
	// ����Ҫ���������̿��
	// ����Ŀ¼�ļ�ֻռһ���̿飬����ֻ��һ�Ϊ��
	int blkno = fatherInode->Bmap(0);
	// ��ȡ���̵�����
	Buf* fatherBuf = this->bufManager->Bread(blkno);
	// ������תΪĿ¼�ṹ
	Directory* fatherDir = Char_to_Directory(fatherBuf->b_addr);
	// ѭ������Ŀ¼�е�ÿ��Ԫ��
	for (int i = 0; i < NUM_SUB_DIR; i++)
	{
		// ����ҵ���Ӧ��Ŀ¼
		if (name == fatherDir->d_filename[i])
		{
			cout << "�ļ��Ѵ���!" << endl;
			throw(EEXIST);
			return -1;
		}
		if (isFull && fatherDir->d_inodenumber[i] == 0)
		{
			isFull = false;
			iinDir = i;
		}
	}

	// ���Ŀ¼����
	if (isFull)
	{
		cout << "Ŀ¼����!" << endl;
		throw(ENOSPC);
		return -1;
	}

	// ��ſ�ʼ�����µ��ļ�
	// ����һ���µ��ڴ�Inode
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

	// ���µ�Inodeд�ش�����
	newinode->WriteI();

	// ���ļ�д��Ŀ¼����
	fatherDir->mkdir(name.c_str(), newinode->i_number);
	
	memcpy(fatherBuf->b_addr, Directory_to_Char(fatherDir), sizeof(Directory));
	this->bufManager->Bwrite(fatherBuf);

	fatherInode->i_size += sizeof(Directory) / NUM_SUB_DIR; // ���׵Ĵ�С����һ��Ŀ¼��
	fatherInode->i_atime = unsigned int(time(NULL));
	fatherInode->i_mtime = unsigned int(time(NULL));

	fatherInode->WriteI();

	// �ͷ�����Inode
	if (fatherInode != this->rootDirInode && fatherInode != this->curDirInode)
		this->IPut(fatherInode);
	this->IPut(newinode);

	return 0;
}

/**************************************************************
* fdelete ɾ���ļ�
* ������path �ļ�·��
* ����ֵ��int ɾ���ɹ�Ϊ0������Ϊ-1
***************************************************************/
int FileSystem::fdelete(string path)
{
	vector<string> paths = stringSplit(path, '/');
	if (paths.size() == 0)
	{
		cout << "·����Ч!" << endl;
		throw(EINVAL);
		return -1;
	}
	string name = paths[paths.size() - 1];
	if (name.size() > NUM_FILE_NAME)
	{
		cout << "�ļ�������!" << endl;
		throw(ENAMETOOLONG);
		return -1;
	}
	else if (name.size() == 0)
	{
		cout << "�ļ�������Ϊ��!" << endl;
		throw(EINVAL);
		return -1;
	}

	Inode* fatherInode;

	// ��·����ɾ���ļ���
	path.erase(path.size() - name.size(), name.size());
	// �ҵ���Ҫ�������ļ��ĸ��ļ�����Ӧ��Inode
	fatherInode = this->NameI(path);

	// û���ҵ���Ӧ��Inode
	if (fatherInode == NULL)
	{
		cout << "û���ҵ�" << path << endl;
		throw(ENOENT);
		return -1;
	}
	// ����ҵ����жϴ������ļ��ĸ��ļ����ǲ����ļ�������
	if (!(fatherInode->i_mode & Inode::INodeMode::IDIR))
	{
		cout << "����һ����ȷ��Ŀ¼��!" << endl;
		throw(ENOTDIR);
		return -1;
	}

	if (this->Access(fatherInode, FileMode::WRITE) == 0)
	{
		cout << "û��Ȩ��ɾ��(д)�ļ�!" << endl;
		throw(EACCES);
		return -1;
	}

	// ��ͨ�����ɾ�����ļ�
	// ����Ȩ��д�ļ�ʱ���ж��Ƿ��������ļ����Ҳ鿴�Ƿ��п��е���Ŀ¼����д
	// ����Ҫ���������̿��
	// ����Ŀ¼�ļ�ֻռһ���̿飬����ֻ��һ�Ϊ��
	int blkno = fatherInode->Bmap(0);
	// ��ȡ���̵�����
	Buf* fatherBuf = this->bufManager->Bread(blkno);
	// ������תΪĿ¼�ṹ
	Directory* fatherDir = Char_to_Directory(fatherBuf->b_addr);
	bool isFind = false;
	int iinDir = 0;
	for (int i = 0; i < NUM_SUB_DIR; i++)
	{
		// ����ҵ���Ӧ�ļ�
		if (!isFind && name == fatherDir->d_filename[i])
		{
			isFind = true;
			iinDir = i;
		}
	}

	if (!iinDir)
	{
		cout << "δ�ҵ���Ҫɾ�����ļ�!" << endl;
		return -1;
	}

	// ��ȡ��Ҫɾ�����ļ���Inode
	Inode* pDeleteInode = this->IGet(fatherDir->d_inodenumber[iinDir]);
	if (NULL == pDeleteInode)
	{
		cout << "δ�ҵ���Ҫɾ�����ļ�!" << endl;
		return -1;
	}
	if (pDeleteInode->i_mode & Inode::INodeMode::IDIR) // ������ļ�������
	{
		cout << "��������ȷ���ļ���!" << endl;
		return -1;
	}

	if (this->Access(pDeleteInode, FileMode::WRITE) == 0)
	{
		cout << "û��Ȩ��ɾ��(д)�ļ�!" << endl;
		throw(EACCES);
		return -1;
	}

	// ɾ����Ŀ¼�µ��ļ���
	fatherDir->rmi(iinDir);
	fatherInode->i_size -= sizeof(Directory) / NUM_SUB_DIR; // ���׵Ĵ�С��Сһ��Ŀ¼��
	fatherInode->i_atime = unsigned int(time(NULL));
	fatherInode->i_mtime = unsigned int(time(NULL));
	memcpy(fatherBuf->b_addr, Directory_to_Char(fatherDir), sizeof(Directory));

	// ͳһд�أ���Ŀ¼inode ��Ŀ¼���ݿ�
	fatherInode->WriteI();
	this->bufManager->Bwrite(fatherBuf);

	// �ͷ�����Inode
	if (fatherInode != this->rootDirInode && fatherInode != this->curDirInode)
		this->IPut(fatherInode);

	// �ͷ�Inode
	pDeleteInode->i_nlink--;
	this->IPut(pDeleteInode); // ��i_nlinkΪ0ʱ�����ͷ�Inode

	return 0;
}

/**************************************************************
* fwrite д�ļ�
* ������buffer д������� count д����ֽ��� fp �ļ�ָ��
* ����ֵ��
***************************************************************/
void FileSystem::fwrite(const char* buffer, int count, File* fp)
{
	if (fp == NULL)
	{
		cout << "�ļ�ָ��Ϊ��!" << endl;
		throw(EBADF);
		return;
	}

	// ����ҵ����ж��Ƿ���Ȩ�޴��ļ�
	if (this->Access(fp->f_inode, FileMode::WRITE) == 0)
	{
		cout << "û��Ȩ��д�ļ�!" << endl;
		throw(EACCES);
		return;
	}

	if (count + fp->f_offset > SIZE_BLOCK * NUM_BLOCK_ILARG)
	{
		cout << "д���ļ�̫��!" << endl;
		throw(EFBIG);
		return;
	}
	// ��ȡ�ļ���Inode
	Inode* pInode = fp->f_inode;

	// д�ļ������������
	// 1. д�����ʼλ��Ϊ�߼������ʼ��ַ��д���ֽ���Ϊ512------�첽д
	// 2. д�����ʼλ�ò����߼������ʼ��ַ��д���ֽ���Ϊ512----�ȶ���д
	//  2.1 д������ĩβ-----------------------------------------�첽д
	//	2.2 û��д������ĩβ-------------------------------------�ӳ�д

	int pos = 0; // �Ѿ�д����ֽ���
	while (pos < count)
	{
		// ���㱾��д��λ�����ļ��е�λ��
		int startpos = fp->f_offset;
		// ���㱾��д�������̿�ţ�����ļ���С�����Ļ����������·��������̿�
		int blkno = pInode->Bmap(startpos / SIZE_BLOCK);
		// ���㱾��д��Ĵ�С
		int size = SIZE_BLOCK - startpos % SIZE_BLOCK;
		if (size > count - pos)
			size = count - pos; // ����д��Ĵ�С

		cout << "[writing block no]:" << blkno << endl;
		// ���д�����ʼλ��Ϊ�߼������ʼ��ַ��д���ֽ���Ϊ512-------�첽д
		if (startpos % SIZE_BLOCK == 0 && size == SIZE_BLOCK)
		{
			// ���뻺��
			Buf* pBuf = this->bufManager->GetBlk(blkno);
			// ������д�뻺��
			memcpy(pBuf->b_addr, buffer + pos, size);
			// ����������д�����
			this->bufManager->Bwrite(pBuf);
		}
		else
		{	// д�����ʼλ�ò����߼������ʼ��ַ��д���ֽ���Ϊ512----�ȶ���д
			// ���뻺��
			Buf* pBuf = this->bufManager->Bread(blkno);
			// ������д�뻺��
			memcpy(pBuf->b_addr + startpos % SIZE_BLOCK, buffer + pos, size);

			// д������ĩβ---�첽д
			if (startpos % SIZE_BLOCK + size == SIZE_BLOCK)
				this->bufManager->Bwrite(pBuf);
			else // û��д������ĩβ---�ӳ�д
				this->bufManager->Bdwrite(pBuf);
		}

		pos += size;
		fp->f_offset += size; // �����ļ�ָ��
	}

	if (unsigned int (pInode->i_size) < fp->f_offset)
		pInode->i_size = fp->f_offset; // �����ļ���С

	pInode->i_mtime = unsigned int(time(NULL));
	pInode->i_atime = unsigned int(time(NULL)); // �����ļ���Ϣ
}

/**************************************************************
* fopen ���ļ�
* ������path �ļ�·��
* ����ֵ��File* ���ش��ļ���ָ��
***************************************************************/
int FileSystem::fopen(string path)
{
	Inode* pinode = this->NameI(path);

	// û���ҵ���Ӧ��Inode
	if (pinode == NULL)
	{
		cout << "û���ҵ���Ӧ���ļ���Ŀ¼!" << endl;
		throw(ENOENT);
		return -1;
	}

	// ����ҵ����ж���Ҫ�ҵ��ļ��ǲ����ļ�����
	if (!(pinode->i_mode & Inode::INodeMode::IFILE))
	{
		cout << "����һ����ȷ���ļ�!" << endl;
		throw(ENOTDIR);
		return -1;
	}

	// �ж��Ƿ��ж�Ȩ�޴��ļ�
	if (this->Access(pinode, FileMode::READ) == 0)
	{
		cout << "û��Ȩ�޶��ļ�!" << endl;
		throw(EACCES);
		return -1;
	}

	// ������ļ����ƿ�File�ṹ
	int fileloc = 0;
	File* pFile = this->FAlloc(fileloc);
	if (NULL == pFile)
	{
		cout << "��̫���ļ�!" << endl;
		throw(ENFILE);
		return -1;
	}
	pFile->f_inode = pinode;
	pFile->f_offset = 0;
	pFile->f_uid = this->curId;
	pFile->f_gid = this->userTable->GetGId(this->curId);

	// �޸ķ���ʱ��
	pinode->i_atime = unsigned int(time(NULL));

	return fileloc;
}

/**************************************************************
* fclose �ر��ļ�
* ������fp �ļ�ָ��
* ����ֵ��
***************************************************************/
// ����fd�ر��ļ�
void FileSystem::fclose(File* fp)
{
	// �ͷ��ڴ���
	this->IPut(fp->f_inode);
	fp->Clean();
}

/**************************************************************
* fread ���ļ����ַ�����
* ������fp �ļ�ָ�� buffer ��ȡ������Ҫ��ŵ��ַ��� count  ��ȡ���ֽ���
* ����ֵ��
***************************************************************/
void FileSystem::fread(File* fp, char*& buffer, int count)
{
	if (fp == NULL)
	{
		cout << "�ļ�ָ��Ϊ��!" << endl;
		throw(EBADF);
		return;
	}

	if (this->Access(fp->f_inode, FileMode::READ) == 0)
	{
		cout << "û��Ȩ�޶��ļ�!" << endl;
		throw(EACCES);
		return;
	}

	// ��ȡ�ļ���Inode
	Inode* pInode = fp->f_inode;
	if (count > 0)
		buffer = new char[count + 1];
	else
	{
		buffer = NULL;
		return;
	}
	int pos = 0; // �Ѿ���ȡ���ֽ���
	while (pos < count)
	{
		// ���㱾��ȡλ�����ļ��е�λ��
		int startpos = fp->f_offset;
		if (startpos >= pInode->i_size) // ��ȡλ�ó����ļ���С
			break;
		// ���㱾�ζ�ȡ�����̿�ţ�������һ���ж�,�����ж�ȡλ�ó����ļ���С������
		int blkno = pInode->Bmap(startpos / SIZE_BLOCK);
		// ���㱾�ζ�ȡ�Ĵ�С
		int size = SIZE_BLOCK - startpos % SIZE_BLOCK;
		if (size > count - pos)
			size = count - pos; // ������ȡ�Ĵ�С

		Buf* pBuf = this->bufManager->Bread(blkno);
		// TODO:���д��ļ�������Ҫ��
		memcpy(buffer + pos, pBuf->b_addr + startpos % SIZE_BLOCK, size);
		pos += size;
		fp->f_offset += size;
	}
	buffer[count] = '\0'; // ���ý������

	pInode->i_atime = unsigned int(time(NULL));
}

/**************************************************************
* fseek �ƶ��ļ�ָ��
* ������fp �ļ�ָ�� offset ƫ���� mode �ƶ���ʽ,SEEK_SET,SEEK_CUR,SEEK_END
* ����ֵ��int �ƶ��ɹ�Ϊ0������Ϊ-1
***************************************************************/
int FileSystem::fseek(File* fp, int offset, int mode)
{
	if (SEEK_SET == mode) // ���ļ�ͷ��ʼ
	{
		if (offset >= 0 && offset <= fp->f_inode->i_size)
			fp->f_offset = offset;
		else
			return -1;
	}
	else if (SEEK_CUR == mode) // �ӵ�ǰλ�ÿ�ʼ
	{
		if (offset >= 0 && (fp->f_offset + offset) >= 0)
			fp->f_offset += offset;
		else
			return -1;
	}
	else if (SEEK_END == mode) // ���ļ�β��ʼ
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
* Access ����pInode�Ƿ��и���mode��Ȩ��
* ������pInode Ҫ���ҵ�Inodeָ�� mode   Ҫ���ҵ�Ȩ��
* ����ֵ�������Ȩ�ޣ�����1�����򷵻�0
***************************************************************/
int FileSystem::Access(Inode* pInode, unsigned int mode)
{
	// ������ļ������� ������ ROOT
	if (this->curId == pInode->i_uid || this->curId == ROOT_ID)
	{
		if (mode == FileMode::WRITE) // дȨ��ǰ�����ж�Ȩ��
			return (pInode->i_mode & Inode::INodeMode::OWNER_R) && (pInode->i_mode & Inode::INodeMode::OWNER_W);
		else if (mode == FileMode::READ)
			return pInode->i_mode & Inode::INodeMode::OWNER_R;
		else
			return 0;
	}

	// �����������ʱ�����ǣ�ֻ��һ���û�
	return 0;
}
