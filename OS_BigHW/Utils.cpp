#include "utils.h"

/**************************************************************
* SuperBlk_to_Char ��SuperBlock*ת��ΪChar*
* ������SuperBlock* Ҫת����SuperBlock��ָ��
* ����ֵ��Char* ת������ַ�
***************************************************************/
char* SuperBlk_to_Char(SuperBlock* spb) {
    try
    {
        char* chptr = new char[sizeof(SuperBlock) + 1];
        chptr[sizeof(SuperBlock)] = '\0';
        // reinterpret_cast���ݴ�һ�����͵�ת��Ϊ��һ�����͡�Ҳ����˵�������Զ����ƴ�����ʽ�����½���
        memcpy(chptr, reinterpret_cast<char*>(spb), sizeof(SuperBlock));
        return chptr;
    }
    catch (exception& e)
    {
        cerr << "���ִ���: " << e.what() << endl;
    }
}

/**************************************************************
* Char_to_DiskInode ��char*ת��ΪDiskInode*
* ������ch* Ҫת�����ַ�
* ����ֵ��DiskInode* ת�����DiskInodeָ��
***************************************************************/
DiskInode* Char_to_DiskInode(char* ch)
{
    try
    {
        DiskInode* objPtr = reinterpret_cast<DiskInode*>(ch);
        return objPtr;
    }
    catch (exception& e)
    {
        cerr << "���ִ���: " << e.what() << endl;
    }
}

/**************************************************************
* Directory_to_Char ��Directory*ת��Ϊchar*
* ������dir Ҫת����Directoryָ��
* ����ֵ��char* ת������ַ�
***************************************************************/
char* Directory_to_Char(Directory* dir)
{
    try
    {
        char* ch = reinterpret_cast<char*>(dir);
        return ch;
    }
    catch (exception& e)
    {
        cerr << "���ִ���: " << e.what() << endl;
    }
}