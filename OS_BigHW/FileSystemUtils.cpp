#include "header.h"
#include "utils.h"

// FileSystem类的构造函数
FileSystem::FileSystem()
{
	// 暂时不需要内容
}

// FileSystem类析构函数
FileSystem::~FileSystem()
{
	/*
	this->exit();
	delete this->bufManager;
	delete this->spb;
	delete this->userTable;
	if (this->curDirInode != this->rootDirInode)
		delete this->rootDirInode;
	delete this->curDirInode;*/
}
 
/**************************************************************
* WriteSpb将SuperBlock写回磁盘
* 参数：
* 返回值：
***************************************************************/
void FileSystem::WriteSpb()
{
	char* p = SuperBlk_to_Char(this->spb);
	Buf* bp = this->bufManager->Bread(POSITION_SUPERBLOCK);
	memcpy(bp->b_addr, p, SIZE_BUFFER);
	this->bufManager->Bwrite(bp);
	bp = this->bufManager->Bread(POSITION_SUPERBLOCK + 1);
	// 这里之前有内存泄漏,没有填充的话这里一直会有问题
	memcpy(bp->b_addr, p + SIZE_BLOCK, SIZE_BLOCK);
	this->bufManager->Bwrite(bp);
}

/**************************************************************
* IAlloc 分配一个空闲的外存Inode
* 参数：
* 返回值：Inode* 返回分配到的内存Inode，如果分配失败，返回NULL
***************************************************************/
Inode* FileSystem::IAlloc()
{
    Buf* pBuf;
    Inode* pNode;
    int ino = 0; // 分配到的空闲外存Inode编号

    // SuperBlock直接管理的空闲Inode索引表已空
    // 注入新的空闲Inode索引表
    if (this->spb->s_ninode <= 0)
    {
        // 依次读入磁盘Inode区中的磁盘块，搜索其中空闲外存Inode，记入空闲Inode索引表
        for (int i = 0; i < this->spb->s_isize; i++)
        {
            pBuf = this->bufManager->Bread(POSITION_DISKINODE + i / NUM_INODE_PER_BLOCK);

            // 获取缓冲区首址
            int* p = (int*)pBuf->b_addr;

            // 检查该缓冲区中每个外存Inode的i_mode != 0，表示已经被占用
            for (int j = 0; j < NUM_INODE_PER_BLOCK; j++)
            {
                ino++;
                int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

                // 该外存Inode已被占用，不能记入空闲Inode索引表
                if (mode != 0)
                {
                    continue;
                }

                /*
                 * 如果外存inode的i_mode==0，此时并不能确定
                 * 该inode是空闲的，因为有可能是内存inode没有写到
                 * 磁盘上,所以要继续搜索内存inode中是否有相应的项
                 * 从源码中得到的注释
                 */
                if (this->IsLoaded(ino) == -1)
                {
                    // 该外存Inode没有对应的内存拷贝，将其记入空闲Inode索引表
                    this->spb->s_inode[this->spb->s_ninode++] = ino;

                    // 如果空闲索引表已经装满，则不继续搜索
                    if (this->spb->s_ninode >= 100)
                        break;
                }
            }

            // 如果空闲索引表已经装满，则不继续搜索
            if (this->spb->s_ninode >= 100)
                break;
        }
    }

    // 如果这样了还没有可用外存Inode，返回NULL
    if (this->spb->s_ninode <= 0)
    {
        cout << "磁盘上外存Inode区已满!" << endl;
        throw(ENOSPC);
        return NULL;
    }

    // 现在从外存分配内存Inode
    int inumber = this->spb->s_inode[--this->spb->s_ninode];
    pNode = IGet(inumber);
    if (NULL == pNode) // 不做修改操作
        return NULL;

    if (0 == pNode->i_mode)
    {
        pNode->Clean();
        return pNode;
    }
    return pNode;
}


/**************************************************************
* IsLoaded 判断指定外存Inode是否已经加载到内存中
* 参数：inumber 外存Inode编号
* 返回值：如果已经加载，返回内存Inode在inodeTable的编号，否则返回-1
***************************************************************/
int FileSystem::IsLoaded(int inumber)
{
    // 寻找指定外存Inode的内存inode拷贝
    for (int i = 0; i < NUM_INODE; i++)
        if (this->inodeTable[i].i_count != 0 && this->inodeTable[i].i_number == inumber)
            return i;

    return -1;
}
/**************************************************************
* IGet 从外存读取指定外存Inode到内存中
* 参数：inumber 外存Inode编号
* 返回值：Inode* 内存Inode拷贝
***************************************************************/
Inode* FileSystem::IGet(int inumber)
{
    Inode* pInode = NULL;
    // 在inodeTable中查找指定外存Inode的内存inode拷贝
    int isInTable = this->IsLoaded(inumber);

    // 如果找到了，直接返回内存inode拷贝，引用计数加1
    if (isInTable != -1)
    {
        pInode = &this->inodeTable[isInTable];
        pInode->i_count++;
        pInode->i_atime = unsigned int(time(NULL));
        return pInode;
    }

    // 没有找到，先从内存Inode节点表中分配一个Inode,再从外存读取
    for (int i = 0; i < NUM_INODE; i++)
        // 如果该内存Inode引用计数为零，则该Inode表示空闲，可以使用
        if (this->inodeTable[i].i_count == 0)
        {
            pInode = &(this->inodeTable[i]);
            break;
        }

    // 如果内存InodeTable已满，抛出异常
    if (pInode == NULL)
    {
        cout << "内存InodeTable已满" << endl;
        throw(ENFILE);
        return pInode;
    }

    // 如果内存InodeTabl没满，从外存读取指定外存Inode到内存中
    pInode->i_number = inumber;
    pInode->i_count++;
    pInode->i_atime = unsigned int(time(NULL));

    // 将该外存Inode读入缓冲区
    Buf* pBuf = this->bufManager->Bread(POSITION_DISKINODE + (inumber - 1) / NUM_INODE_PER_BLOCK);
    // 将缓冲区中的外存Inode信息拷贝到新分配的内存Inode中
    pInode->ICopy(pBuf, inumber);
    return pInode;
}