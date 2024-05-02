#include "header.h"
#include "utils.h"

extern FileSystem fs;

// SuperBlock构造函数
SuperBlock::SuperBlock()
{
	// 暂时不需要内容
}

// SuperBlock析构函数
SuperBlock::~SuperBlock() 
{
	// 暂时不需要内容
}

/**************************************************************
* Init初始化SuperBlock
* 参数：
* 返回值：
***************************************************************/
void SuperBlock::Init()
{
    this->s_isize = NUM_DISKINODE / NUM_INODE_PER_BLOCK;

    this->s_fsize = NUM_BLOCK_ALL;
    this->s_ninode = 0;
    for (int i = NUM_FREE_INODE - 1; i >= 0; i--)
        this->s_inode[this->s_ninode++] = i + 1; // 序号反着装入，从位置最低的inode开始使用
 
    // inode从1开始计数
    // superblock一开始管理第1组的100个盘块（NUM_FREE_BLOCK_GROUP=100）
    // 成组链接,从地址最大开始进行链接
    // SuperBlock管理位置最小的盘块
    int end = NUM_BLOCK_ALL;
    int start = POSITION_BLOCK;
    int numgroup = (end - start + 1) / NUM_FREE_BLOCK_GROUP + 1; // 一共82组
    // 由于第一组是99个，所以一开始s_nfree会加1，一开始管理59个盘块
    this->s_nfree = end - start - (numgroup - 1) * NUM_FREE_BLOCK_GROUP + 1; // s_nfree=59

    BufferManager* bufManager = fs.GetBufferManager();
    Buf* bp;

    // 开始分配，总共分配的盘块号[start,end]，并先给superblock分配最后1组
    for (int i = 0; i < this->s_nfree; i++) 
        this->s_free[i] = this->s_nfree + start - i - 1;// 反着写让位置小的盘块先分

    //  给组内第一个盘块写入数据，分配的盘块号[starti,endi)
    int iblk = start + this->s_nfree - 1;                    // 组内第一个盘块号
    int istart_addr = iblk * SIZE_BLOCK;                     // 组内第一个盘块的地址
    int inum = NUM_FREE_BLOCK_GROUP;                         // 上一组分了100块
    int starti = start + this->s_nfree;                      // 上一组开始的盘块号
    int endi = start + this->s_nfree + NUM_FREE_BLOCK_GROUP; // 上一组结束的盘块号
    for (int i = 2; i <= numgroup; i++)                      // 从2开始，因为最后一组已经给SuperBlock了
    {
        // 每次新建一个保证都是清零的
        int* stack = new int[inum + 1] {0}; // 第一位是链接的上一组的盘块个数
        int j = starti;
        stack[0] = inum;   // 第一位是链接的上一组的盘块个数
        if (i == numgroup) // 第一组盘块的第一个内容是0，标志结束
        {
            stack[1] = 0;
            j++;
        }
        for (; j < endi; j++) // 循环写入链接的上一组盘块号
            stack[j - starti + 1] = j;
        // 将组内第一个盘块写入磁盘
        bp = bufManager->GetBlk(iblk);
        // 由于写入字节数是sizeof(int)*(100+1)=404, 所以只需要一个盘块
        memcpy(bp->b_addr, stack, (inum + 1) * sizeof(int));
        bufManager->Bwrite(bp);

        // 更新各个参数
        iblk = stack[1];
        istart_addr = iblk * SIZE_BLOCK;
        inum = (i != (numgroup - 1)) ? NUM_FREE_BLOCK_GROUP : (NUM_FREE_BLOCK_GROUP - 1); // 第一组是99个（NUM_FREE_BLOCK_GROUP - 1）
        starti = endi;
        endi = endi + inum;

        delete[] stack;
    }
}