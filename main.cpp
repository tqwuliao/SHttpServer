#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <map>
#include <fstream>
#include <time.h>

#include "HttpThread.h"


#pragma comment(lib,"ws2_32.lib")


#define POOL_SIZE 8

int main(int argc, char** argv) {
	for (int i = 0;i < argc;i++) {
		printf(argv[i]);
		printf("\n");
	}
	ThreadPool t_manager;
	for (int i = 0;i < POOL_SIZE;i++) {
		t_manager.push_back(new HttpThread());
	}
	const WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		return 0;
	}
	SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (slisten == INVALID_SOCKET) {
		printf("socket error\h");
		return 0;
	}

	sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(3030);
	sin.sin_addr.S_un.S_addr = INADDR_ANY;

	if (bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		printf("bind error !");
	}
	timeval timeout = { 3,0 };
	timeval timeout__ = { 0,0 };
	int timeout_ = 3000;
	//开始监听    
	if (listen(slisten, 5) == SOCKET_ERROR)
	{
		printf("listen error !");
		return 0;
	}

	//循环接收数据    
	
	sockaddr_in remoteAddr;
	int nAddrlen = sizeof(remoteAddr);
	char revData[BUFFER_SIZE+1];
	int length = 20;
	linger l = {1,0};
	unsigned long asyncSet = 1;
	while (true)
	{
		SOCKET sClient;
		
		sClient = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);
		setsockopt(sClient, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeval));
		setsockopt(sClient, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeval));
		int flag = setsockopt(sClient, SOL_SOCKET, SO_LINGER, (char*)&l, sizeof(l));

		if (sClient == INVALID_SOCKET)
		{
			continue;
		}
		
		
		{
			auto t_handler = (t_manager.top());
			t_handler->tailProt.lock();
			int p = t_handler->tail;
			
			t_handler->queue[p].socket = sClient;
			t_handler->tail++;
			t_handler->tailProt.unlock();
			t_handler->cv.notify_one();

			t_manager.update();
		}

	}

	closesocket(slisten);
	WSACleanup();
	for (auto i : t_manager) {
		delete i;
	}
	return 0;
}