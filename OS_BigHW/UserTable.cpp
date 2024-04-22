#include "header.h"
#include "utils.h"


// UserTable�๹�캯��
UserTable::UserTable()
{
	for (int i = 0; i < NUM_USER; i++)
	{
		this->u_id[i] = -1;
		this->u_gid[i] = -1;
		strcpy(this->u_name[i], "");
		strcpy(this->u_password[i], "");
	}
}

UserTable::~UserTable()
{

}
/**************************************************************
* AddRoot ����Root�û�
* ������
* ����ֵ��
***************************************************************/
void UserTable::AddRoot() {
	this->u_id[0] = ROOT_ID;
	this->u_gid[0] = ROOT_GID;
	strcpy(this->u_name[0], "root");
	strcpy(this->u_password[0], "root");
}

/**************************************************************
* GetGId �����û�id��ȡ�û�������id
* ������id  �û�id
* ����ֵ�������û�������id
***************************************************************/
short UserTable::GetGId(const short id)
{
	for (int i = 0; i < NUM_USER; i++)
		if (this->u_id[i] == id)
			return this->u_gid[i];
	return -1;
}

/**************************************************************
* FindUser �����û�
* ������name �û��� password ����
* ����ֵ�� �����û�id	���û���ҵ�����-1
***************************************************************/
short UserTable::FindUser(const char* name, const char* password)
{
	for (int i = 0; i < NUM_USER; i++)
		if (strcmp(this->u_name[i], name) == 0 && strcmp(this->u_password[i], password) == 0)
			return this->u_id[i];
	return -1;

}