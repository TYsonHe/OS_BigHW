#include "header.h"
#include "utils.h"

extern FileSystem fs;

// SuperBlock���캯��
SuperBlock::SuperBlock()
{
	// ��ʱ����Ҫ����
}

// SuperBlock��������
SuperBlock::~SuperBlock() 
{
	// ��ʱ����Ҫ����
}

/**************************************************************
* Init��ʼ��SuperBlock
* ������
* ����ֵ��
***************************************************************/
void SuperBlock::Init()
{
    this->s_isize = NUM_DISKINODE / NUM_INODE_PER_BLOCK;

    this->s_fsize = NUM_BLOCK_ALL;
    this->s_ninode = 0;
    for (int i = NUM_FREE_INODE - 1; i >= 0; i--)
        this->s_inode[this->s_ninode++] = i + 1; // ��ŷ���װ�룬��λ����͵�inode��ʼʹ��
 
    // inode��1��ʼ����
    // superblockһ��ʼ�����1���100���̿飨NUM_FREE_BLOCK_GROUP=100��
    // ��������,�ӵ�ַ���ʼ��������
    // SuperBlock����λ����С���̿�
    int end = NUM_BLOCK_ALL;
    int start = POSITION_BLOCK;
    int numgroup = (end - start + 1) / NUM_FREE_BLOCK_GROUP + 1; // һ��82��
    // ���ڵ�һ����99��������һ��ʼs_nfree���1��һ��ʼ����59���̿�
    this->s_nfree = end - start - (numgroup - 1) * NUM_FREE_BLOCK_GROUP + 1; // s_nfree=59

    BufferManager* bufManager = fs.GetBufferManager();
    Buf* bp;

    // ��ʼ���䣬�ܹ�������̿��[start,end]�����ȸ�superblock�������1��
    for (int i = 0; i < this->s_nfree; i++) 
        this->s_free[i] = this->s_nfree + start - i - 1;// ����д��λ��С���̿��ȷ�

    //  �����ڵ�һ���̿�д�����ݣ�������̿��[starti,endi)
    int iblk = start + this->s_nfree - 1;                    // ���ڵ�һ���̿��
    int istart_addr = iblk * SIZE_BLOCK;                     // ���ڵ�һ���̿�ĵ�ַ
    int inum = NUM_FREE_BLOCK_GROUP;                         // ��һ�����100��
    int starti = start + this->s_nfree;                      // ��һ�鿪ʼ���̿��
    int endi = start + this->s_nfree + NUM_FREE_BLOCK_GROUP; // ��һ��������̿��
    for (int i = 2; i <= numgroup; i++)                      // ��2��ʼ����Ϊ���һ���Ѿ���SuperBlock��
    {
        // ÿ���½�һ����֤���������
        int* stack = new int[inum + 1] {0}; // ��һλ�����ӵ���һ����̿����
        int j = starti;
        stack[0] = inum;   // ��һλ�����ӵ���һ����̿����
        if (i == numgroup) // ��һ���̿�ĵ�һ��������0����־����
        {
            stack[1] = 0;
            j++;
        }
        for (; j < endi; j++) // ѭ��д�����ӵ���һ���̿��
            stack[j - starti + 1] = j;
        // �����ڵ�һ���̿�д�����
        bp = bufManager->GetBlk(iblk);
        // ����д���ֽ�����sizeof(int)*(100+1)=404, ����ֻ��Ҫһ���̿�
        memcpy(bp->b_addr, stack, (inum + 1) * sizeof(int));
        bufManager->Bwrite(bp);

        // ���¸�������
        iblk = stack[1];
        istart_addr = iblk * SIZE_BLOCK;
        inum = (i != (numgroup - 1)) ? NUM_FREE_BLOCK_GROUP : (NUM_FREE_BLOCK_GROUP - 1); // ��һ����99����NUM_FREE_BLOCK_GROUP - 1��
        starti = endi;
        endi = endi + inum;

        delete[] stack;
    }
}