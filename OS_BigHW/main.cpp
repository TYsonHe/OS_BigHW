#include "header.h"
#include "utils.h"

FileSystem fs;

int main()
{
	// cout << sizeof(DiskInode) << endl;
	// cout << sizeof(Inode) << endl;
	cout << "��ӭ����TysonFilesϵͳ!" << endl;
	cout << "��ʼroot�û���root������root" << endl;
	// cerr << "�������" << endl;
	cout<< endl;

	fstream fd;
	fd.open(DISK_PATH, ios::in);
	if (!fd.is_open())
	{
		printf("�ļ�ϵͳ�����ڣ����ڽ��г�ʼ��\n");
		fs.fformat();
		fs.exit();
	}
	else
	{
		cout << "�ļ�ϵͳ�Ѵ��ڣ����ڼ���" << endl;
		fs.init();
	}

	fs.run();

	return 0;
}
