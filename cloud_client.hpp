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

//���ݹ���ģ��

class FileUtil//�ļ�����ģ��
{
public:
	//���ļ��ж�ȡ����
	static bool Read(const std::string &name, std::string *body)
	{
		std::ifstream fs(name, std::ios::binary);//�����ļ������Զ������ļ���
		if (fs.is_open() == false)//�ж��ļ��Ƿ�򿪳ɹ�
		{
			std::cout << "open file " << name << "failed!\n";
			return false;
		}

		int64_t fsize = boost::filesystem::file_size(name);//��ȡ�ļ���С
		body->resize(fsize);//���û�������С
		fs.read(&(*body)[0], fsize);//���ļ��е����ݶ�ȡ����������
		if (fs.good() == false)//�ж϶�ȡ�Ƿ�ɹ�//�ж���һ�β����Ƿ�ɹ�
		{
			std::cout << "file " << name << "read data failed!\n";
			return false;
		}
		fs.close();//�ر��ļ��������
		return true;
	}
	//���ļ���д������
	static bool Write(const std::string &name, std::string body)
	{
		//ofstream,Ĭ�ϴ��ļ�ʱ�������ԭ������;
		std::ofstream ofs(name, std::ios::binary);//����ļ������Զ����ƴ��ļ�
		if (ofs.is_open() == false)//�ж��ļ��Ƿ�д�ɹ�
		{
			std::cout << "open file " << name << "failed!\n";
			return true;
		}
		ofs.write(&body[0], body.size());//���������е�����д�뵽�ļ���
		if (ofs.good() == false)
		{
			std::cout << "file " << name << "write data failed!\n";
		}
		ofs.close();//�ر��ļ��������
		return true;
	}
};

class DataManger
{
public:
	DataManger(const std::string &filename) :
		m_store_file(filename)
	{}

	bool Insert(const std::string &filename, const std::string &val)//����͸�������
	{
		m_backup_list[filename] = val;
		Storage();
		return true;
	}

	bool GetEtag(const std::string &filename, std::string *val)//��ȡetag��Ϣ
	{
		auto it = m_backup_list.find(filename);
		if (it == m_backup_list.end())
		{
			return false;
		}
		*val = it->second;
		return true;
	}

	bool Storage()//�־û��洢
	{
		//���л����־û��洢
		std::cout << "�־û��洢" << std::endl;
		std::stringstream tmp;//ʵ����һ��string������
		for (auto it = m_backup_list.begin(); it !=  m_backup_list.end(); ++it)
		{
			tmp << it->first << " " << it->second << "\r\n";
		}
		FileUtil::Write(m_store_file, tmp.str());//д��
		return true;
	}

	bool InitLoad()//����ʱ��ʼ������ԭ������
	{
		std::cout << "��ʼ������" << std::endl;
		//1:�������ļ���ȡ����
		std::string body;
		if (FileUtil::Read(m_store_file, &body) == false)
		{
			return false;
		}
		//2��ʹ��boost�⣬���ַ������д�������\r\n���зָ�
		std::vector<std::string> list;

		boost::split(list, body, boost::is_any_of("\r\n"), boost::token_compress_off);
		//3:ÿһ�а��տո���зָ�
		for (auto i : list)
		{
			size_t pos = i.find(" ");
			if (pos == std::string::npos)
			{
				continue;
			}
			std::string key = i.substr(0, pos);
			std::string val = i.substr(pos + 1);
			//4����key/val��ӵ�m_file_list��
			Insert(key, val);
		}
		return true;
	}

private:	

	std::string m_store_file;//�־û��洢�ļ�����
	std::unordered_map<std::string, std::string> m_backup_list;//������Ϣ
};


//���ݼ��ģ��
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

	bool Start()//���������ļ���������
	{	 
		m_data_manage.InitLoad();
		while (1)
		{
			std::vector<std::string> list;
			GetBuckupFileListen(&list);//��ȡ��������Ҫ���ݵ��ļ���
			for (int i = 0; i < list.size(); i++)
			{
				std::string name = list[i];
				std::string pathname = m_listen_list + name;
				std::cout << pathname << ": is need to backup\n";
				std::string body;
				FileUtil::Read(pathname, &body);//��ȡ������Ϊ����
				httplib::Client client(m_srv_ip, m_srv_port);
				std::string req_path = "/" + name;//����·��
				auto rsp = client.Put(req_path.c_str(), body, "application/ocete-stream");
				if (rsp == NULL || rsp->status != 200)
				{
					//�ļ��ϴ�ʧ��
					std::cout << pathname << ": backup failed\n";
					continue;
				}
				//���ݳɹ���������Ϣ
				std::string etag;
				GetEtag(pathname, &etag);
				m_data_manage.Insert(name, etag);

				std::cout << pathname << ": backup success\n";

			}
			Sleep(1000);//����һ�����
		}
		return true;
	}

	bool GetBuckupFileListen(std::vector<std::string> *list)//��ȡ��Ҫ���ݵ��ļ��б�
	{
		if (boost::filesystem::exists(m_listen_list) == false)
		{
			boost::filesystem::create_directory(m_listen_list);//��Ŀ¼�����ھʹ���
		}
		boost::filesystem::directory_iterator begin(m_listen_list);
		boost::filesystem::directory_iterator end;
		for (; begin != end; ++begin)
		{
			if (boost::filesystem::is_directory(begin->status()))
			{
				continue;//Ŀ¼�����б���
			}
			std::string pathname = begin->path().string();
			std::string name = begin->path().filename().string();
			std::string cur_etag;//��ǰetag
			GetEtag(pathname, &cur_etag);
			std::string old_stag;//ԭ��etag
			m_data_manage.GetEtag(name, &old_stag);
			if (cur_etag != old_stag)
			{
				list->push_back(name);//����
			}
		}

		return true;
	}
	static bool GetEtag(const std::string &pathname, std::string *etag)//����etag��Ϣ
	{
		//����һ��etag���ļ���С���ļ����һ�ε��޸�ʱ�䣩
		int64_t fsize = boost::filesystem::file_size(pathname);//�ļ���С
		time_t mtime = boost::filesystem::last_write_time(pathname); //�ļ����һ���޸�ʱ��
		*etag = std::to_string(fsize) + "-" + std::to_string(mtime);//���ʹ��MD5
		
		return true;
	}

private:
	
	std::string m_srv_ip;
	uint16_t m_srv_port;
	DataManger m_data_manage;
	std::string m_listen_list;
};