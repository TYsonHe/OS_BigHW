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
        cerr << "出现错误: " << e.what() << endl;
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
        cerr << "出现错误: " << e.what() << endl;
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
        cerr << "出现错误: " << e.what() << endl;
    }
}