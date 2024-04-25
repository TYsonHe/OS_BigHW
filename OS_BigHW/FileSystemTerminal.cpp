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
    cout <<"        \033[31m请勿随便关掉控制台,想要正确退出系统一定要输入exit\033[0m\n"; // 设置用红色字打印出来
    printf("--------------目录相关---------------\n");
    printf("ls                                      查看当前目录下的子目录\n");
    printf("ll                                      查看当前目录下的详细信息\n");
    printf("cd    <dir-name>                        打开在当前目录下名称为dir-name的子目录\n");
    printf("mkdir <dir-name>                        创建在当前目录下名称为dir-name的子目录\n");
    printf("rmdir <dir-name>                        删除在当前目录下名称为dir-name的子目录\n");
    printf("--------------文件相关---------------\n");
    printf("touch <file-name>                       在当前目录下创建名称为file-name的文件\n");
    printf("rm    <file-name>                       删除当前目录里名称为file-name的文件\n");
    printf("open  <file-name>                       打开当前目录里名称为file-name的文件\n");
    printf("chmod <file-name> <mode>                修改当前目录下名称为file-name的文件的权限为mode\n");
    printf("                                        mode格式:rwrwrw,r代表可读,w代表可写,-代表没有这个权限\n");
    printf("                                        三组分别代表文件创建者权限、同组用户权限和其他用户权限\n");
    printf("close <file-name>                       关闭当前目录里名称为file-name的文件\n");
    printf("cat <file-name>                         读取并打印当前目录里名称为file-name的文件内容(需要先打开文件)\n");
    printf("fseek <file-name> <offset>              移动文件指针offset个偏移量，可以为负\n");
    printf("flseek <file-name>                      查看文件的指针位置\n");
    printf("write <file-name> [mode]                在当前目录里名称为file-name的文件里开始写入(需要先打开文件)\n");
    printf("                                        mode可选,有三种模式:0表示从文件头位置开始写,\n");
    printf("                                        1表示从文件指针位置开始写,2表示从文件尾开始写,默认模式为0\n");
    printf("                                        输入后进入写入模式,输入写入内容,按ESC键表示结束\n");
    printf("cpfwin <win-path>                       将windows系统电脑上路径为win-path的文件复制到当前目录中\n");
    printf("cpffs <file-name> <win-path> <count>    将本系统上当前目录中名称为file-name的文件按从文件指针开始的位置复制count个字节\n");
    printf("                                        到电脑上路径为win-path的文件里(需要先打开文件)\n");
    printf("listopen                                打印已打开文件列表\n");
    printf("----------------其他----------------\n");
    printf("clear                                   清空屏幕内容\n");
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
* ll 查看当前目录的详细信息
* 参数：
* 返回值：
***************************************************************/
void FileSystem::ll()
{
    cout<< this->curDir << "目录下的文件:" << endl;
    Directory* dir = this->curDirInode->GetDir();
    int i;
    cout << std::left << setw(12) << "所有权限"
        << std::left << setw(12) << "当前权限"
        << std::left << setw(20) << "修改时间"
        << std::left << setw(10) << "文件类型"
        << std::left << setw(15) << "文件大小"
        << std::left << "文件名" << endl;
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
* openFile 打开文件
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::openFile(string path)
{
    int fd = this->fopen(path);
    if (fd == -1)
    {
        cerr << "文件打开失败!" << endl;
        return;
    }
    else
    {
        this->openFileMap[this->GetAbsolutionPath(path)] = fd + 1;
        cout << "文件打开成功!" << endl;
    }
}

/**************************************************************
* closeFile 关闭文件
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::closeFile(string path)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd == 0)
    {
        cerr << "文件未打开!" << endl;
        return;
    }
    else
    {
        this->fclose(&(this->openFileTable[fd - 1]));
        this->openFileMap.erase(this->GetAbsolutionPath(path));
        cout << "成功关闭文件!" << endl;
    }
}

/**************************************************************
* createFile 创建文件
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::createFile(string path)
{
    if (path.find_first_of("/") != -1)
    {
        cerr << "文件名不能包含'/'!" << endl;
        return;
    }
    if (path == "." || path == "..")
    {
        cerr << "文件名不规范!" << endl;
        return;
    }

    int res = this->fcreate(path);
    if (res == 0)
    {
        cout << "文件创建成功!" << endl;
        return;
    }
    
    cerr << "文件创建失败!" << endl;
    return;

}

/**************************************************************
* removeFile 删除文件
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::removeFile(string path)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd != 0)
    {
        cout << "文件已打开!将自动帮您关闭文件" << endl;
        this->closeFile(path);
    }
    int res = this->fdelete(this->curDir + path);
    if (res == 0)
    {
        cout << "文件删除成功!" << endl;
        return;
    }

    cerr << "文件删除失败!" << endl;
    return;

}

/**************************************************************
* writeFile 写文件
* 参数：path 文件路径 mode写模式 0-文件头 1-文件指针位置 2-文件尾
* 默认mode为0
* 返回值：
***************************************************************/
void FileSystem::writeFile(string path, int mode)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd == 0)
    {
        cout << "文件未打开!请先使用open指令打开文件" << endl;
        return;
    }

    File* fp = &(this->openFileTable[fd - 1]);

    // 检查权限
    if (this->Access(fp->f_inode, FileMode::WRITE) == 0)
    {
        cerr << "文件没有写权限!" << endl;
        return;
    }

    if (this->fseek(fp, 0, mode) == -1)
    {
        cout << "文件指针移动失败!" << endl;
        return;
    }

    cout << "开始输入字符(按ESC键退出):" << endl;
    string input;
    int i = 0;
    while (true)
    {
        if (_kbhit()) // 有按键按下
        {                       
            char ch = _getch(); // 获取单个字符输入

            if (ch == 27) // ESC 键
            {          
                break;
            }

            if (ch == '\r')
            {                  // 检查是否输入回车符
                input += '\n'; // 将回车符转换为换行符并添加到输入字符串中
                cout << endl;  // 输出换行符
            }
            else
            {
                input += ch; // 将字符添加到输入字符串中
                cout << ch;  // 显示当前输入的字符
            }
            i++;
        }
    }
    cout << endl
        << "本次输入字符个数：" << i << endl;

    this->fwrite(input.c_str(), (int)input.size(), fp);
}

