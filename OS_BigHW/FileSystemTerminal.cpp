#include "header.h"
#include "utils.h"

/**************************************************************
* help �����ֲ�
* ������
* ����ֵ��
***************************************************************/
void FileSystem::help()
{
    printf("���������д�'<>'�����Ǳ���ģ���'[]'�����ǿ�ѡ���\n");
    printf("��ע�⣺��ϵͳ��·����'/'�ָ���windowsϵͳ·����'\\'�ָ�\n");
    printf("        VS��Ĭ�ϱ�����GBK,��Ҫ��ȷ����ļ����ݣ��뱣�ֱ���һ��\n");
    cout <<"        \033[31m�������ص�����̨,��Ҫ��ȷ�˳�ϵͳһ��Ҫ����exit\033[0m\n"; // �����ú�ɫ�ִ�ӡ����
    printf("--------------Ŀ¼���---------------\n");
    printf("ls                                      �鿴��ǰĿ¼�µ���Ŀ¼\n");
    printf("ll                                      �鿴��ǰĿ¼�µ���ϸ��Ϣ\n");
    printf("cd    <dir-name>                        ���ڵ�ǰĿ¼������Ϊdir-name����Ŀ¼\n");
    printf("mkdir <dir-name>                        �����ڵ�ǰĿ¼������Ϊdir-name����Ŀ¼\n");
    printf("rmdir <dir-name>                        ɾ���ڵ�ǰĿ¼������Ϊdir-name����Ŀ¼\n");
    printf("--------------�ļ����---------------\n");
    printf("touch <file-name>                       �ڵ�ǰĿ¼�´�������Ϊfile-name���ļ�\n");
    printf("rm    <file-name>                       ɾ����ǰĿ¼������Ϊfile-name���ļ�\n");
    printf("open  <file-name>                       �򿪵�ǰĿ¼������Ϊfile-name���ļ�\n");
    printf("chmod <file-name> <mode>                �޸ĵ�ǰĿ¼������Ϊfile-name���ļ���Ȩ��Ϊmode\n");
    printf("                                        mode��ʽ:rwrwrw,r����ɶ�,w�����д,-����û�����Ȩ��\n");
    printf("                                        ����ֱ�����ļ�������Ȩ�ޡ�ͬ���û�Ȩ�޺������û�Ȩ��\n");
    printf("close <file-name>                       �رյ�ǰĿ¼������Ϊfile-name���ļ�\n");
    printf("cat <file-name>                         ��ȡ����ӡ��ǰĿ¼������Ϊfile-name���ļ�����(��Ҫ�ȴ��ļ�)\n");
    printf("fseek <file-name> <offset>              �ƶ��ļ�ָ��offset��ƫ����������Ϊ��\n");
    printf("flseek <file-name>                      �鿴�ļ���ָ��λ��\n");
    printf("write <file-name> [mode]                �ڵ�ǰĿ¼������Ϊfile-name���ļ��￪ʼд��(��Ҫ�ȴ��ļ�)\n");
    printf("                                        mode��ѡ,������ģʽ:0��ʾ���ļ�ͷλ�ÿ�ʼд,\n");
    printf("                                        1��ʾ���ļ�ָ��λ�ÿ�ʼд,2��ʾ���ļ�β��ʼд,Ĭ��ģʽΪ0\n");
    printf("                                        ��������д��ģʽ,����д������,��ESC����ʾ����\n");
    printf("cpfwin <win-path>                       ��windowsϵͳ������·��Ϊwin-path���ļ����Ƶ���ǰĿ¼��\n");
    printf("cpffs <file-name> <win-path> <count>    ����ϵͳ�ϵ�ǰĿ¼������Ϊfile-name���ļ������ļ�ָ�뿪ʼ��λ�ø���count���ֽ�\n");
    printf("                                        ��������·��Ϊwin-path���ļ���(��Ҫ�ȴ��ļ�)\n");
    printf("listopen                                ��ӡ�Ѵ��ļ��б�\n");
    printf("----------------����----------------\n");
    printf("clear                                   �����Ļ����\n");
    printf("format                                  ��ʽ���ļ�ϵͳ\n");
    printf("exit                                    �˳�ϵͳ\n");
}

