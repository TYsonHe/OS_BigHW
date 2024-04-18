#include "header.h"
#include "utils.h"

Buf::Buf()
{
    this->b_flags = BufFlag::B_NONE;
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
    // ÔÝÊ±Îª¿Õ
}