#include "header.h"
#include "utils.h"

Buf::Buf()
{
    // b_flags��b_wcount��b_blkno�ڻ��湹��ʱ��ֵ
    this->b_flags = BufFlag::B_NONE; // ��ʼ��ΪNONE
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
    // ��ʱΪ��
}