/**************************************************************
* login �û���¼
* ������
* ����ֵ��
***************************************************************/
void FileSystem::login()
{
    string name, pswd;
    short id;
    while (true)
    {
        cout << "�������û���:";
        getline(cin, name);
        cout << "����������:";
        getline(cin, pswd);
        if (name.empty() || pswd.empty())
        {
            cout << "����Ƿ�!" << endl;
            continue;
        }
        id = this->userTable->FindUser(name.c_str(), pswd.c_str());
        if (id == -1)
        {
            cout << "�û�������!" << endl;
            continue;
        }
        else
            break;
    }
    cout << "��½�ɹ�!" << endl
        << endl;
    this->curId = id;
    this->curName = name;
}

/**************************************************************
* ls �г�Ŀ¼
* ������
* ����ֵ��
***************************************************************/
void FileSystem::ls()
{
    Directory* dir = this->curDirInode->GetDir();
    int i;
    for (i = 0; i < NUM_SUB_DIR; i++)
    {
        if (dir->d_inodenumber[i] == 0)
            break;
        cout << dir->d_filename[i] << "\t";
    }
}

/**************************************************************
* cd �л�Ŀ¼
* ������subname ��Ŀ¼����
* ����ֵ��
***************************************************************/
void FileSystem::cd(string subname)
{
    // ���˵���Ŀ¼�����
    if (subname == "..")
    {
        if (this->curDir == "/") // ��Ŀ¼���
            return;

        this->curDir.erase(this->curDir.find_last_of('/'));                        // ɾ�����һ��'/'
        this->curDir = this->curDir.substr(0, this->curDir.find_last_of('/') + 1); // ��ȡ���һ��'/'
        Inode* p = this->curDirInode;
        // ���˸�Ŀ¼��Inode
        this->curDirInode = this->IGet(this->curDirInode->GetParentInumber());
        this->IPut(p); // �ͷŵ�ǰĿ¼��Inode

        return;
    }
    else if (subname == ".") // ��ǰĿ¼���
    {
        return;
    }

    // ��ͨ������������ļ�����
    Directory* dir = this->curDirInode->GetDir();
    Inode* pInode = NULL;
    int i;
    for (i = 0; i < NUM_SUB_DIR; i++)
    {
        if (dir->d_inodenumber[i] == 0)
            continue;
        if (strcmp(dir->d_filename[i], subname.c_str()) == 0)
        {
            pInode = this->IGet(dir->d_inodenumber[i]);
            if (pInode->i_mode & Inode::INodeMode::IFILE)
                continue;
            else if (pInode->i_mode & Inode::INodeMode::IDIR)
                break;
        }
    }
    if (i == NUM_SUB_DIR)
    {
        cout << "Ŀ¼������!" << endl;
        return;
    }
    this->curDir += subname + "/";
    this->curDirInode = this->IGet(dir->d_inodenumber[i]);
}

/**************************************************************
* mkdir_terminal �½�Ŀ¼
* ������subname ��Ŀ¼����
* ����ֵ��
***************************************************************/
void FileSystem::mkdir_terminal(string subname)
{

}


/**************************************************************
* rmdir ɾ����Ŀ¼
* ������subname ��Ŀ¼����
* ����ֵ��
***************************************************************/
void FileSystem::rmdir(string subname)
{

}

