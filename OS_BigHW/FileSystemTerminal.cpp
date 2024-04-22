#include "header.h"
#include "utils.h"

/**************************************************************
* help 帮助手册
* 参数：
* 返回值：
***************************************************************/
void FileSystem::help()
{
    printf("下列命令中带'<>'的项是必须的，带'[]'的项是可选择的\n");
    printf("请注意：本系统中路径用'/'分隔，windows系统路径用'\\'分割\n");
    printf("        VS中默认编码是GBK,想要正确输出文件内容，请保持编码一致\n");
    cout << "        \033[31m请勿随便关掉控制台,想要正确退出系统一定要输入exit\033[0m\n"; // 设置用红色字打印出来
    printf("--------------目录相关---------------\n");
    printf("ls                                      查看当前目录下的子目录\n");
    printf("dir                                     查看当前目录下的详细信息\n");
    printf("cd    <dir-name>                        打开在当前目录下名称为dir-name的子目录\n");
    printf("mkdir <dir-name>                        创建在当前目录下名称为dir-name的子目录\n");
    printf("rmdir <dir-name>                        删除在当前目录下名称为dir-name的子目录\n");
    printf("--------------文件相关---------------\n");
    printf("touch <file-name>                       在当前目录下创建名称为file-name的文件\n");
    printf("rm    <file-name>                       删除当前目录里名称为file-name的文件\n");
    printf("open  <file-name>                       打开当前目录里名称为file-name的文件\n");
    /*printf("chmod <file-name> <mode>                修改当前目录下名称为file-name的文件的权限为mode\n");
    printf("                                        mode格式:rwrwrw,r代表可读,w代表可写,-代表没有这个权限\n");
    printf("                                                三组分别代表文件创建者权限、同组用户权限和其他用户权限\n");
    printf("                                                eg. rwr-r-代表文件创建者权限可读写、同组用户权限可读和其他用户权限可读\n");*/
    printf("close <file-name>                       关闭当前目录里名称为file-name的文件\n");
    printf("print <file-name>                       读取并打印当前目录里名称为file-name的文件内容(需要先打开文件)\n");
    printf("fseek <file-name> <offset>              移动文件指针offset个偏移量，可以为负\n");
    printf("write <file-name> [mode]                在当前目录里名称为file-name的文件里开始写入(需要先打开文件)\n");
    printf("                                        mode可选,有三种模式:0表示从文件头位置开始写,\n");
    printf("                                        1表示从文件指针位置开始写,2表示从文件尾开始写,默认模式为0\n");
    printf("                                        输入后进入写入模式,输入写入内容,按ESC键表示结束\n");
    printf("cpfwin <win-path>                       将windows系统电脑上路径为win-path的文件复制到当前目录中\n");
    printf("cpffs <file-name> <win-path> <count>    将本系统上当前目录中名称为file-name的文件按从文件指针开始的位置复制count个字节\n");
    printf("                                        到电脑上路径为win-path的文件里(需要先打开文件)\n");
    printf("listopen                                打印已打开文件列表\n");
    printf("--------------用户相关---------------\n");
    printf("relogin                                 重新登录,会关闭所有的文件,完成之前所有的任务\n");
    /*printf("adduser                                 添加新用户,但是只能由root用户操作\n");
    printf("deluser                                 删除用户,但是只能由root用户操作\n");
    printf("chgroup                                 改变用户用户组,但是只能由root用户操作\n");*/
    printf("listuser                                打印所有用户信息\n");
    printf("----------------其他----------------\n");
    printf("format                                  格式化文件系统\n");
    printf("exit                                    退出系统\n");
}

