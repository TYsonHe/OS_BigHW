#include "header.h"
#include "utils.h"

extern FileSystem fs;

// Inode构造函数
Inode::Inode()
{
    this->i_mode = 0;
    this->i_count = 0;
    this->i_nlink = 0;
    this->i_number = -1;
    this->i_uid = -1;
    this->i_gid = -1;
    this->i_size = 0;
    for (int i = 0; i < 10; i++)
    {
        this->i_addr[i] = 0;
    }
}

Inode::~Inode() {

}

/**************************************************************
* Clean 清空Inode内容
* 参数：
* 返回值：
***************************************************************/
void Inode::Clean()
{
    // 参考了Unix V6++ 源码
    this->i_mode = 0;
    this->i_count = 0;
    this->i_nlink = 0;
    this->i_number = -1;
    this->i_uid = -1;
    this->i_gid = -1;
    this->i_size = 0;
    for (int i = 0; i < 10; i++)
    {
        this->i_addr[i] = 0;
    }
}
/**************************************************************
* ICopy 根据缓存内容bp将外存Inode读取数据到内存Inode
* 参数：bp  缓冲区指针, inumber  外存Inode编号
* 返回值：
***************************************************************/
void Inode::ICopy(Buf* bp, int inumber)
{
    // 从外存Inode读取数据到内存Inode
    DiskInode* dp;

    int offset = ((inumber - 1) % NUM_INODE_PER_BLOCK) * sizeof(DiskInode);
    dp = Char_to_DiskInode(bp->b_addr + offset);

    this->i_mode = dp->d_mode;
    this->i_nlink = dp->d_nlink;
    this->i_uid = dp->d_uid;
    this->i_gid = dp->d_gid;
    this->i_size = dp->d_size;
    this->i_number = inumber;
    this->i_atime = dp->d_atime;
    this->i_mtime = dp->d_mtime;
    this->i_count = 1;
    for (int i = 0; i < NUM_I_ADDR; i++)
        this->i_addr[i] = dp->d_addr[i];
}
/**************************************************************
* WriteI 将内存Inode更新到外存中
* 参数：
* 返回值：
***************************************************************/
void Inode::WriteI()
{
    Buf* bp;
    BufferManager* bufMgr = fs.GetBufferManager();

    // 从磁盘读取磁盘Inode
    bp = bufMgr->Bread(POSITION_DISKINODE + (this->i_number - 1) / NUM_INODE_PER_BLOCK);
    int offset = ((this->i_number - 1) % NUM_INODE_PER_BLOCK) * sizeof(DiskInode);

    DiskInode* dp = new DiskInode;
    // 将内存Inode复制到磁盘Inode中
    dp->d_mode = this->i_mode;
    dp->d_nlink = this->i_nlink;
    dp->d_uid = this->i_uid;
    dp->d_gid = this->i_gid;
    dp->d_size = this->i_size;
    dp->d_atime = this->i_atime;
    dp->d_mtime = this->i_mtime;
    for (int i = 0; i < NUM_I_ADDR; i++)
        dp->d_addr[i] = this->i_addr[i];
    memcpy(bp->b_addr + offset, dp, sizeof(DiskInode));
    bufMgr->Bwrite(bp);
    delete dp;
}