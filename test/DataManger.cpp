#include"cloud_backup.hpp"

int main(int argc, char *argv)
{
	m_cloud_sys::DataManger data_manage("./text.txt");
	data_manage.InitLoad();//加载本地资源

	//获取所有信息
	std::vector<std::string> list;
	data_manage.GetAllName(&list);
	for(auto e: list)
	{
		std::cout << e.c_str() << std::endl;
	}
	std::cout << std::endl;

	//获取未压缩信息
	list.clear();
	data_manage.NonCompressList(&list);
	for(auto e: list)
	{
		std::cout << e.c_str() << std::endl;
	}

	//插入
	/*data_manage.Insert("a.txt", "a.txt");
	data_manage.Insert("b.txt", "b.txt.gz");
	data_manage.Insert("c.txt", "c.txt.gz");
	data_manage.Insert("d.txt", "d.txt");
	data_manage.Insert("f.txt", "e.txt.gz");*/

	/*data_manage.Storage();*///实例化存储

	return 0;
}

