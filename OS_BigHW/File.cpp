#include "header.h"
#include "utils.h"

// File¹¹Ôìº¯Êý
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