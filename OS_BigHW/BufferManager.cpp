#include "header.h"
#include "utils.h"


// BufferManager构造函数
BufferManager::BufferManager()
{
    // 自由缓存队列控制块，但是本身没有对应的缓冲区数组
    // 初始化：整个缓存控制数组就是一个自由缓存队列
    this->bFreeList.b_forw = &m_Buf[NUM_BUF - 1];
    this->bFreeList.b_back = &m_Buf[0];
    this->bFreeList.av_forw = &m_Buf[0];
    this->bFreeList.av_back = &m_Buf[NUM_BUF - 1];

    // 链接缓存控制块数组中每一个缓存，实现双向链表链接
    for (auto i = 0; i < NUM_BUF; i++)
    {
        this->m_Buf[i].b_addr = this->Buffer[i]; // 绑定控制块和它的数据块

        // 前驱节点
        // this->m_Buf[i].b_forw = (i - 1 >= 0) ? (&m_Buf[i - 1]) : (&bFreeList);
        // 后继节点
        // this->m_Buf[i].b_back = (i + 1 < NUM_BUF) ? (&m_Buf[i + 1]) : (&bFreeList);
        // 设置双向链表的前驱和后继
        if (i == 0) {
            m_Buf[i].b_forw = &bFreeList; // 第一个的前驱指向 bFreeList
        }
        else {
            m_Buf[i].b_forw = &m_Buf[i - 1]; // 其他的前驱指向前一个
        }

        if (i == NUM_BUF - 1) {
            m_Buf[i].b_back = &bFreeList; // 最后一个的后继指向 bFreeList
        }
        else {
            m_Buf[i].b_back = &m_Buf[i + 1]; // 其他的后继指向后一个
        }
        
        // 上一个空闲缓存控制块的指针
        // this->m_Buf[i].av_forw = (i + 1 < NUM_BUF) ? (&m_Buf[i + 1]) : (&bFreeList);
        // 下一个空闲缓存控制块的指针
        // this->m_Buf[i].av_back = (i - 1 >= 0) ? (&m_Buf[i - 1]) : (&bFreeList);


        // 设置空闲列表的前驱和后继
        // 不断地把新的放到bFreeList队头，最终是反向链接
        if (i == 0) {
            m_Buf[i].av_back = &bFreeList; // 第一个的后继指向 bFreeList
        }
        else {
            m_Buf[i].av_back = &m_Buf[i - 1]; // 其他的后继指向前一个
        }

        if (i == NUM_BUF - 1) {
            m_Buf[i].av_forw = &bFreeList; // 最后一个的前驱指向 bFreeList
        }
        else {
            m_Buf[i].av_forw = &m_Buf[i + 1]; // 其他的前驱指向后一个
        }
    }

    // 磁盘设备表
    // 只有一个设备，直接赋值即可
    this->devtab.b_forw = &devtab;
    this->devtab.b_back = &devtab;
    this->devtab.av_forw = &devtab;
    this->devtab.av_back = &devtab;
}

// 析构函数
BufferManager::~BufferManager() 
{
    // 暂无内容
}

/**************************************************************
* GetBlk申请缓存，分配缓存块
* 参数：blkno 逻辑块号
* 返回值：Buf 分配到的缓存块
***************************************************************/
Buf* BufferManager::GetBlk(int blkno)
{
    Buf* bp = NULL;

    // 在设备队列中找与blkno相同者（缓存命中可以重复利用）
    for (bp = this->devtab.b_back; bp != &(this->devtab); bp = bp->b_back)
    {
        if (bp->b_blkno == blkno)
        {
            return bp;
        }
    }

    // 缓存未命中
    // 在自由队列中寻找
    // 自由队列为空
    if (this->bFreeList.av_forw == &this->bFreeList)
    {
        cerr << "内存空间已满，自由队列为空" << endl;
        // 一般不会发生，因为每次都是设备读写之后就立即释放字符块
    }

    // 取出自由队列队头
    bp = this->bFreeList.av_back;
    // 从自由队列取出，调整指针
    bp->av_forw->av_back = bp->av_back;
    bp->av_back->av_forw = bp->av_forw;
    // 从原设备队列或NODEV队列取出，调整指针
    bp->b_forw->b_back = bp->b_back;
    bp->b_back->b_forw = bp->b_forw;

    if (bp->b_flags & Buf::B_DELWRI)
        this->Bwrite(bp);// 如果该字符块带延迟写标记，就需要先异步写到磁盘上

    bp->b_flags = Buf::B_NONE; // 置初始化标记

    // 加入设备队列队头
    this->devtab.b_back->b_forw = bp;
    bp->b_back = this->devtab.b_back;
    bp->b_forw = &(this->devtab);
    this->devtab.b_back = bp;

    bp->b_blkno = blkno;
    return bp;
}