/**************************************************************
* ll �鿴��ǰĿ¼����ϸ��Ϣ
* ������
* ����ֵ��
***************************************************************/
void FileSystem::ll()
{
    cout<< this->curDir << "Ŀ¼�µ��ļ�:" << endl;
    Directory* dir = this->curDirInode->GetDir();
    int i;
    cout << std::left << setw(12) << "����Ȩ��"
        << std::left << setw(12) << "��ǰȨ��"
        << std::left << setw(20) << "�޸�ʱ��"
        << std::left << setw(10) << "�ļ�����"
        << std::left << setw(15) << "�ļ���С"
        << std::left << "�ļ���" << endl;
    for (i = 0; i < NUM_SUB_DIR; i++)
    {
        if (dir->d_inodenumber[i] == 0)
            break;
        Inode* p = this->IGet(dir->d_inodenumber[i]);
        string time = Timestamp_to_String(p->i_mtime);
        if (p->i_mode & Inode::INodeMode::IFILE)
            cout << std::left << setw(12) << FileMode_to_String(p->i_mode)
            << std::left << setw(12) << p->GetModeString(this->curId, this->userTable->GetGId(this->curId))
            << std::left << setw(20) << time
            << std::left << setw(10) << " "
            << std::left << setw(15) << p->i_size
            << std::left << dir->d_filename[i] << endl;
        else if (p->i_mode & Inode::INodeMode::IDIR)
            cout << std::left << setw(12) << FileMode_to_String(p->i_mode)
            << std::left << setw(12) << p->GetModeString(this->curId, this->userTable->GetGId(this->curId))
            << std::left << setw(20) << time
            << std::left << setw(10) << "<DIR>"
            << std::left << setw(15) << " "
            << std::left << dir->d_filename[i] << endl;
        this->IPut(p);
    }
}

/**************************************************************
* openFile ���ļ�
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::openFile(string path)
{
    int fd = this->fopen(path);
    if (fd == -1)
    {
        cerr << "�ļ���ʧ��!" << endl;
        return;
    }
    else
    {
        this->openFileMap[this->GetAbsolutionPath(path)] = fd + 1;
        cout << "�ļ��򿪳ɹ�!" << endl;
    }
}

/**************************************************************
* closeFile �ر��ļ�
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::closeFile(string path)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd == 0)
    {
        cerr << "�ļ�δ��!" << endl;
        return;
    }
    else
    {
        this->fclose(&(this->openFileTable[fd - 1]));
        this->openFileMap.erase(this->GetAbsolutionPath(path));
        cout << "�ɹ��ر��ļ�!" << endl;
    }
}

/**************************************************************
* createFile �����ļ�
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::createFile(string path)
{
    if (path.find_first_of("/") != -1)
    {
        cerr << "�ļ������ܰ���'/'!" << endl;
        return;
    }
    if (path == "." || path == "..")
    {
        cerr << "�ļ������淶!" << endl;
        return;
    }

    int res = this->fcreate(path);
    if (res == 0)
    {
        cout << "�ļ������ɹ�!" << endl;
        return;
    }
    
    cerr << "�ļ�����ʧ��!" << endl;
    return;

}

/**************************************************************
* removeFile ɾ���ļ�
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::removeFile(string path)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd != 0)
    {
        cout << "�ļ��Ѵ�!���Զ������ر��ļ�" << endl;
        this->closeFile(path);
    }
    int res = this->fdelete(this->curDir + path);
    if (res == 0)
    {
        cout << "�ļ�ɾ���ɹ�!" << endl;
        return;
    }

    cerr << "�ļ�ɾ��ʧ��!" << endl;
    return;

}

/**************************************************************
* writeFile д�ļ�
* ������path �ļ�·�� modeдģʽ 0-�ļ�ͷ 1-�ļ�ָ��λ�� 2-�ļ�β
* Ĭ��modeΪ0
* ����ֵ��
***************************************************************/
void FileSystem::writeFile(string path, int mode)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd == 0)
    {
        cout << "�ļ�δ��!����ʹ��openָ����ļ�" << endl;
        return;
    }

    File* fp = &(this->openFileTable[fd - 1]);

    // ���Ȩ��
    if (this->Access(fp->f_inode, FileMode::WRITE) == 0)
    {
        cerr << "�ļ�û��дȨ��!" << endl;
        return;
    }

    if (this->fseek(fp, 0, mode) == -1)
    {
        cout << "�ļ�ָ���ƶ�ʧ��!" << endl;
        return;
    }

    cout << "��ʼ�����ַ�(��ESC���˳�):" << endl;
    string input;
    int i = 0;
    while (true)
    {
        if (_kbhit()) // �а�������
        {                       
            char ch = _getch(); // ��ȡ�����ַ�����

            if (ch == 27) // ESC ��
            {          
                break;
            }

            if (ch == '\r')
            {                  // ����Ƿ�����س���
                input += '\n'; // ���س���ת��Ϊ���з�����ӵ������ַ�����
                cout << endl;  // ������з�
            }
            else
            {
                input += ch; // ���ַ���ӵ������ַ�����
                cout << ch;  // ��ʾ��ǰ������ַ�
            }
            i++;
        }
    }
    cout << endl
        << "���������ַ�������" << i << endl;

    this->fwrite(input.c_str(), (int)input.size(), fp);
}

