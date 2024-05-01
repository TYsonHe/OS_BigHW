#include "header.h"
#include "utils.h"

Buf::Buf()
{
    // b_flags、b_wcount、b_blkno在缓存构造时赋值
    this->b_flags = BufFlag::B_NONE; // 初始化为NONE
    this->av_back = NULL;
    this->av_forw = NULL;
    this->b_forw = NULL;
    this->b_back = NULL;

    this->b_wcount = -1;
    this->b_blkno = -1;
    this->b_addr = NULL;
}

Buf::~Buf()
{
    // 暂时为空
}