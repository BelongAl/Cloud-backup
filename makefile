compress:compress.cpp
	g++ -g -std=c++11 $^ -o $@ -lz -lpthread -lboost_filesystem -lboost_system