/**************************************************************
* printFile ��ӡ�ļ����� ��Ӧcatָ��
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::printFile(string path)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd == 0)
    {
        cout << "�ļ�δ��!����ʹ��openָ����ļ�" << endl;
        return;
    }

    File* fp = &(this->openFileTable[fd - 1]);
    char* buffer = NULL;
    int count = fp->f_inode->i_size;
    int oldoffset = fp->f_offset; // ��¼�ɵ��ļ�ָ��λ��
    fp->f_offset = 0;
    this->fread(fp, buffer, count);
    fp->f_offset = oldoffset;
    if (buffer == NULL)
    {
        cout << "�ļ�Ϊ��!" << endl;
        return;
    }
    cout << "�ļ�����Ϊ:" << endl;
    cout << "\033[31m" << buffer << "\033[0m"; // �����ú�ɫ�ִ�ӡ����
    cout << endl << "�ļ�����!" << endl;
}

/**************************************************************
* change_fseek �����ļ���дָ�� CURģʽ
* ������path �ļ�·�� offset �ļ�ָ��λ��(��Ϊ����,�����Ϳ���ʵ������)
* ����ֵ��
***************************************************************/
void FileSystem::change_fseek(string path, int offset)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd == 0)
    {
        cout << "�ļ�δ��!����ʹ��openָ����ļ�" << endl;
        return;
    }

    Inode* p = this->NameI(this->GetAbsolutionPath(path));
    if (p == NULL)
    {
        cout << "�ļ�������!" << endl;
        return;
    }
    File* fp = &(this->openFileTable[fd - 1]);
    if ((fp->f_offset + offset) < 0 || int(fp->f_offset + offset) > p->i_size)
    {
        cout << "�ļ�ָ�볬����Χ!" << endl;
        return;
    }
    fp->f_offset += offset;
    cout << "�ļ�ָ�����ƶ���" << fp->f_offset << endl;
}

