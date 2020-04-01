#pragma once

#include<iostream>
#include<sstream>
#include<vector>
#include<string>
#include<fstream>
#include<unordered_map>

#include<boost/filesystem.hpp>
#include<boost/algorithm/string.hpp>//splitc
#include"httplib.h"

//数据管理模块

class FileUtil//文件操作模块
{
public:
	//从文件中读取内容
	static bool Read(const std::string &name, std::string *body)
	{
		std::ifstream fs(name, std::ios::binary);//输入文件流，以二进制文件打开
		if (fs.is_open() == false)//判断文件是否打开成功
		{
			std::cout << "open file " << name << "failed!\n";
			return false;
		}

		int64_t fsize = boost::filesystem::file_size(name);//获取文件大小
		body->resize(fsize);//设置缓冲区大小
		fs.read(&(*body)[0], fsize);//将文件中的内容读取到缓冲区中
		if (fs.good() == false)//判断读取是否成功//判断上一次操作是否成功
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
		if (ofs.is_open() == false)//判断文件是否写成功
		{
			std::cout << "open file " << name << "failed!\n";
			return true;
		}
		ofs.write(&body[0], body.size());//将缓冲区中的内容写入到文件中
		if (ofs.good() == false)
		{
			std::cout << "file " << name << "write data failed!\n";
		}
		ofs.close();//关闭文件操作句柄
		return true;
	}
};

class DataManger
{
public:
	DataManger(const std::string &filename) :
		m_store_file(filename)
	{}

	bool Insert(const std::string &filename, const std::string &val)//插入和更新数据
	{
		m_backup_list[filename] = val;
		Storage();
		return true;
	}

	bool GetEtag(const std::string &filename, std::string *val)//获取etag信息
	{
		auto it = m_backup_list.find(filename);
		if (it == m_backup_list.end())
		{
			return false;
		}
		*val = it->second;
		return true;
	}

	bool Storage()//持久化存储
	{
		//序列化，持久化存储
		std::cout << "持久化存储" << std::endl;
		std::stringstream tmp;//实例化一个string流对象
		for (auto it = m_backup_list.begin(); it !=  m_backup_list.end(); ++it)
		{
			tmp << it->first << " " << it->second << "\r\n";
		}
		FileUtil::Write(m_store_file, tmp.str());//写入
		return true;
	}

	bool InitLoad()//启动时初始化加载原有数据
	{
		std::cout << "初始化数据" << std::endl;
		//1:讲备份文件读取出来
		std::string body;
		if (FileUtil::Read(m_store_file, &body) == false)
		{
			return false;
		}
		//2：使用boost库，对字符串进行处理，按照\r\n进行分割
		std::vector<std::string> list;

		boost::split(list, body, boost::is_any_of("\r\n"), boost::token_compress_off);
		//3:每一行按照空格进行分割
		for (auto i : list)
		{
			size_t pos = i.find(" ");
			if (pos == std::string::npos)
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

	std::string m_store_file;//持久化存储文件名称
	std::unordered_map<std::string, std::string> m_backup_list;//备份信息
};


//备份监测模块
class CloudClient
{
public:
	CloudClient(const std::string &filename, const std::string store_file,
				const std::string &srv_ip, const uint16_t srv_port):
		m_listen_list(filename),
		m_data_manage(store_file),
		m_srv_ip(srv_ip),
		m_srv_port(srv_port)
	{}

	bool Start()//完成整体的文件备份流程
	{	 
		m_data_manage.InitLoad();
		while (1)
		{
			std::vector<std::string> list;
			GetBuckupFileListen(&list);//获取到所有需要备份的文件名
			for (int i = 0; i < list.size(); i++)
			{
				std::string name = list[i];
				std::string pathname = m_listen_list + name;
				std::cout << pathname << ": is need to backup\n";
				std::string body;
				FileUtil::Read(pathname, &body);//读取数据作为正文
				httplib::Client client(m_srv_ip, m_srv_port);
				std::string req_path = "/" + name;//请求路径
				auto rsp = client.Put(req_path.c_str(), body, "application/ocete-stream");
				if (rsp == NULL || rsp->status != 200)
				{
					//文件上传失败
					std::cout << pathname << ": backup failed\n";
					continue;
				}
				//备份成功，更新信息
				std::string etag;
				GetEtag(pathname, &etag);
				m_data_manage.Insert(name, etag);

				std::cout << pathname << ": backup success\n";

			}
			Sleep(1000);//休眠一秒继续
		}
		return true;
	}

	bool GetBuckupFileListen(std::vector<std::string> *list)//获取需要备份的文件列表
	{
		if (boost::filesystem::exists(m_listen_list) == false)
		{
			boost::filesystem::create_directory(m_listen_list);//若目录不存在就创建
		}
		boost::filesystem::directory_iterator begin(m_listen_list);
		boost::filesystem::directory_iterator end;
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{
				continue;//目录不进行备份
			}
			std::string pathname = begin->path().string();
			std::string name = begin->path().filename().string();
			std::string cur_etag;//当前etag
			GetEtag(pathname, &cur_etag);
			std::string old_stag;//原有etag
			m_data_manage.GetEtag(name, &old_stag);
			if (cur_etag != old_stag)
			{
				list->push_back(name);//备份
			}
		}

		return true;
	}
	static bool GetEtag(const std::string &pathname, std::string *etag)//计算etag信息
	{
		//计算一个etag（文件大小和文件最后一次的修改时间）
		int64_t fsize = boost::filesystem::file_size(pathname);//文件大小
		time_t mtime = boost::filesystem::last_write_time(pathname); //文件最后一次修改时间
		*etag = std::to_string(fsize) + "-" + std::to_string(mtime);//最好使用MD5
		
		return true;
	}

private:
	
	std::string m_srv_ip;
	uint16_t m_srv_port;
	DataManger m_data_manage;
	std::string m_listen_list;
};