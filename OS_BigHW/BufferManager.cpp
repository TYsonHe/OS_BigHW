#include "header.h"
#include "utils.h"


// BufferManager���캯��
BufferManager::BufferManager()
{
    // ���ɻ�����п��ƿ飬ʵ��˫�����������ӣ����Ǳ���û�ж�Ӧ�Ļ���������
    // һ��ʼ������������������һ�����ɻ������
    this->bFreeList.b_forw = &m_Buf[NUM_BUF - 1];
    this->bFreeList.b_back = &m_Buf[0];
    this->bFreeList.av_forw = &m_Buf[0];
    this->bFreeList.av_back = &m_Buf[NUM_BUF - 1];

    // �Ի�����ƿ�������ÿһ������ʵ�����ӣ�b_flags��b_wcount��b_blkno�ڻ��湹��ʱ���ѹ���
    for (auto i = 0; i < NUM_BUF; i++)
    {
        this->m_Buf[i].b_addr = this->Buffer[i];

        // ǰ���ڵ�
        this->m_Buf[i].b_forw = (i - 1 >= 0) ? (&m_Buf[i - 1]) : (&bFreeList);
        // ��̽ڵ�
        this->m_Buf[i].b_back = (i + 1 < NUM_BUF) ? (&m_Buf[i + 1]) : (&bFreeList);
        // ���ϵذ��µķŵ�bFreeList��ͷ�������Ƿ�������
        // ��һ�����л�����ƿ��ָ��
        this->m_Buf[i].av_forw = (i + 1 < NUM_BUF) ? (&m_Buf[i + 1]) : (&bFreeList);
        // ��һ�����л�����ƿ��ָ��
        this->m_Buf[i].av_back = (i - 1 >= 0) ? (&m_Buf[i - 1]) : (&bFreeList);
    }

    // �����豸��
    this->devtab.b_forw = &devtab;
    this->devtab.b_back = &devtab;
    this->devtab.av_forw = &devtab;
    this->devtab.av_back = &devtab;
}

// ��������
BufferManager::~BufferManager() 
{

}

/**************************************************************
* GetBlk���뻺�棬���仺���
* ������blkno �߼����
* ����ֵ��Buf ���䵽�Ļ����
***************************************************************/
Buf* BufferManager::GetBlk(int blkno)
{
    Buf* bp = NULL;

    // ���豸����������blkno��ͬ�ߣ�ֱ������
    for (bp = this->devtab.b_back; bp != &(this->devtab); bp = bp->b_back)
    {
        if (bp->b_blkno == blkno)
        {
            return bp;
        }
    }

    // �����ɶ�����Ѱ��
    // ���ɶ���Ϊ��
    if (this->bFreeList.av_forw == &this->bFreeList)
    {
        cerr << "�ڴ�ռ����������ɶ���Ϊ��" << endl;
        // ��̫���ܣ���Ϊÿ�ζ����豸��д֮��������ͷ��ַ���
    }

    // ȡ�����ɶ��ж�ͷ
    bp = this->bFreeList.av_back;
    // �����ɶ���ȡ��
    bp->av_forw->av_back = bp->av_back;
    bp->av_back->av_forw = bp->av_forw;
    // ��ԭ�豸���л�NODEV����ȡ��
    bp->b_forw->b_back = bp->b_back;
    bp->b_back->b_forw = bp->b_forw;

    // ������ַ������ӳ�д�������첽д��������
    if (bp->b_flags & Buf::B_DELWRI)
        this->Bwrite(bp);

    bp->b_flags = Buf::B_NONE;

    // �����豸���ж�ͷ
    this->devtab.b_back->b_forw = bp;
    bp->b_back = this->devtab.b_back;
    bp->b_forw = &(this->devtab);
    this->devtab.b_back = bp;

    bp->b_blkno = blkno;
    return bp;
}

