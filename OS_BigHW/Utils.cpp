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
        cerr << "SuperBlk_to_Char���ִ���: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* Char_to_SuperBlock ��char*ת��ΪSuperBlock*
* ������char* Ҫת����char��ָ��
* ����ֵ��SuperBlock* ת�����SuperBlock�ṹ
***************************************************************/
SuperBlock* Char_to_SuperBlock(char* ch)
{
    try
    {
        SuperBlock* objPtr = reinterpret_cast<SuperBlock*>(ch);
        return objPtr;
    }
    catch (exception& e)
    {
        cerr << "Char_to_SuperBlock���ִ���: " << e.what() << endl;
        return NULL;
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
        cerr << "Char_to_DiskInode���ִ���: " << e.what() << endl;
        return NULL;
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
        cerr << "Directory_to_Char���ִ���: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* UserTable_to_Char ��UserTable*ת��Ϊchar*
* ������usertable Ҫת����UserTableָ��
* ����ֵ��char* ת������ַ�
***************************************************************/
char* UserTable_to_Char(UserTable* usertable)
{
    try
    {
        char* ch = reinterpret_cast<char*>(usertable);
        return ch;
    }
    catch (exception& e)
    {
        cerr << "UserTable_to_Char���ִ���: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* Char_to_UserTable ��char*ת��ΪUserTable*
* ������ch Ҫת����charָ��
* ����ֵ��UserTable* ת�����UserTable�ṹ
***************************************************************/
UserTable* Char_to_UserTable(char* ch)
{
    try
    {
        UserTable* objPtr = reinterpret_cast<UserTable*>(ch);
        return objPtr;
    }
    catch (exception& e)
    {
        cerr << "Char_to_UserTable���ִ���: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* Char_to_Directory ��char*ת��ΪDirectory*
* ������ch Ҫת�����ַ�
* ����ֵ��Directory* ת�����Directoryָ��
***************************************************************/
Directory* Char_to_Directory(char* ch)
{
    try
    {
        Directory* objPtr = reinterpret_cast<Directory*>(ch);
        return objPtr;
    }
    catch (exception& e)
    {
        cerr << "Char_to_Directory���ִ���: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* IntArray_to_Char ��Int����ת��Ϊchar*
* ������arr int����ָ��
* ����ֵ��char* ת�����charָ��
***************************************************************/
char* IntArray_to_Char(int* arr, int len)
{
    try
    {
        char* ch = new char[len * sizeof(int)];
        for (int i = 0; i < len; i++)
        {
            memcpy(ch, &(arr[i]), sizeof(int));
            ch += sizeof(int);
        }
        ch -= sizeof(int) * len;
        return ch;
    }
    catch (exception& e)
    {
        cerr << "IntArray_to_Char���ִ���: " << e.what() << endl;
        return NULL;
    }
}

/**************************************************************
* stringSplit ���ַ������շָ����ָ�
* ������strIn  Ҫ�ָ���ַ��� delim  �ָ���
* ����ֵ��vector<string> �ָ����ַ�������
***************************************************************/
vector<string> stringSplit(const string& strIn, char delim)
{
    char* str = new char[strIn.size() + 1];
    strcpy(str, strIn.c_str());
    str[strIn.size()] = '\0';
    string s;
    s.append(1, delim);
    vector<string> res;
    char* splitted = strtok(str, s.c_str());
    while (splitted != NULL)
    {
        res.push_back(string(splitted));
        splitted = strtok(NULL, s.c_str());
    }
    return res;
}