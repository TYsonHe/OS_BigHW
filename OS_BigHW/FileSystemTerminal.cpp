#include "header.h"
#include "utils.h"

/**************************************************************
* login 用户登录
* 参数：
* 返回值：
***************************************************************/
void FileSystem::login()
{
    string name, pswd;
    short id;
    while (true)
    {
        cout << "请输入用户名:";
        getline(cin, name);
        cout << "请输入密码:";
        getline(cin, pswd);
        if (name.empty() || pswd.empty())
        {
            cout << "输入非法!" << endl;
            continue;
        }
        id = this->userTable->FindUser(name.c_str(), pswd.c_str());
        if (id == -1)
        {
            cout << "用户不存在!" << endl;
            continue;
        }
        else
            break;
    }
    cout << "登陆成功!" << endl
        << endl;
    this->curId = id;
    this->curName = name;
}