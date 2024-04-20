#include "header.h"
#include "utils.h"

// File构造函数
File::File()
{
    this->f_inode = NULL;
    this->f_offset = 0;
    this->f_uid = -1;
    this->f_gid = -1;
}

File::~File()
{

}

/**************************************************************
* Clean 清空文件结构
* 参数：
* 返回值：
***************************************************************/
void File::Clean()
{
    this->f_inode = NULL;
    this->f_offset = 0;
    this->f_uid = -1;
    this->f_gid = -1;
}