/**************************************************************
* Bread根据物理设备块号读取缓存
* 参数：blkno 所要进行读取的物理设备块号
* 返回值：Buf 返回读取到的缓存块
***************************************************************/
Buf* BufferManager::Bread(int blkno)
{
    Buf* bp;
    // 根据设备号，字符块号申请缓存
    bp = this->GetBlk(blkno);
    // 如果在设备队列中找到所需缓存，含B_DONE标识，就不需再读取一遍
    if (bp->b_flags & Buf::B_DONE)
    {
        return bp;
    }

    // 没有找到相应缓存,进行读操作
    bp->b_flags |= Buf::B_READ;
    // 放设备请求队列队尾
    this->devtab.av_forw->av_back = bp;
    bp->av_forw = this->devtab.av_forw;
    bp->av_back = &(this->devtab);
    this->devtab.av_forw = bp;

    // 开始读操作
    fstream fin;
    fin.open(DISK_PATH, ios::in | ios::binary);
    if (!fin.is_open())
    {
        cout << "无法打开一级磁盘文件myDisk.img" << endl;
        throw(ENOENT);
        return NULL;
    }
    fin.seekg(streampos(blkno) * streampos(SIZE_BUFFER), ios::beg);
    fin.read(bp->b_addr, SIZE_BUFFER);
    fin.close();

    // 读操作完成
    bp->b_flags = Buf::BufFlag::B_DONE;

    // 缓存释放过程
    // 从I/O请求队列取出
    bp->av_forw->av_back = bp->av_back;
    bp->av_back->av_forw = bp->av_forw;
    // 加入自由队列队尾
    bp->av_forw = this->bFreeList.av_forw;
    bp->av_back = &(this->bFreeList);
    bp->av_forw->av_back = bp;
    bp->av_back->av_forw = bp;

    return bp;
}

/**************************************************************
* Bwrite将缓存块bp写到磁盘上
* 参数：bp 要写的缓存块
* 返回值：
***************************************************************/
void BufferManager::Bwrite(Buf* bp)
{
    // 先将缓存放入设备队列队尾
    this->devtab.av_forw->av_back = bp;
    bp->av_forw = this->devtab.av_forw;
    bp->av_back = &(this->devtab);
    this->devtab.av_forw = bp;

    // 开始写操作
    bp->b_flags |= Buf::B_WRITE;
    fstream fd;
    fd.open(DISK_PATH, ios::in | ios::out | ios::binary);
    if (!fd.is_open())
    {
        cout << "无法打开一级磁盘文件myDisk.img" << endl;
        throw(ENOENT);
    }
    // 先读后写
    // 只修改缓存中内容，所以写入SIZE_BUFFER大小
    fd.seekp(streampos(bp->b_blkno) * streampos(SIZE_BUFFER), ios::beg); // 移动到对应的块位置
    fd.write((const char*)bp->b_addr, SIZE_BUFFER);
    fd.close();
    // 写操作完成
    bp->b_flags = Buf::BufFlag::B_DONE;

    // 缓存释放过程
    // 从I/O请求队列取出
    bp->av_forw->av_back = bp->av_back;
    bp->av_back->av_forw = bp->av_forw;
    // 加入自由队列
    bp->av_forw = (this->bFreeList).av_forw;
    bp->av_back = &(this->bFreeList);
    bp->av_forw->av_back = bp;
    bp->av_back->av_forw = bp;
}

/**************************************************************
* Bdwrite将缓存块bp延迟写到磁盘上
* 参数：bp 要写的缓存块
* 返回值：
***************************************************************/
void BufferManager::Bdwrite(Buf* bp)
{
    // 将缓存放入设备的I/O请求队列队尾
    this->devtab.av_forw->av_back = bp;
    bp->av_forw = this->devtab.av_forw;
    bp->av_back = &(this->devtab);
    this->devtab.av_forw = bp;

    // 标记为延迟写
    bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);

    // 缓存释放即可
    // 因为下次GetBlk取到这个Buf时，带延迟写标记会先写入
    // 从I/O请求队列取出
    bp->av_forw->av_back = bp->av_back;
    bp->av_back->av_forw = bp->av_forw;
    // 加入自由队列
    bp->av_forw = (this->bFreeList).av_forw;
    bp->av_back = &((this->bFreeList));
    bp->av_forw->av_back = bp;
    bp->av_back->av_forw = bp;
}

/**************************************************************
* Flush 将所有缓存写入磁盘
* 参数：
* 返回值：
***************************************************************/
void BufferManager::Flush()
{
    for (int i = 0; i < NUM_BUF; i++)
        if (this->m_Buf[i].b_flags & Buf::B_DELWRI)
            this->Bwrite(&this->m_Buf[i]);
}

/**************************************************************
* CleanBuf 清空某块缓存
* 参数：
* 返回值：
***************************************************************/
void BufferManager::CleanBuf(Buf* bp)
{
    int* pInt = (int*)bp->b_addr;
    for (unsigned int i = 0; i < SIZE_BUFFER / sizeof(int); i++)
        pInt[i] = 0;
    bp->b_wcount = 0;
}