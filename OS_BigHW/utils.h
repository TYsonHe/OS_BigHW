#include "header.h"

// ���ߺ���
// ��Ҫ��Ϊ�˶�д��ת��
// ʵ�ָ������ݽṹ��ת��ΪChar���ͺô洢
Directory* Char_to_Directory(char* ch);

DiskInode* Char_to_DiskInode(char* ch);

SuperBlock* Char_to_SuperBlock(char* ch);

// UserTable* char2UserTable(char* ch);
UserTable* Char_to_UserTable(char* ch);

char* Directory_to_Char(Directory* dir);

char* IntArray_to_Char(int* arr, int len);

char* UserTable_to_Char(UserTable* user);

char* SuperBlk_to_Char(SuperBlock* spb);

// ʵ���ַ����ָ�
vector<string> stringSplit(const string& strIn, char delim);

string Timestamp_to_String(unsigned int timestamp);

string FileMode_to_String(unsigned short mode);