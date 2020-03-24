#include<cstdio>
#include<string>
#include<vector>
#include<fstream>
#include<unordered_map>
#include<zlib.h>//压缩
#include<pthread.h>
#include<iostream>
#include<boost/filesystem.hpp>

#include"httplib.h"//http


namespace m_cloud_sys
{
	class FileUtil//文件操作模块
	{
	public:
		//从文件中读取内容
		static bool Read(const std::string &name, std::string *body )
		{
			std::ifstream fs(name, std::ios::binary);//输入文件流，以二进制文件打开
			if(fs.is_open() == false)//判断文件是否打开成功
			{
				std::cout << "open file " << name << "failed!\n";
				return false;
			}
			int64_t fsize = boost::filesystem::file_size(name);//获取文件大小
			body->resize(fsize);//设置缓冲区大小
			fs.read(&(*body)[0], fsize);//将文件中的内容读取到缓冲区中
			if(fs.good() == false)//判断读取是否成功//判断上一次操作是否成功
			{
				std::cout << "file " << name << "read data failed!\n";
				return false;
			}
			fs.close();//关闭文件操作句柄
			return true;
		}
		//向文件中写入数据
		static bool Write(const std::string &name, std::string &body)
		{
			//ofstream,默认打开文件时，会清空原有内容;
			std::ofstream ofs(name, std::ios::binary);//输出文件流，以二进制打开文件
			if(ofs.is_open() == false)//判断文件是否写成功
			{
				std::cout << "open file " << name << "failed!\n";
				return true;
			}
			ofs.write(&body[0], body.size());//将缓冲区中的内容写入到文件中
			if(ofs.good() == false)
			{
				std::cout << "file " << name << "write data failed!\n";
			}
			ofs.close();//关闭文件操作句柄
			return true;
		}
	};

	class CompressUtil//压缩解压缩模块
	{
	public:
		//文件压缩函数，源文件名称-压缩包名称
		static bool Compress(const std::string &src, const std::string &dst)
		{
			std::string body;//定义一个缓冲区
			FileUtil::Read(src, &body);//将源文件中断数据读入缓冲区中

			gzFile gf = gzopen(dst.c_str(), "wb");//打开只写压缩包
			if(gf == NULL)//判断压缩包是否打开成功
			{
				std::cout << "open file " << dst << "failed!\n";
				return false;
			}
			int wlen = 0;//记录已经压缩的数据
			while(wlen < body.size())//判断是否压缩完毕
			{
				int ret = gzwrite(gf, &body[wlen], body.size() - wlen);//压缩数据，（操作句柄， 压缩的位置， 需压缩文件大小）
				if(ret == 0)
				{
					std::cout << "file" << dst << "weite compress data failed!\n"; 
					return false;
				}
				wlen += ret;
			}
			gzclose(gf);//关闭压缩包
			return true;
		}
		//文件解压缩。压缩包名称-源文件名称
		static bool UnCompress(const std::string &src, const char *dst)
		{
			std::ofstream ofs(dst, std::ios::binary);//打开解压后的文件
			if(ofs.is_open() == false) //判断解压后的文件是否打开成功
			{
				std::cout << "open file" << dst << "failed!\n";
				return false;
			}
			gzFile gf = gzopen(src.c_str(), "rb");//以只读的方式打开压缩包
			if(gf == NULL)//判断压缩包是否打开成功
			{
				std::cout << "open file" << src << "failed!\n";
				ofs.close();
				return false;
			}

			char tmp[4096] = {0};//定义缓冲区
			int ret = 0;//定义返回值
			while((ret = gzread(gf, tmp, 4096)) > 0 )//从压缩包中读取文件（操作句柄，未压缩缓冲区， 缓冲区大小）
			{
				ofs.write(tmp, ret);//将缓冲区中的数据写入文件中；
			}

			ofs.close();//关闭文件句柄
			gzclose(gf);//关闭压缩文件句柄
			return true;
		}
	};

	class DataManger//数据管理模块
	{
	public:
		DataManger(const std::string &path):
			m_back_file(path)
		{}
		~DataManger()
		{}

		bool Exists(const std::string &name);//判断文件是否存在
		bool IsCompress(const std::string &name);//判断文件是否已经压缩
		bool NonCompressList(std::vector<std::string> *list);//获取未压缩文件列表
		bool Insert(const std::string src, const std::string &std);//插入更新数据
		bool GetAllName(std::vector<std::string> *list);//获取所有文件名称
		bool Storage();//数据改变后持久化存储
		bool InitLoad();//启动时初始化加载原有数据

	private:
		std::string m_back_file;//持久化数据存储文件名称
		std::unordered_map<std::string, std::string> m_file_list;//数据管理容器
		pthread_rwlock_t m_rwlock;//读写锁
	};

	class NonHotCompress
	{
	public:
		NonHotCompress()
		{}
		~NonHotCompress()
		{}
			
		bool Start();//总体向外提供的功能接口，开始压缩模块
	private:
		bool FileIsHot(const std::string &name);//判断一个文件是否是一个热点文件	

	private:
		std::string m_gz_dir;//压缩文件存储路径

	};

	class Server
	{
	public:
		Server()
		{}
		~Server()
		{}

		bool Start();//网络通信开启接口

	private:
		//文件上传处理回调函数
		static void UpLoad(const httplib::Request &req, httplib::Response &rsp);
		//文件列表处理回调函数
		static void List(const httplib::Request &req, httplib::Request &rsp);
		//文件下载处理回调函数
		static void DowanLoad(const httplib::Request &req, httplib::Response &rsp);

	private:
		std::string m_file_dir;//文件上传辈份路径
		httplib::Server m_server;
	};

}
