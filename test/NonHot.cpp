#include<thread>
#include"cloud_backup.hpp"

void m_non_compress()
{
	m_cloud_sys::NonHotCompress ncom(GZFILE_DIR, BACKUP_DIR);
	ncom.Start();
	return;
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
	m_cloud_sys::data_manger.Insert("test.cpp", "test.cpp");
	std::thread thr(m_non_compress);
	thr.join();

	return 0;
}

