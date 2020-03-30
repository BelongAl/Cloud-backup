#include<cstdio>
#include<string>
#include<vector>
#include<fstream>
#include<unordered_map>
#include<zlib.h>//压缩
#include<pthread.h>
#include<iostream>
#include<boost/filesystem.hpp>
#include<boost/algorithm/string.hpp>
#include<unistd.h>

#include"httplib.h"//http

#define NONHOT_TIME 10 //最后一次访问时间在10s以内的
#define INTERVAL_TIME 30 //非热店文件检测，每30s一次
#define BACKUP_DIR "./backup/" //文件备份目录
#define GZFILE_DIR "./gzfile/" //压缩后的文件目录
#define DATA_FILE "./list.backup" //数据管理模块备份路径

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
		static bool Write(const std::string &name, std::string body)
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
		DataManger(const std::string  &path):
			m_back_file(path)
		{
			pthread_rwlock_init(&m_rwlock, NULL);
		}

		~DataManger()
		{
			pthread_rwlock_destroy(&m_rwlock);
		}

		bool Exists(const std::string &name)//判断文件是否存在
		{
			pthread_rwlock_rdlock(&m_rwlock);//加读锁
			if(m_file_list.find(name) == m_file_list.end())
			{
				pthread_rwlock_unlock(&m_rwlock);
				return false;
			}
			pthread_rwlock_unlock(&m_rwlock);
			return true;
		}
		bool IsCompress(const std::string &name)//判断文件是否已经压缩
		{
			//判断文件是否存在
			pthread_rwlock_rdlock(&m_rwlock);//加读锁
			auto it = m_file_list.find(name);
			if(it == m_file_list.end())
			{
				pthread_rwlock_unlock(&m_rwlock);
				return false;
			}
			//查看是否压缩
			if(it->first == it->second)
			{
				pthread_rwlock_unlock(&m_rwlock);
				return false;
			}
			pthread_rwlock_unlock(&m_rwlock);
			return true;

		}
		bool NonCompressList(std::vector<std::string> *list)//获取未压缩文件列表
		{
			pthread_rwlock_rdlock(&m_rwlock);//加读锁
			for(auto it = m_file_list.begin(); it != m_file_list.end(); it++)
			{
				if(it->first == it->second)
				{
					list->push_back(it->first);
				}
			}
			pthread_rwlock_unlock(&m_rwlock);
			return true;
		}
		bool Insert(const std::string src, const std::string &dst)//插入更新数据
		{	
			pthread_rwlock_wrlock(&m_rwlock);//加写锁子
			m_file_list[src] = dst;
			pthread_rwlock_unlock(&m_rwlock);
			Storage();//保存
			return true;
		}

		bool GetAllName(std::vector<std::string> *list)//获取所有文件名称
		{
			pthread_rwlock_rdlock(&m_rwlock);
			for(auto it = m_file_list.begin(); it != m_file_list.end(); ++it)
			{
				list->push_back(it->first);
			}
			pthread_rwlock_unlock(&m_rwlock);
			return true;
 		}
		bool Storage()//数据改变后持久化存储(对文件名)
		{
			std::stringstream tmp;//实例化一个string流对象
			pthread_rwlock_rdlock(&m_rwlock);
			for (auto it = m_file_list.begin(); it != m_file_list.end(); ++it)
			{
				tmp << it->first << " " << it->second << "\r\n";
			}
			pthread_rwlock_unlock(&m_rwlock);
			FileUtil::Write(m_back_file,tmp.str());//写入
			return true;	
		}
		bool InitLoad()//启动时初始化加载原有数据
		{
			//1:讲备份文件读取出来
			std::string body;
			if(FileUtil::Read(m_back_file, &body) == false)
			{
				return false;
			}
			//2：使用boost库，对字符串进行处理，按照\r\n进行分割
			std::vector<std::string> list;
			boost::split(list, body, boost::is_any_of("\r\n"), boost::token_compress_off);
			//3:每一行按照空格进行分割
			for(auto i: list)
			{
				size_t pos = i.find(" ");
				if(pos == std::string::npos)
				{
					continue;
				}
				std::string key = i.substr(0, pos);
				std::string val = i.substr(pos + 1);
				//4：把key/val添加到m_file_list中
				Insert(key, val);
			}
			return true;
		}


	private:
		std::string m_back_file;//持久化数据存储文件名称
		std::unordered_map<std::string, std::string> m_file_list;//数据管理容器
		pthread_rwlock_t m_rwlock;//读写锁
	};


	DataManger data_manger(DATA_FILE);//数据管理对象

	class NonHotCompress
	{
	public:
		NonHotCompress(const std::string dir_name, const std::string src_name):
			m_gz_dir(dir_name),
			m_src_name(src_name)
		{}
			
		bool Start()//总体向外提供的功能接口，开始压缩模块
		{
			//一个一直循环操作，对文件进行判断，压缩
			while(true)
			{
				//1：获取未压缩文件列表
				std::vector<std::string> list;
				data_manger.NonCompressList(&list);
				//2：判断是否为热点文件
				for(auto e: list)
				{
					//3：是否为非热点就压缩
					bool ret = FileIsHot(e);
					if(!ret)
					{
						std::cout << "---压缩文件： " << e.c_str() <<  "---" << std::endl;
						std::string src_name = BACKUP_DIR + e;//源文件名称
						std::string dst_name = GZFILE_DIR + e + ".gz";//压缩文件名称
						if(CompressUtil::Compress(src_name, dst_name))
						{
							data_manger.Insert(e, e+".gz");//更新数据信息
							unlink(src_name.c_str());//删除源文件(删除文件目录项)
						}
					}
				}
				//4：休眠
				sleep(INTERVAL_TIME);
			} 
			return true;
		}
	
	private:
		bool FileIsHot(const std::string &name)//判断一个文件是否是一个热点文件
		{
			time_t cur_t = time(NULL);//获取当前时间
			struct stat st;//文件信息结构体
			if(stat(name.c_str(), &st) < 0)//文件状态获取失败
			{	
				return false;
			}
			if((cur_t - st.st_atime) > NONHOT_TIME)
			{
				return false;//非热点文件
			}
			return true;//热点文件
		}

	private:
		std::string m_gz_dir;//压缩文件存储路径
		std::string m_src_name;//原文件路径
	};

	class Server
	{
	public:
		Server()
		{}
		~Server()
		{}

		bool Start()//网络通信开启接口
		{
			m_server.Put("/upload", UpLoad);//往http对象中添加处理路径和方法
			m_server.Get("/list", List);
			m_server.Get("/download(.*)",DowanLoad);//（。*）：正则表达式：(匹配任意字符，捕捉任意字符):(防止与list混淆)
			m_server.listen("0.0.0.0", 9000);//监听任意ip和443端口

			return true;
		}

	private:
		//文件上传处理回调函数
		static void UpLoad(const httplib::Request &req, httplib::Response &rsp)
		{
			rsp.status = 200;
			rsp.set_content("upload", 6, "text/html");
		}
		
		//文件列表处理回调函数
		static void List(const httplib::Request &req, httplib::Response &rsp)
		{
			rsp.status = 200;
			rsp.set_content("list", 4, "text/html");//正文数据，正文数据长度，正文类型
		}
		//文件下载处理回调函数
		static void DowanLoad(const httplib::Request &req, httplib::Response &rsp)
		{
			rsp.status = 200;
			std::string path = req.matches[1];//正则表达式捕捉的字符串//[0]为整体字符串
			rsp.set_content(path.c_str(), path.size(), "text/html");
		}

	private:
		std::string m_file_dir;//文件上传辈份路径
		httplib::Server m_server;
	};

}