/**************************************************************
* flseek �鿴��ǰ�ļ�ָ��
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::flseek(string path)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd == 0)
    {
        cout << "�ļ�δ��!����ʹ��openָ����ļ�" << endl;
        return;
    }

    Inode* p = this->NameI(this->GetAbsolutionPath(path));
    if (p == NULL)
    {
        cout << "�ļ�������!" << endl;
        return;
    }
    File* fp = &(this->openFileTable[fd - 1]);
    cout << "��ǰ�ļ�ָ��λ��: " << fp->f_offset << endl;
}

/**************************************************************
* copy_from_win ��Windowsϵͳ�µ��ļ����ƽ���FileSystem(��ǰĿ¼��)
* ������path Windows���ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::copy_from_win(string path)
{
    fstream fd_win;
    fd_win.open(path, ios::in | ios::binary);
    if (!fd_win.is_open())
    {
        cerr << "�޷���windows�´��ļ�" << path << endl;
        return;
    }
    fd_win.seekg(0, fd_win.end);
    int filesize = static_cast<int>(fd_win.tellg()); // ��ȡ�ļ���С
    fd_win.seekg(0, fd_win.beg);
    char* buffer = new char[filesize + 1];
    fd_win.read(buffer, filesize); // ��ȡ�ļ�����
    buffer[filesize] = '\0';
    fd_win.close();

    vector<string> paths = stringSplit(path, '\\'); // winϵͳ�ϵ�·���ָ��Ϊ'\'
    string filename = paths[paths.size() - 1];      // ��ȡ�ļ���

    // �����ļ�
    int res = this->fcreate(filename);
    if (res == 0)
    {
        int fileloc = this->fopen(filename);
        File* fileptr = &(this->openFileTable[fileloc]);
        this->fwrite(buffer, filesize, fileptr);
        this->fclose(fileptr);
        cout << "�ɹ�����windows�ļ�" << filename << ",д���СΪ" << filesize << endl;
    }
}

/**************************************************************
* copy_from_fs ����FileSystemϵͳ��ǰĿ¼�µ��ļ����ƽ�Windowsϵͳ��
* ������filename �ļ��� winpath Windows���ļ�·�� count ���Ƶ��ֽ������ӵ�ǰ���ļ�ָ�뿪ʼ��
* ����ֵ��
***************************************************************/
void FileSystem::copy_from_fs(string filename, string winpath, int count)
{
    fstream fd;
    fd.open(winpath, ios::out);
    if (!fd.is_open())
    {
        cerr << "�޷���windows�ļ�" << winpath << endl;
        return;
    }

    // ��fs�е��ļ�
    int fileloc = this->openFileMap[this->GetAbsolutionPath(filename)];
    if (fileloc == 0)
    {
        cerr << "��ϵͳ�ļ�δ��!����ʹ��openָ����ļ�" << endl;
        return;
    }

    File* fp = &(this->openFileTable[fileloc - 1]);

    char* buffer = NULL;
    // ������ȡ��С
    int actualcount = ((count + (int)fp->f_offset) < fp->f_inode->i_size) ? count : (fp->f_inode->i_size - fp->f_offset);
    this->fread(fp, buffer, actualcount);

    // �鿴��ϵͳ�ļ�
    if (buffer == NULL)
    {
        cerr << "��ϵͳ�ļ�Ϊ��!" << endl;
        return;
    }
    fd.write(buffer, actualcount); // д���ļ�����
    fd.close(); // �ر�windows�ļ�
    cout << "�ɹ������ļ�" << filename << ",д���СΪ" << actualcount << endl;
}

/**************************************************************
* print0penFileList ��ӡ���ļ��б�
* ������
* ����ֵ��
***************************************************************/
void FileSystem::print0penFileList()
{
    cout << "��ǰ���ļ��б�:" << endl;
    if (this->openFileMap.empty())
    {
        cout << "�޴��ļ�!" << endl;
        return;
    }
    cout << std::left << setw(20) << "�ļ���·��" << setw(10) << "�ļ�������" << setw(10) << "�ļ�ָ��" << endl;
    for (const auto& pair : this->openFileMap)
        cout << std::left << setw(20) << pair.first << setw(10) << pair.second << setw(10) << this->openFileTable[pair.second - 1].f_offset << endl;
    cout << endl;
}

/**************************************************************
* chmod �����ļ�Ȩ��
* ������path �ļ�·�� mode Ȩ�޲���
* ����ֵ��
***************************************************************/
void FileSystem::chmod(string path, string mode)
{
    if (mode.size() != 6)
    {
        cerr << "�����Ȩ�޸�ʽ����ȷ!" << endl;
        return;
    }

    Inode* p = this->NameI(path);
    if (p == NULL)
    {
        cerr << "�ļ�������!" << endl;
        return;
    }

    unsigned short modeNum = p->String_to_Mode(mode);
    if (modeNum == -1)
    {
        cerr << "�����Ȩ�޸�ʽ����ȷ!" << endl;
        return;
    }

    int res = p->AssignMode(modeNum);
    if (res == 0)
        cout << "�޸ĳɹ�!" << endl;
    else
        cerr << "���ܸı��ļ�����!" << endl;
}

/**************************************************************
* format ��ʽ������
* ������
* ����ֵ��
***************************************************************/
void FileSystem::format()
{
    cout << "ȷ��Ҫ���и�ʽ��?[y/n] ";
    string strIn;
    getline(cin, strIn);
    
    if (strIn == "y" || strIn == "Y")
    {
        cout << "���ڸ�ʽ������" << endl;
        this->fformat();
        cout << "��ʽ������" << endl;
    }
    else if (strIn == "n" || strIn == "N")
        return;
    else
    {
        cout << "����Ƿ�" << endl;
        return;
    }
}