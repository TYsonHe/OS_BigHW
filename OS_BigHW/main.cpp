#include "header.h"
#include "utils.h"

FileSystem fs;

int main()
{
	// cout << sizeof(DiskInode) << endl;
	// cout << sizeof(Inode) << endl;
	cout << "欢迎来到TysonFiles系统!" << endl;
	cout << "初始root用户名root，密码root" << endl;
	// cerr << "错误测试" << endl;
	cout<< endl;

	fstream fd;
	fd.open(DISK_PATH, ios::in);
	if (!fd.is_open())
	{
		printf("文件系统不存在，正在进行初始化\n");
		fs.fformat();
		fs.exit();
	}
	else
	{
		cout << "文件系统已存在，正在加载" << endl;
		fs.init();
	}

	fs.run();

	return 0;
}
