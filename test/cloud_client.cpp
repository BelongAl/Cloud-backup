#include"cloud_client.hpp"


#define STORE_FILE "./list_backup"//���ݹ���Ŀ¼
#define LISTEN_DIR "./backup"//�ļ����Ŀ¼
#define SERVER_IP "49.233.166.208"
#define SERVER_PORT 9000

int main()
{
	CloudClient client(LISTEN_DIR, STORE_FILE, SERVER_IP, SERVER_PORT);
	client.Start();
	return 0;
}