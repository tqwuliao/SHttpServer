#pragma once
#include <thread>
#include <queue>
#include <map>
#include "Router.h"
#include <WinSock2.h>
#include <mutex>
#include <algorithm>
#include <condition_variable>

#define BUFFER_SIZE 255
#define MAX_QUEUE 32

template<typename T> 
inline void swap(T& a, T& b) {
	T c;
	c = a;
	a = b;
	b = c;
}

struct ServerTask {
	std::string data;
	SOCKET socket;
};

class HttpThread
{
public:
	ServerTask queue[MAX_QUEUE];
	int head, tail;
	std::mutex tailProt;
	std::condition_variable cv;
	std::thread thrd;
	enum Method {
		GET,
		POST
	} method;

	HttpThread() : 
		head(0),
		tail(0),
		thrd(ThreadFunc, queue, &head, &tail, std::ref(tailProt), std::ref(cv))
	{
		
		
	}

	bool operator<(const HttpThread& right) {
		return this->tail - this->head < right.tail - this->head;
	}

	static void ThreadFunc(ServerTask* q,int* head,int* tail,std::mutex& lock,
		std::condition_variable& cv) {
		std::unique_lock<std::mutex> lcx(lock);

		timeval timeout = { 3,0 };
		timeval immediate = { 0,0 };

		while (1) {

			while (*head < *tail) {
				SOCKET sClient = q[*head].socket;
				
				//接收数据    
				
				fd_set f;
				char revData[BUFFER_SIZE+1];
				int repeat = 0;
				
				FD_ZERO(&f);

				do {
					int ret = 1;
					std::string unparsed_data = "";
					for (repeat = 0;repeat < 10 && !FD_ISSET(sClient, &f);repeat++) {
						FD_ZERO(&f);
						FD_SET(sClient, &f);
						int t = select(0, &f, 0, 0, &immediate);
						if (t < 0) {
							repeat = 0;
							break;
						}
					}
					if (!FD_ISSET(sClient, &f)) {
						break;
					}
					repeat = 0;
					while (1)
					{
						ret = recv(sClient, revData, BUFFER_SIZE, 0);
						if (ret <= 0) break;
						revData[ret] = 0x00;
						unparsed_data += revData;
						if (ret < BUFFER_SIZE) break;
					}
					if (ret < 0) break;
					if (ret == 0 && FD_ISSET(sClient, &f)) {
						repeat = 0;
						break;
					}

					if (unparsed_data.empty()) {
						
						continue;
					}


					int start = 0;
					std::map<std::string, std::string> user_data;

					while (unparsed_data[start++] != ' ');
					unparsed_data[start - 1] = 0;
					user_data.insert(std::make_pair(std::string("method"), std::string(&unparsed_data[0])));
					Method method = (Method)(user_data["method"] == "POST");
					int last = start;

					while (unparsed_data[start++] != ' ');
					unparsed_data[start - 1] = 0;
					user_data.insert(std::make_pair(std::string("url"), std::string(&unparsed_data[last])));
					last = start;

					while (unparsed_data[start++] != '\n');
					unparsed_data[start - 1] = 0;
					user_data.insert(std::make_pair(std::string("version"), std::string(&unparsed_data[last])));
					last = start;

					std::string key, value;
					bool state = true;
					for (;unparsed_data[start];start++) {
						if (unparsed_data[start] == '\n') {
							unparsed_data[start] = 0;
							if (method == POST && key.empty()) {
								break;
							}
							user_data.insert(std::make_pair(std::move(key),
								std::move(std::string(&unparsed_data[last]))));
							last = start + 1;
							state = true;

						}
						else if (unparsed_data[start] == ':' && state) {
							unparsed_data[start] = 0;
							key = &unparsed_data[last];
							last = start + 2;
							state = false;
						}
					}

					if (method == POST) {
						printf(&unparsed_data[start + 1]);
					}
					std::map<std::string, std::string> header;
					header["Status"] = "200";
					time_t t = time(NULL);
					char c[26];

					asctime_s(c, gmtime(&t));
					c[strlen(c) - 1] = 0;
					header["Date"] = c;
					header["Content-Type"] = "text/html;";
					header["Connection"] = "keep-alive";
					header["Keep-Alive"] = "timeout=20";
					header["Vary"] = "Accept-Encoding";

					//header["Strict-Transport-Security"] = "max-age=86400";
					std::string data = Router::instance().fetch(user_data["url"].data(), header);
					//发送数据    
					std::string sendData = "HTTP/1.1 " + header["Status"] + "\n";
					for (const auto& it : header) {
						if (it.first == "Status") continue;
						sendData += it.first + ": " + it.second + "\n";

					}

					sendData += "\n";

					sendData += data + "\n\n";

					int sendC = send(q[*head].socket, sendData.data(), sendData.length(), 0);
				
					
				} while (1);

				if (repeat) {
					if (*head < *tail - 1) {
						swap(q[*head], q[(*head) + 1]);
						continue;
					}
					else continue;
				}
				
				shutdown(q[*head].socket, SD_BOTH);
				closesocket(q[*head].socket);

				(*head)++;

				if (*head < MAX_QUEUE / 2) continue;
				else {
					lock.lock();
					if(*head < *tail) memmove(q, &q[*head], (*tail - *head) * sizeof(ServerTask));
					*tail = *tail - *head;
					*head = 0;
					lock.unlock();
				}
			};
			cv.wait(lcx);
		}
	}

	~HttpThread();


};

class ThreadPool : public std::vector<HttpThread*> {
public:

	HttpThread* top() {
		return (*this)[0];
	}
	void update() {
		int index = 0;
		int child = index * 2 + 1;
		if (*this->at(child + 1) < *this->at(child)) child++;
		do {
			std::swap(this->at(child), this->at(index));
			index = child;
			child = index * 2 + 1;
		} while ( child < this->size() && *this->at(child) < *this->at(index));
	}
};