/**************************************************************
* Bread���������豸��Ŷ�ȡ����
* ������blkno ��Ҫ���ж�ȡ�������豸���
* ����ֵ��Buf ���ض�ȡ���Ļ����
***************************************************************/
Buf* BufferManager::Bread(int blkno)
{
    Buf* bp;
    // �����豸�ţ��ַ�������뻺��
    bp = this->GetBlk(blkno);
    // ������豸�������ҵ����軺�棬��B_DONE�����ã��Ͳ������I/O����
    if (bp->b_flags & Buf::B_DONE)
    {
        return bp;
    }

    // û���ҵ���Ӧ����,I/O���������豸������ж�β
    bp->b_flags |= Buf::B_READ;
    this->devtab.av_forw->av_back = bp;
    bp->av_forw = this->devtab.av_forw;
    bp->av_back = &(this->devtab);
    this->devtab.av_forw = bp;

    // ��ʼ������
    fstream fin;
    fin.open(DISK_PATH, ios::in | ios::binary);
    if (!fin.is_open())
    {
        cout << "�޷���һ�������ļ�myDisk.img" << endl;
        throw(ENOENT);
        return NULL;
    }
    fin.seekg(streampos(blkno) * streampos(SIZE_BUFFER), ios::beg);
    fin.read(bp->b_addr, SIZE_BUFFER);
    fin.close();

    // ���������
    // ���±�־
    bp->b_flags = Buf::BufFlag::B_DONE;

    // ��I/O�������ȡ��
    bp->av_forw->av_back = bp->av_back;
    bp->av_back->av_forw = bp->av_forw;
    // �������ɶ��ж�β
    bp->av_forw = this->bFreeList.av_forw;
    bp->av_back = &(this->bFreeList);
    bp->av_forw->av_back = bp;
    bp->av_back->av_forw = bp;

    return bp;
}

/**************************************************************
* Bwrite�������bpд��������
* ������
* bp Ҫд�Ļ����
* ����ֵ��
***************************************************************/
void BufferManager::Bwrite(Buf* bp)
{
    // ����������豸���ж�β
    this->devtab.av_forw->av_back = bp;
    bp->av_forw = this->devtab.av_forw;
    bp->av_back = &(this->devtab);
    this->devtab.av_forw = bp;

    // ��ʼд����
    bp->b_flags |= Buf::B_WRITE;
    fstream fd;
    fd.open(DISK_PATH, ios::in | ios::out | ios::binary);
    if (!fd.is_open())
    {
        cout << "�޷���һ�������ļ�myDisk.img" << endl;
        throw(ENOENT);
    }
    // ���ȶ��̿飬��д�̿�������ݣ�ֻ�޸Ļ��������ݣ�������SIZE_BUFFER��д
    fd.seekp(streampos(bp->b_blkno) * streampos(SIZE_BUFFER), ios::beg);
    fd.write((const char*)bp->b_addr, SIZE_BUFFER);
    fd.close();
    // д�������

    // ���±�־
    bp->b_flags = Buf::BufFlag::B_DONE;
    // ��I/O�������ȡ��
    bp->av_forw->av_back = bp->av_back;
    bp->av_back->av_forw = bp->av_forw;
    // �������ɶ���
    bp->av_forw = (this->bFreeList).av_forw;
    bp->av_back = &(this->bFreeList);
    bp->av_forw->av_back = bp;
    bp->av_back->av_forw = bp;
}

/**************************************************************
* Bdwrite�������bp�ӳ�д��������
* ������bp Ҫд�Ļ����
* ����ֵ��
***************************************************************/
void BufferManager::Bdwrite(Buf* bp)
{
    // ����������豸��I/O������ж�β
    this->devtab.av_forw->av_back = bp;
    bp->av_forw = this->devtab.av_forw;
    bp->av_back = &(this->devtab);
    this->devtab.av_forw = bp;

    // ���Ϊ�ӳ�д
    bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);

    // ��I/O�������ȡ��
    bp->av_forw->av_back = bp->av_back;
    bp->av_back->av_forw = bp->av_forw;
    // �������ɶ���
    bp->av_forw = (this->bFreeList).av_forw;
    bp->av_back = &((this->bFreeList));
    bp->av_forw->av_back = bp;
    bp->av_back->av_forw = bp;
}

/**************************************************************
* Flush �����л���д�����
* ������
* ����ֵ��
***************************************************************/
void BufferManager::Flush()
{
    for (int i = 0; i < NUM_BUF; i++)
        if (this->m_Buf[i].b_flags & Buf::B_DELWRI)
            this->Bwrite(&this->m_Buf[i]);
}

/**************************************************************
* CleanBuf ���ĳ�黺��
* ������
* ����ֵ��
***************************************************************/
void BufferManager::CleanBuf(Buf* bp)
{
    int* pInt = (int*)bp->b_addr;
    for (unsigned int i = 0; i < SIZE_BUFFER / sizeof(int); i++)
        pInt[i] = 0;
    bp->b_wcount = 0;
}