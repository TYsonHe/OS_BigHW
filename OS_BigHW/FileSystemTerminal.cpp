#include "header.h"
#include "utils.h"

/**************************************************************
* login �û���¼
* ������
* ����ֵ��
***************************************************************/
void FileSystem::login()
{
    string name, pswd;
    short id;
    while (true)
    {
        cout << "�������û���:";
        getline(cin, name);
        cout << "����������:";
        getline(cin, pswd);
        if (name.empty() || pswd.empty())
        {
            cout << "����Ƿ�!" << endl;
            continue;
        }
        id = this->userTable->FindUser(name.c_str(), pswd.c_str());
        if (id == -1)
        {
            cout << "�û�������!" << endl;
            continue;
        }
        else
            break;
    }
    cout << "��½�ɹ�!" << endl
        << endl;
    this->curId = id;
    this->curName = name;
}