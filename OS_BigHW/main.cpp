#include "header.h"
#include "utils.h"

FileSystem fs;

int main()
{
	// cout << sizeof(DiskInode);
	cout << "��ӭ����TysonFilesϵͳ!" << endl;
	cout << "��ʼroot�û�����root������Ҳ��root" << endl;
	// cout << "��ʼ��ͨ�û�����unix������Ҳ��1" << endl;
	cout << "�Լ����Կ���!" << endl
		<< endl;

	fstream fd;
	fd.open(DISK_PATH, ios::in);
	if (!fd.is_open())
	{
		printf("�ļ�ϵͳ�����ڣ����ڽ��г�ʼ��\n\n");
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
