#include<thread>
#include"cloud_backup.hpp"

void m_non_compress()
{
	m_cloud_sys::NonHotCompress ncom(GZFILE_DIR, BACKUP_DIR);
	ncom.Start();
	return;
}

void thr_http_server()
{
	m_cloud_sys::Server server;
	server.Start();
	return ;
}

int main()
{
	//文件备份路径不存在则创建新的
	if(!boost::filesystem::exists(GZFILE_DIR))
	{
		boost::filesystem::create_directory(BACKUP_DIR);
	}
	//压缩包路径若不存在
	if(!boost::filesystem::exists(GZFILE_DIR))
	{
		boost::filesystem::create_directory(GZFILE_DIR);
	}

	//热点文件监测
	std::thread thr_compress(m_non_compress);
	//网络通信服务端启动(文件备份，解压，返回列表)
	std::thread thr_server(thr_http_server);
	thr_compress.join();
	thr_server.join();

	return 0;
}