/**************************************************************
* login 用户登录
* 参数：
* 返回值：
***************************************************************/
void FileSystem::login()
{
    string name, pswd;
    short id;
    while (true)
    {
        cout << "请输入用户名:";
        getline(cin, name);
        cout << "请输入密码:";
        getline(cin, pswd);
        if (name.empty() || pswd.empty())
        {
            cout << "输入非法!" << endl;
            continue;
        }
        id = this->userTable->FindUser(name.c_str(), pswd.c_str());
        if (id == -1)
        {
            cout << "用户不存在!" << endl;
            continue;
        }
        else
            break;
    }
    cout << "登陆成功!" << endl
        << endl;
    this->curId = id;
    this->curName = name;
}

/**************************************************************
* ls 列出目录
* 参数：
* 返回值：
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
* cd 切换目录
* 参数：subname 子目录名称
* 返回值：
***************************************************************/
void FileSystem::cd(string subname)
{
    // 回退到父目录的情况
    if (subname == "..")
    {
        if (this->curDir == "/") // 根目录情况
            return;

        this->curDir.erase(this->curDir.find_last_of('/'));                        // 删除最后一个'/'
        this->curDir = this->curDir.substr(0, this->curDir.find_last_of('/') + 1); // 截取最后一个'/'
        Inode* p = this->curDirInode;
        // 回退父目录的Inode
        this->curDirInode = this->IGet(this->curDirInode->GetParentInumber());
        this->IPut(p); // 释放当前目录的Inode

        return;
    }
    else if (subname == ".") // 当前目录情况
    {
        return;
    }

    // 普通情况，进入子文件夹中
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
        cout << "目录不存在!" << endl;
        return;
    }
    this->curDir += subname + "/";
    this->curDirInode = this->IGet(dir->d_inodenumber[i]);
}

/**************************************************************
* mkdir_terminal 新建目录
* 参数：subname 子目录名称
* 返回值：
***************************************************************/
void FileSystem::mkdir_terminal(string subname)
{

}


/**************************************************************
* rmdir 删除子目录
* 参数：subname 子目录名称
* 返回值：
***************************************************************/
void FileSystem::rmdir(string subname)
{

}

/**************************************************************
* dir 查看当前目录的详细信息
* 参数：
* 返回值：
***************************************************************/
void FileSystem::dir()
{

}

/**************************************************************
* openFile 打开文件
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::openFile(string path)
{

}

/**************************************************************
* closeFile 关闭文件
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::closeFile(string path)
{

}

/**************************************************************
* createFile 创建文件
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::createFile(string path)
{

}

/**************************************************************
* removeFile 删除文件
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::removeFile(string path)
{

}

/**************************************************************
* writeFile 写文件
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::writeFile(string path, int mode)
{

}

/**************************************************************
* printFile 打印文件内容
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::printFile(string path)
{

}

/**************************************************************
* change_fseek 更改文件读写指针 CUR模式
* 参数：path 文件路径 offset 文件指针位置(可为负数,这样就可以实现往回)
* 返回值：
***************************************************************/
void FileSystem::change_fseek(string path, int offset)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd == 0)
    {
        cout << "文件未打开!请先使用open指令打开文件" << endl;
        return;
    }

    Inode* p = this->NameI(this->GetAbsolutionPath(path));
    if (p == NULL)
    {
        cout << "文件不存在!" << endl;
        return;
    }
    File* fp = &(this->openFileTable[fd - 1]);
    if ((fp->f_offset + offset) < 0 || (fp->f_offset + offset) > p->i_size)
    {
        cout << "文件指针超出范围!" << endl;
        return;
    }
    fp->f_offset += offset;
    cout << "文件指针已移动到" << fp->f_offset << endl;
}

/**************************************************************
* flseek 查看当前文件指针
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::flseek(string path)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd == 0)
    {
        cout << "文件未打开!请先使用open指令打开文件" << endl;
        return;
    }

    Inode* p = this->NameI(this->GetAbsolutionPath(path));
    if (p == NULL)
    {
        cout << "文件不存在!" << endl;
        return;
    }
    File* fp = &(this->openFileTable[fd - 1]);
    cout << "当前文件指针位置: " << fp->f_offset << endl;
}