/**************************************************************
* printFile 打印文件内容 对应cat指令
* 参数：path 文件路径
* 返回值：
***************************************************************/
void FileSystem::printFile(string path)
{
    int fd = this->openFileMap[this->GetAbsolutionPath(path)];
    if (fd == 0)
    {
        cout << "文件未打开!请先使用open指令打开文件" << endl;
        return;
    }

    File* fp = &(this->openFileTable[fd - 1]);
    char* buffer = NULL;
    int count = fp->f_inode->i_size;
    int oldoffset = fp->f_offset; // 记录旧的文件指针位置
    fp->f_offset = 0;
    this->fread(fp, buffer, count);
    fp->f_offset = oldoffset;
    if (buffer == NULL)
    {
        cout << "文件为空!" << endl;
        return;
    }
    cout << "文件内容为:" << endl;
    cout << "\033[31m" << buffer << "\033[0m"; // 设置用红色字打印出来
    cout << endl << "文件结束!" << endl;
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
    if ((fp->f_offset + offset) < 0 || int(fp->f_offset + offset) > p->i_size)
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

/**************************************************************
* copy_from_win 将Windows系统下的文件复制进本FileSystem(当前目录下)
* 参数：path Windows下文件路径
* 返回值：
***************************************************************/
void FileSystem::copy_from_win(string path)
{
    fstream fd_win;
    fd_win.open(path, ios::in | ios::binary);
    if (!fd_win.is_open())
    {
        cerr << "无法在windows下打开文件" << path << endl;
        return;
    }
    fd_win.seekg(0, fd_win.end);
    int filesize = static_cast<int>(fd_win.tellg()); // 获取文件大小
    fd_win.seekg(0, fd_win.beg);
    char* buffer = new char[filesize + 1];
    fd_win.read(buffer, filesize); // 读取文件内容
    buffer[filesize] = '\0';
    fd_win.close();

    vector<string> paths = stringSplit(path, '\\'); // win系统上的路径分割符为'\'
    string filename = paths[paths.size() - 1];      // 获取文件名

    // 创建文件
    int res = this->fcreate(filename);
    if (res == 0)
    {
        int fileloc = this->fopen(filename);
        File* fileptr = &(this->openFileTable[fileloc]);
        this->fwrite(buffer, filesize, fileptr);
        this->fclose(fileptr);
        cout << "成功导入windows文件" << filename << ",写入大小为" << filesize << endl;
    }
}

/**************************************************************
* copy_from_fs 将本FileSystem系统当前目录下的文件复制进Windows系统中
* 参数：filename 文件名 winpath Windows下文件路径 count 复制的字节数（从当前该文件指针开始）
* 返回值：
***************************************************************/
void FileSystem::copy_from_fs(string filename, string winpath, int count)
{
    fstream fd;
    fd.open(winpath, ios::out);
    if (!fd.is_open())
    {
        cerr << "无法打开windows文件" << winpath << endl;
        return;
    }

    // 打开fs中的文件
    int fileloc = this->openFileMap[this->GetAbsolutionPath(filename)];
    if (fileloc == 0)
    {
        cerr << "本系统文件未打开!请先使用open指令打开文件" << endl;
        return;
    }

    File* fp = &(this->openFileTable[fileloc - 1]);

    char* buffer = NULL;
    // 修正读取大小
    int actualcount = ((count + (int)fp->f_offset) < fp->f_inode->i_size) ? count : (fp->f_inode->i_size - fp->f_offset);
    this->fread(fp, buffer, actualcount);

    // 查看本系统文件
    if (buffer == NULL)
    {
        cerr << "本系统文件为空!" << endl;
        return;
    }
    fd.write(buffer, actualcount); // 写入文件内容
    fd.close(); // 关闭windows文件
    cout << "成功导出文件" << filename << ",写入大小为" << actualcount << endl;
}

/**************************************************************
* print0penFileList 打印打开文件列表
* 参数：
* 返回值：
***************************************************************/
void FileSystem::print0penFileList()
{
    cout << "当前打开文件列表:" << endl;
    if (this->openFileMap.empty())
    {
        cout << "无打开文件!" << endl;
        return;
    }
    cout << std::left << setw(20) << "文件名路径" << setw(10) << "文件描述符" << setw(10) << "文件指针" << endl;
    for (const auto& pair : this->openFileMap)
        cout << std::left << setw(20) << pair.first << setw(10) << pair.second << setw(10) << this->openFileTable[pair.second - 1].f_offset << endl;
    cout << endl;
}

/**************************************************************
* chmod 更改文件权限
* 参数：path 文件路径 mode 权限参数
* 返回值：
***************************************************************/
void FileSystem::chmod(string path, string mode)
{
    if (mode.size() != 6)
    {
        cerr << "输入的权限格式不正确!" << endl;
        return;
    }

    Inode* p = this->NameI(path);
    if (p == NULL)
    {
        cerr << "文件不存在!" << endl;
        return;
    }

    unsigned short modeNum = p->String_to_Mode(mode);
    if (modeNum == -1)
    {
        cerr << "输入的权限格式不正确!" << endl;
        return;
    }

    int res = p->AssignMode(modeNum);
    if (res == 0)
        cout << "修改成功!" << endl;
    else
        cerr << "不能改变文件属性!" << endl;
}

/**************************************************************
* format 格式化磁盘
* 参数：
* 返回值：
***************************************************************/
void FileSystem::format()
{
    cout << "确定要进行格式化?[y/n] ";
    string strIn;
    getline(cin, strIn);
    
    if (strIn == "y" || strIn == "Y")
    {
        cout << "正在格式化……" << endl;
        this->fformat();
        cout << "格式化结束" << endl;
    }
    else if (strIn == "n" || strIn == "N")
        return;
    else
    {
        cout << "输入非法" << endl;
        return;
    }
}