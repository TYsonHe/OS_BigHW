#include "header.h"
#include "utils.h"

// Directory构造函数
Directory::Directory()
{
    // 从Inode构建文件目录
    for (int i = 0; i < NUM_SUB_DIR; i++)
    {
        this->d_inodenumber[i] = 0;
        strcpy(this->d_filename[i], "");
    }
}

// Directory析构函数
Directory::~Directory()
{
	// 暂时为空
}

/**************************************************************
* mkdir 根据目录名name和Inode号inumber创建一个子目录
* 参数：name 要创建的目录名 inumber 要创建的目录Inode号
* 返回值：int 0表示成功,-1表示失败
***************************************************************/
int Directory::mkdir(const char* name, const int inumber)
{
    if (inumber < 0) // 没有Inode了
        return -1;
    bool isFull = true;   // 一开始先认为已满
    int idxEmpty = 0;     // 空闲的第一个位置
    for (int i = 0; i < NUM_SUB_DIR; i++)
    {
        // 如果找到对应子目录,说明目录已存在
        if (strcmp(this->d_filename[i], name) == 0)
        {
            cout << "目录已存在!" << endl;
            throw(EEXIST);
            return -1;
        }
        if (isFull && this->d_inodenumber[i] == 0)
        {
            // 找到空的位置
            isFull = false;
            idxEmpty = i;
        }
    }

    // 如果目录已满
    if (isFull)
    {
        cout << "目录已满!" << endl;
        throw(ENOSPC);
        return -1;
    }

    // 将子目录名和Inode号写入目录
    strcpy(this->d_filename[idxEmpty], name);
    this->d_inodenumber[idxEmpty] = inumber;
    return 0;
}

/**************************************************************
* rmi 清除目录中的第iloc个子目录
* 参数：iloc 子目录位置
* 返回值：
***************************************************************/
void Directory::rmi(int iloc)
{
    if (iloc < 0 || iloc >= NUM_SUB_DIR)
    {
        cerr << "iloc号不存在" << endl;
        return;
    }
    this->d_inodenumber[iloc] = 0;
    strcpy(this->d_filename[iloc], "");

    // 除了删除子目录，还要将后边的目录移到前面
    for (int i = iloc + 1; i < NUM_SUB_DIR; i++)
    {
        this->d_inodenumber[i - 1] = this->d_inodenumber[i];
        strcpy(this->d_filename[i - 1], this->d_filename[i]);
    }
    // 清空目录内容
    this->d_inodenumber[NUM_SUB_DIR - 1] = 0;
    strcpy(this->d_filename[NUM_SUB_DIR - 1], "");
}