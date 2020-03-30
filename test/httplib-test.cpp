#include<iostream>
#include"../httplib.h"

using namespace std;

void helloword(const httplib::Request &req, httplib::Response &rsp)
{
	cout << "method: " << req.method << endl;
	cout << "path: " << req.path << endl;
	cout << "body: " << req.body << endl;

	rsp.status = 200;
	rsp.body = "<html><body><h1>helloworld</h1><body></html>";
	rsp.set_header("Content-Type", "text/html");
	sp.set_header("Content-Length", to_string(rsp.body.size()));
	return;
}

int main()
{
	httplib::Server server;
	server.Get("/", helloword);

	server.listen("0.0.0.0", 9000);

	return 0;
}


