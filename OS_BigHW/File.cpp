#include "header.h"
#include "utils.h"

// File���캯��
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