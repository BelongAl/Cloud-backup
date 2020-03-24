#include"cloud_backup.hpp"


int main(int argc, char*argv[])
{
	//argv[1] = 源文件名称
	//argv[2] = 压缩包名称
	
	m_cloud_sys::CompressUtil::Compress(argv[1], argv[2]);//压缩
	std::string file =  argv[2];
	file += ".txt";
	m_cloud_sys::CompressUtil::UnCompress(argv[2], file.c_str());//解压

	return 0;
}
