#include "header.h"
#include "utils.h"


// BufferManager���캯��
BufferManager::BufferManager()
{
    // ���ɻ�����п��ƿ飬���Ǳ���û�ж�Ӧ�Ļ���������
    // ��ʼ����������������������һ�����ɻ������
    this->bFreeList.b_forw = &m_Buf[NUM_BUF - 1];
    this->bFreeList.b_back = &m_Buf[0];
    this->bFreeList.av_forw = &m_Buf[0];
    this->bFreeList.av_back = &m_Buf[NUM_BUF - 1];

    // ���ӻ�����ƿ�������ÿһ�����棬ʵ��˫����������
    for (auto i = 0; i < NUM_BUF; i++)
    {
        this->m_Buf[i].b_addr = this->Buffer[i]; // �󶨿��ƿ���������ݿ�

        // ǰ���ڵ�
        // this->m_Buf[i].b_forw = (i - 1 >= 0) ? (&m_Buf[i - 1]) : (&bFreeList);
        // ��̽ڵ�
        // this->m_Buf[i].b_back = (i + 1 < NUM_BUF) ? (&m_Buf[i + 1]) : (&bFreeList);
        // ����˫�������ǰ���ͺ��
        if (i == 0) {
            m_Buf[i].b_forw = &bFreeList; // ��һ����ǰ��ָ�� bFreeList
        }
        else {
            m_Buf[i].b_forw = &m_Buf[i - 1]; // ������ǰ��ָ��ǰһ��
        }

        if (i == NUM_BUF - 1) {
            m_Buf[i].b_back = &bFreeList; // ���һ���ĺ��ָ�� bFreeList
        }
        else {
            m_Buf[i].b_back = &m_Buf[i + 1]; // �����ĺ��ָ���һ��
        }
        
        // ��һ�����л�����ƿ��ָ��
        // this->m_Buf[i].av_forw = (i + 1 < NUM_BUF) ? (&m_Buf[i + 1]) : (&bFreeList);
        // ��һ�����л�����ƿ��ָ��
        // this->m_Buf[i].av_back = (i - 1 >= 0) ? (&m_Buf[i - 1]) : (&bFreeList);


        // ���ÿ����б��ǰ���ͺ��
        // ���ϵذ��µķŵ�bFreeList��ͷ�������Ƿ�������
        if (i == 0) {
            m_Buf[i].av_back = &bFreeList; // ��һ���ĺ��ָ�� bFreeList
        }
        else {
            m_Buf[i].av_back = &m_Buf[i - 1]; // �����ĺ��ָ��ǰһ��
        }

        if (i == NUM_BUF - 1) {
            m_Buf[i].av_forw = &bFreeList; // ���һ����ǰ��ָ�� bFreeList
        }
        else {
            m_Buf[i].av_forw = &m_Buf[i + 1]; // ������ǰ��ָ���һ��
        }
    }

    // �����豸��
    // ֻ��һ���豸��ֱ�Ӹ�ֵ����
    this->devtab.b_forw = &devtab;
    this->devtab.b_back = &devtab;
    this->devtab.av_forw = &devtab;
    this->devtab.av_back = &devtab;
}

// ��������
BufferManager::~BufferManager() 
{
    // ��������
}

/**************************************************************
* GetBlk���뻺�棬���仺���
* ������blkno �߼����
* ����ֵ��Buf ���䵽�Ļ����
***************************************************************/
Buf* BufferManager::GetBlk(int blkno)
{
    Buf* bp = NULL;

    // ���豸����������blkno��ͬ�ߣ��������п����ظ����ã�
    for (bp = this->devtab.b_back; bp != &(this->devtab); bp = bp->b_back)
    {
        if (bp->b_blkno == blkno)
        {
            return bp;
        }
    }

    // ����δ����
    // �����ɶ�����Ѱ��
    // ���ɶ���Ϊ��
    if (this->bFreeList.av_forw == &this->bFreeList)
    {
        cerr << "�ڴ�ռ����������ɶ���Ϊ��" << endl;
        // һ�㲻�ᷢ������Ϊÿ�ζ����豸��д֮��������ͷ��ַ���
    }

    // ȡ�����ɶ��ж�ͷ
    bp = this->bFreeList.av_back;
    // �����ɶ���ȡ��������ָ��
    bp->av_forw->av_back = bp->av_back;
    bp->av_back->av_forw = bp->av_forw;
    // ��ԭ�豸���л�NODEV����ȡ��������ָ��
    bp->b_forw->b_back = bp->b_back;
    bp->b_back->b_forw = bp->b_forw;

    if (bp->b_flags & Buf::B_DELWRI)
        this->Bwrite(bp);// ������ַ�����ӳ�д��ǣ�����Ҫ���첽д��������

    bp->b_flags = Buf::B_NONE; // �ó�ʼ�����

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
    // ������豸�������ҵ����軺�棬��B_DONE��ʶ���Ͳ����ٶ�ȡһ��
    if (bp->b_flags & Buf::B_DONE)
    {
        return bp;
    }

    // û���ҵ���Ӧ����,���ж�����
    bp->b_flags |= Buf::B_READ;
    // ���豸������ж�β
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
    bp->b_flags = Buf::BufFlag::B_DONE;

    // �����ͷŹ���
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
* ������bp Ҫд�Ļ����
* ����ֵ��
***************************************************************/
void BufferManager::Bwrite(Buf* bp)
{
    // �Ƚ���������豸���ж�β
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
    // �ȶ���д
    // ֻ�޸Ļ��������ݣ�����д��SIZE_BUFFER��С
    fd.seekp(streampos(bp->b_blkno) * streampos(SIZE_BUFFER), ios::beg); // �ƶ�����Ӧ�Ŀ�λ��
    fd.write((const char*)bp->b_addr, SIZE_BUFFER);
    fd.close();
    // д�������
    bp->b_flags = Buf::BufFlag::B_DONE;

    // �����ͷŹ���
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

    // �����ͷż���
    // ��Ϊ�´�GetBlkȡ�����Bufʱ�����ӳ�д��ǻ���д��
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