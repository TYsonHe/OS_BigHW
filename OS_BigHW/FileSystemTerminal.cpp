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
    cout << "        \033[31m�������ص�����̨,��Ҫ��ȷ�˳�ϵͳһ��Ҫ����exit\033[0m\n"; // �����ú�ɫ�ִ�ӡ����
    printf("--------------Ŀ¼���---------------\n");
    printf("ls                                      �鿴��ǰĿ¼�µ���Ŀ¼\n");
    printf("dir                                     �鿴��ǰĿ¼�µ���ϸ��Ϣ\n");
    printf("cd    <dir-name>                        ���ڵ�ǰĿ¼������Ϊdir-name����Ŀ¼\n");
    printf("mkdir <dir-name>                        �����ڵ�ǰĿ¼������Ϊdir-name����Ŀ¼\n");
    printf("rmdir <dir-name>                        ɾ���ڵ�ǰĿ¼������Ϊdir-name����Ŀ¼\n");
    printf("--------------�ļ����---------------\n");
    printf("touch <file-name>                       �ڵ�ǰĿ¼�´�������Ϊfile-name���ļ�\n");
    printf("rm    <file-name>                       ɾ����ǰĿ¼������Ϊfile-name���ļ�\n");
    printf("open  <file-name>                       �򿪵�ǰĿ¼������Ϊfile-name���ļ�\n");
    /*printf("chmod <file-name> <mode>                �޸ĵ�ǰĿ¼������Ϊfile-name���ļ���Ȩ��Ϊmode\n");
    printf("                                        mode��ʽ:rwrwrw,r����ɶ�,w�����д,-����û�����Ȩ��\n");
    printf("                                                ����ֱ�����ļ�������Ȩ�ޡ�ͬ���û�Ȩ�޺������û�Ȩ��\n");
    printf("                                                eg. rwr-r-�����ļ�������Ȩ�޿ɶ�д��ͬ���û�Ȩ�޿ɶ��������û�Ȩ�޿ɶ�\n");*/
    printf("close <file-name>                       �رյ�ǰĿ¼������Ϊfile-name���ļ�\n");
    printf("print <file-name>                       ��ȡ����ӡ��ǰĿ¼������Ϊfile-name���ļ�����(��Ҫ�ȴ��ļ�)\n");
    printf("fseek <file-name> <offset>              �ƶ��ļ�ָ��offset��ƫ����������Ϊ��\n");
    printf("write <file-name> [mode]                �ڵ�ǰĿ¼������Ϊfile-name���ļ��￪ʼд��(��Ҫ�ȴ��ļ�)\n");
    printf("                                        mode��ѡ,������ģʽ:0��ʾ���ļ�ͷλ�ÿ�ʼд,\n");
    printf("                                        1��ʾ���ļ�ָ��λ�ÿ�ʼд,2��ʾ���ļ�β��ʼд,Ĭ��ģʽΪ0\n");
    printf("                                        ��������д��ģʽ,����д������,��ESC����ʾ����\n");
    printf("cpfwin <win-path>                       ��windowsϵͳ������·��Ϊwin-path���ļ����Ƶ���ǰĿ¼��\n");
    printf("cpffs <file-name> <win-path> <count>    ����ϵͳ�ϵ�ǰĿ¼������Ϊfile-name���ļ������ļ�ָ�뿪ʼ��λ�ø���count���ֽ�\n");
    printf("                                        ��������·��Ϊwin-path���ļ���(��Ҫ�ȴ��ļ�)\n");
    printf("listopen                                ��ӡ�Ѵ��ļ��б�\n");
    printf("--------------�û����---------------\n");
    printf("relogin                                 ���µ�¼,��ر����е��ļ�,���֮ǰ���е�����\n");
    /*printf("adduser                                 ������û�,����ֻ����root�û�����\n");
    printf("deluser                                 ɾ���û�,����ֻ����root�û�����\n");
    printf("chgroup                                 �ı��û��û���,����ֻ����root�û�����\n");*/
    printf("listuser                                ��ӡ�����û���Ϣ\n");
    printf("----------------����----------------\n");
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
    // cout << endl;
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
* dir �鿴��ǰĿ¼����ϸ��Ϣ
* ������
* ����ֵ��
***************************************************************/
void FileSystem::dir()
{

}

/**************************************************************
* openFile ���ļ�
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::openFile(string path)
{

}

/**************************************************************
* closeFile �ر��ļ�
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::closeFile(string path)
{

}

/**************************************************************
* createFile �����ļ�
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::createFile(string path)
{

}

/**************************************************************
* removeFile ɾ���ļ�
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::removeFile(string path)
{

}

/**************************************************************
* writeFile д�ļ�
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::writeFile(string path, int mode)
{

}

/**************************************************************
* printFile ��ӡ�ļ�����
* ������path �ļ�·��
* ����ֵ��
***************************************************************/
void FileSystem::printFile(string path)
{

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
    if ((fp->f_offset + offset) < 0 || (fp->f_offset + offset) > p->i_size)
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