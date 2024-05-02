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
    // 注意不能删i_count和i_number,不然初始化时会有bug
    // this->i_count = 0;
    // this->i_number = -1;
    this->i_mode = 0;
    this->i_nlink = 0;
    this->i_uid = -1;
    this->i_gid = -1;
    this->i_size = 0;
    for (int i = 0; i < 10; i++)
    {
        this->i_addr[i] = 0;
    }
}

/**************************************************************
* GetParentInumber 获取父目录inodenumber
* 参数：
* 返回值：int 返回父目录的inodenumber
***************************************************************/
int Inode::GetParentInumber()
{
    
    if (!(this->i_mode & Inode::INodeMode::IDIR))// 非目录文件
        return NULL;

    BufferManager* bufMgr = fs.GetBufferManager();

    // 先获取本目录的目录项，它存有父目录位置
    Buf* bp = bufMgr->Bread(this->i_addr[0]);
    Directory* dir = Char_to_Directory(bp->b_addr);

    return dir->d_inodenumber[1];
}

/**************************************************************
* ITrunc 释放数据盘块
* 参数：
* 返回值：
***************************************************************/
void Inode::ITrunc()
{
    BufferManager* bufMgr = fs.GetBufferManager();

    for (int i = 0; i < NUM_I_ADDR; i++)
    {
        if (this->i_addr[i] != 0)
        {
            if (i < NUM_BLOCK_IFILE) // 释放直接索引项
                fs.Free(this->i_addr[i]);
            else // 需要释放间接索引项
            {
                Buf* pFirstBuf = bufMgr->Bread(this->i_addr[i]);
                int* p = (int*)pFirstBuf->b_addr;
                for (int j = 0; j < NUM_FILE_INDEX; j++)
                {
                    if (p[j] != 0)
                        fs.Free(p[j]);
                }
                fs.Free(this->i_addr[i]);
            }
            this->i_addr[i] = 0;
        }
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

    DiskInode* dp = new DiskInode; // 申请内存
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
    delete dp;// 释放内存
}

/**************************************************************
* GetDir 获取目录内容
* 参数：
* 返回值：Directory* 返回目录指针
***************************************************************/
Directory* Inode::GetDir()
{
    // 非目录文件不能获取目录内容
    if (!(this->i_mode & Inode::INodeMode::IDIR))
        return NULL;

    BufferManager* bufMgr = fs.GetBufferManager();

    Buf* bp = bufMgr->Bread(this->i_addr[0]);
    Directory* dir = Char_to_Directory(bp->b_addr);

    return dir;
}

/**************************************************************
* GetModeString 根据用户id和gid获取文件权限字符串
* 参数： id 用户id gid 用户组id
* 返回值：permissionString 共6位，没有x权限
***************************************************************/
string Inode::GetModeString(int id, int gid)
{
    string permissionString;

    if (id == this->i_uid)
    {
        // 所有者权限
        permissionString += (this->i_mode & OWNER_R) ? "r" : "-";
        permissionString += (this->i_mode & OWNER_W) ? "w" : "-";
    }
    else if (gid == this->i_gid)
    {
        permissionString += (this->i_mode & GROUP_R) ? "r" : "-";
        permissionString += (this->i_mode & GROUP_W) ? "w" : "-";
    }
    else
    {
        permissionString += (this->i_mode & OTHER_R) ? "r" : "-";
        permissionString += (this->i_mode & OTHER_W) ? "w" : "-";
    }
    return permissionString;
}

/**************************************************************
* String_to_Mode 将文件权限字符串转换为mode
* 参数： mode 文件权限字符串
* 返回值：unsigned short 返回mode 错误返回-1
***************************************************************/
unsigned short Inode::String_to_Mode(string mode)
{
    if (mode.length() != 6)
        return -1;
    unsigned short modeNum = 0;
    if (mode[0] == 'r')
        modeNum |= OWNER_R;
    else if (mode[0] != '-')
        return -1;

    if (mode[1] == 'w')
        modeNum |= OWNER_W;
    else if (mode[1] != '-')
        return -1;

    if (mode[2] == 'r')
        modeNum |= GROUP_R;
    else if (mode[2] != '-')
        return -1;

    if (mode[3] == 'w')
        modeNum |= GROUP_W;
    else if (mode[3] != '-')
        return -1;

    if (mode[4] == 'r')
        modeNum |= OTHER_R;
    else if (mode[4] != '-')
        return -1;

    if (mode[5] == 'w')
        modeNum |= OTHER_W;
    else if (mode[5] != '-')
        return -1;

    return modeNum;
}

/**************************************************************
* AssignMode 根据规则给内存Inode赋予文件权限
* 参数： 
* 返回值：int 正确返回0，错误返回-1
***************************************************************/
int Inode::AssignMode(unsigned short mode)
{
    if (mode & Inode::IFILE || mode & Inode::IDIR || mode & Inode::ILARG)
        return -1;
    this->i_mode &= ~(OWNER_R | OWNER_W | GROUP_R | GROUP_W | OTHER_R | OTHER_W);
    this->i_mode |= mode & (OWNER_R | OWNER_W | GROUP_R | GROUP_W | OTHER_R | OTHER_W);
    return 0;
}

/**************************************************************
* Bmap 将逻辑块号lbn映射到物理盘块号phyBlkno
* 参数：lbn  逻辑块号lbn，是在i_addr[]中的索引idx
* 返回值：返回物理盘块号phyBlkno
***************************************************************/
int Inode::Bmap(int lbn)
{
    /******************************************************************************
     * 我设计的文件索引结构：
     * (1) 小型文件：i_addr[0] - i_addr[4]为直接索引表，文件长度范围是0 ~ 5个盘块
     * 对应字节数为[0 ~ 5*512]
     * (2) 大型文件：i_addr[5] - i_addr[9]为一次间接索引表所在磁盘块号，每磁盘块
     * 上存放128个(512/4)文件数据盘块号，此类文件长度范围是6 ~ (128 * 5 + 5)个盘块；
     * 对应字节数为[5*512+1 ~ (5+128*5)*512]
     ******************************************************************************/

    Buf* pFirstBuf, * pSecondBuf;
    int phyBlkno; // 转换后的物理盘块号
    BufferManager* bufMgr = fs.GetBufferManager();

    // 底下每一次的Alloc都会有Bdwrite
    if (lbn < NUM_BLOCK_IFILE)
    {
        phyBlkno = this->i_addr[lbn];
        if (phyBlkno == 0 && (pFirstBuf = fs.Alloc()) != NULL) // 虽然这里有Alloc，接下来就会有Bdwrite
        {
            bufMgr->Bdwrite(pFirstBuf);
            // 因为后面很可能马上还要用到此处新分配的数据块，所以不急于立刻输出到
            // 磁盘上；而是将缓存标记为延迟写方式，这样可以减少系统的I/O操作。
            phyBlkno = pFirstBuf->b_blkno;
            // 将逻辑块号lbn映射到物理盘块号phyBlkno
            this->i_addr[lbn] = phyBlkno;
        }
    }
    else
    {
        int index = (lbn - NUM_BLOCK_IFILE) / NUM_FILE_INDEX + NUM_BLOCK_IFILE;
        phyBlkno = this->i_addr[index];
        if (phyBlkno == 0)
        {
            this->i_mode |= Inode::ILARG;
            // 分配一空闲盘块存放间接索引表
            if ((pFirstBuf = fs.Alloc()) == NULL) // 这里有Alloc，但是接下来就会有Bdwrite
                return 0;
            // i_addr[index]中记录间接索引表的物理盘块号
            this->i_addr[index] = pFirstBuf->b_blkno;
        }
        else
        {
            // 读出存储间接索引表的字符块
            pFirstBuf = bufMgr->Bread(phyBlkno);
        }

        int* p = (int*)pFirstBuf->b_addr;

        // 计算在间接索引表中的偏移量
        index = (lbn - NUM_BLOCK_IFILE) % NUM_FILE_INDEX;

        if ((phyBlkno = p[index]) == 0 && (pSecondBuf = fs.Alloc()) != NULL)
        {
            // 将分配到的文件数据盘块号登记在一次间接索引表中
            phyBlkno = pSecondBuf->b_blkno;
            p[index] = phyBlkno;
            // 将数据盘块、更改后的一次间接索引表用延迟写方式输出到磁盘
            bufMgr->Bdwrite(pSecondBuf);
            bufMgr->Bdwrite(pFirstBuf);
        }
    }

    return phyBlkno;
}