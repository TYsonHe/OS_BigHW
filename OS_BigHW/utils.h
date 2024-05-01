#include "header.h"

// 工具函数
// 主要是为了读写的转换
// 实现各个数据结构的转换为Char类型好存储
Directory* Char_to_Directory(char* ch);

DiskInode* Char_to_DiskInode(char* ch);

SuperBlock* Char_to_SuperBlock(char* ch);

// UserTable* char2UserTable(char* ch);
UserTable* Char_to_UserTable(char* ch);

char* Directory_to_Char(Directory* dir);

char* IntArray_to_Char(int* arr, int len);

char* UserTable_to_Char(UserTable* user);

char* SuperBlk_to_Char(SuperBlock* spb);

// 实现字符串分割
vector<string> stringSplit(const string& strIn, char delim);

string Timestamp_to_String(unsigned int timestamp);

string FileMode_to_String(unsigned short mode);