#include "header.h"

Directory* char2Directory(char* ch);

DiskInode* Char_to_DiskInode(char* ch);

SuperBlock* char2SuperBlock(char* ch);

UserTable* char2UserTable(char* ch);

char* Directory_to_Char(Directory* dir);

char* uintArray2Char(int* arr, int len);

char* userTable2Char(UserTable* user);

char* SuperBlk_to_Char(SuperBlock* spb);

// ÊµÏÖ×Ö·û´®·Ö¸î
vector<string> stringSplit(const string& strIn, char delim);

string timestampToString(unsigned int timestamp);

string mode2String(unsigned short mode);