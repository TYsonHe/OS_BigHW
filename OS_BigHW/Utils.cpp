#include "utils.h"

/**************************************************************
* SuperBlk_to_Char 将SuperBlock*转换为Char*
* 参数：SuperBlock* 要转换的SuperBlock的指针
* 返回值：Char* 转换后的字符
***************************************************************/
char* SuperBlk_to_Char(SuperBlock* spb) {
    try
    {
        char* chptr = new char[sizeof(SuperBlock) + 1];
        chptr[sizeof(SuperBlock)] = '\0';
        // reinterpret_cast数据从一种类型的转换为另一种类型。也就是说将数据以二进制存在形式的重新解释
        memcpy(chptr, reinterpret_cast<char*>(spb), sizeof(SuperBlock));
        return chptr;
    }
    catch (exception& e)
    {
        cerr << "SuperBlk_to_Char出现错误: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* Char_to_SuperBlock 将char*转换为SuperBlock*
* 参数：char* 要转换的char的指针
* 返回值：SuperBlock* 转换后的SuperBlock结构
***************************************************************/
SuperBlock* Char_to_SuperBlock(char* ch)
{
    try
    {
        SuperBlock* objPtr = reinterpret_cast<SuperBlock*>(ch);
        return objPtr;
    }
    catch (exception& e)
    {
        cerr << "Char_to_SuperBlock出现错误: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* Char_to_DiskInode 将char*转换为DiskInode*
* 参数：ch* 要转换的字符
* 返回值：DiskInode* 转换后的DiskInode指针
***************************************************************/
DiskInode* Char_to_DiskInode(char* ch)
{
    try
    {
        DiskInode* objPtr = reinterpret_cast<DiskInode*>(ch);
        return objPtr;
    }
    catch (exception& e)
    {
        cerr << "Char_to_DiskInode出现错误: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* Directory_to_Char 将Directory*转换为char*
* 参数：dir 要转换的Directory指针
* 返回值：char* 转换后的字符
***************************************************************/
char* Directory_to_Char(Directory* dir)
{
    try
    {
        char* ch = reinterpret_cast<char*>(dir);
        return ch;
    }
    catch (exception& e)
    {
        cerr << "Directory_to_Char出现错误: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* UserTable_to_Char 将UserTable*转换为char*
* 参数：usertable 要转换的UserTable指针
* 返回值：char* 转换后的字符
***************************************************************/
char* UserTable_to_Char(UserTable* usertable)
{
    try
    {
        /*char* ch = reinterpret_cast<char*>(usertable);
        return ch;*/
        if (usertable == nullptr) {
            return nullptr;
        }

        // 获取对象的大小
        size_t size = sizeof(UserTable);

        // 分配内存
        char* result = new char[size];

        // 将对象的内存内容复制到字节数组
        std::memcpy(result, usertable, size);

        return result;
    }
    catch (exception& e)
    {
        cerr << "UserTable_to_Char出现错误: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* Char_to_UserTable 将char*转换为UserTable*
* 参数：ch 要转换的char指针
* 返回值：UserTable* 转换后的UserTable结构
***************************************************************/
UserTable* Char_to_UserTable(char* ch)
{
    try
    {
        UserTable* objPtr = reinterpret_cast<UserTable*>(ch);
        return objPtr;
    }
    catch (exception& e)
    {
        cerr << "Char_to_UserTable出现错误: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* Char_to_Directory 将char*转换为Directory*
* 参数：ch 要转换的字符
* 返回值：Directory* 转换后的Directory指针
***************************************************************/
Directory* Char_to_Directory(char* ch)
{
    try
    {
        Directory* objPtr = reinterpret_cast<Directory*>(ch);
        return objPtr;
    }
    catch (exception& e)
    {
        cerr << "Char_to_Directory出现错误: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* IntArray_to_Char 将Int数组转换为char*
* 参数：arr int数组指针
* 返回值：char* 转换后的char指针
***************************************************************/
char* IntArray_to_Char(int* arr, int len)
{
    try
    {
        char* ch = new char[len * sizeof(int)];
        for (int i = 0; i < len; i++)
        {
            memcpy(ch, &(arr[i]), sizeof(int));
            ch += sizeof(int);
        }
        ch -= sizeof(int) * len;
        return ch;
    }
    catch (exception& e)
    {
        cerr << "IntArray_to_Char出现错误: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* FileMode_to_String 将文件权限转化为6位标识，没有x执行权限
* 参数：mode 文件权限
* 返回值：permissionString
***************************************************************/
string FileMode_to_String(unsigned short mode)
{
    /*
        OWNER_R = 0x400,
        OWNER_W = 0x200,
        OWNER_X = 0x100,
        GROUP_R = 0x40,
        GROUP_W = 0x20,
        GROUP_X = 0x10,
        OTHER_R = 0x4,
        OTHER_W = 0x2,
        OTHER_X = 0x1,*/
    std::string permissionString;

    // 所有者权限
    permissionString += (mode & Inode::OWNER_R) ? "r" : "-";
    permissionString += (mode & Inode::OWNER_W) ? "w" : "-";

    // 组权限
    permissionString += (mode & Inode::GROUP_R) ? "r" : "-";
    permissionString += (mode & Inode::GROUP_W) ? "w" : "-";

    // 其他用户权限
    permissionString += (mode & Inode::OTHER_R) ? "r" : "-";
    permissionString += (mode & Inode::OTHER_W) ? "w" : "-";

    return permissionString;
}


/**************************************************************
* Timestamp_to_String 将时间戳转化为字符串
* 参数：timestamp 时间戳
* 返回值：string 时间字符串
***************************************************************/
string Timestamp_to_String(unsigned int timestamp)
{
    time_t t = static_cast<time_t>(timestamp);
    tm* timeInfo = localtime(&t);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeInfo);

    return string(buffer);
}

/**************************************************************
* stringSplit 将字符串按照分隔符分割
* 参数：strIn  要分割的字符串 delim  分隔符
* 返回值：vector<string> 分割后的字符串数组
***************************************************************/
vector<string> stringSplit(const string& strIn, char delim)
{
    char* str = new char[strIn.size() + 1];
    strcpy(str, strIn.c_str());
    str[strIn.size()] = '\0';
    string s;
    s.append(1, delim);
    vector<string> res;
    char* splitted = strtok(str, s.c_str());
    while (splitted != NULL)
    {
        res.push_back(string(splitted));
        splitted = strtok(NULL, s.c_str());
    }
    return res;
}

