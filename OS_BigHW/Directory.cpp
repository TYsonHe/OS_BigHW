#include "header.h"
#include "utils.h"

// Directory���캯��
Directory::Directory()
{
    // ��Inode�����ļ�Ŀ¼
    for (int i = 0; i < NUM_SUB_DIR; i++)
    {
        this->d_inodenumber[i] = 0;
        strcpy(this->d_filename[i], "");
    }
}

// Directory��������
Directory::~Directory()
{
	// ��ʱΪ��
}

/**************************************************************
* mkdir ����Ŀ¼��name��Inode��inumber����һ����Ŀ¼
* ������name Ҫ������Ŀ¼�� inumber Ҫ������Ŀ¼Inode��
* ����ֵ��int 0��ʾ�ɹ�,-1��ʾʧ��
***************************************************************/
int Directory::mkdir(const char* name, const int inumber)
{
    if (inumber < 0) // û��Inode��
        return -1;
    bool isFull = true;   // һ��ʼ����Ϊ����
    int idxEmpty = 0;     // ���еĵ�һ��λ��
    for (int i = 0; i < NUM_SUB_DIR; i++)
    {
        // ����ҵ���Ӧ��Ŀ¼,˵��Ŀ¼�Ѵ���
        if (strcmp(this->d_filename[i], name) == 0)
        {
            cout << "Ŀ¼�Ѵ���!" << endl;
            throw(EEXIST);
            return -1;
        }
        if (isFull && this->d_inodenumber[i] == 0)
        {
            // �ҵ��յ�λ��
            isFull = false;
            idxEmpty = i;
        }
    }

    // ���Ŀ¼����
    if (isFull)
    {
        cout << "Ŀ¼����!" << endl;
        throw(ENOSPC);
        return -1;
    }

    // ����Ŀ¼����Inode��д��Ŀ¼
    strcpy(this->d_filename[idxEmpty], name);
    this->d_inodenumber[idxEmpty] = inumber;
    return 0;
}

/**************************************************************
* rmi ���Ŀ¼�еĵ�iloc����Ŀ¼
* ������iloc ��Ŀ¼λ��
* ����ֵ��
***************************************************************/
void Directory::rmi(int iloc)
{
    if (iloc < 0 || iloc >= NUM_SUB_DIR)
    {
        cerr << "iloc�Ų�����" << endl;
        return;
    }
    this->d_inodenumber[iloc] = 0;
    strcpy(this->d_filename[iloc], "");

    // ����ɾ����Ŀ¼����Ҫ����ߵ�Ŀ¼�Ƶ�ǰ��
    for (int i = iloc + 1; i < NUM_SUB_DIR; i++)
    {
        this->d_inodenumber[i - 1] = this->d_inodenumber[i];
        strcpy(this->d_filename[i - 1], this->d_filename[i]);
    }
    // ���Ŀ¼����
    this->d_inodenumber[NUM_SUB_DIR - 1] = 0;
    strcpy(this->d_filename[NUM_SUB_DIR - 1], "");
}