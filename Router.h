#pragma once
#include <fstream>
#include <functional>
#include <map>
#include <vector>

#define ROUTER_WITH_SUB(url) matchlist.insert(std::make_pair(std::string(url),	\
							(routeFunc)[&](std::string* list ,void* params) -> std::string {	\
								std::map<std::string,routeFunc> matchlist;
#define _404_handle "cant find"
#define END_ROUTER_LEVEL return safe_search(matchlist,list,params);}))
#define ROUTER(url,func) matchlist.insert(std::make_pair(std::string(url),	\
							(routeFunc)func))

#define routeFuncDcl [&](std::string* list, void* params) -> std::string

#define WWWROOT "C:/Users/dell/Desktop/cc-sh/newTest"

typedef std::vector<std::string> StringList;
typedef std::function<std::string(std::string*, void*)> routeFunc;

inline std::string safe_search(
	std::map<std::string, routeFunc>& matchlist,
	std::string* list,
	void* params) {
	return (matchlist.find(list[0]) == matchlist.end() ? _404_handle : matchlist[list[0]](&list[1], params));
}

class Router
{
public:
	Router() {
		ROUTER("", routeFuncDcl{ return openfile("/index.html"); });
		ROUTER_WITH_SUB("article")
			ROUTER("", routeFuncDcl{ return openfile("/article/index.html"); });
			ROUTER("list", routeFuncDcl{ return
				"[{\"id\":\"1\",\"title\":\"hello\",\"content\":\"world\"},"
				"{\"id\":\"2\",\"title\":\"hello\",\"content\":\"world\"}]";
			});
		END_ROUTER_LEVEL;
		ROUTER("music", routeFuncDcl{ return openfile("/music/index.html"); });
	}
	~Router();
	std::string fetch(const char* url,std::map<std::string,std::string>& header) {
		StringList strlist;

		{
			int i = strlen(url) - 1;
			for (;i >= 0 && url[i] != '.';i--);
			if (i > 0) {
				if (&url[i + 1] == std::string("js")) {
					header["Content-Type"] = "text/javascript";
				} else if (&url[i + 1] == std::string("css")) {
					header["Content-Type"] = "text/css";
				}
				return openfile(url);
			}
		} // parse,static 
		
		std::string post = url;
		int last = 1;
		for (int i = 1;i < post.size();i++) {
			if (post[i] == '/') {
				post[i] = 0;
				strlist.push_back(&post[last]);
				last = i + 1;
			}
		}

		strlist.push_back("");
		if(strlist.size() == 1) strlist.push_back("");

		auto it = matchlist.find(strlist[0]);
		if (it == matchlist.end()) {
			return _404_handle;
			// 404;
		}
		else {
			return it->second(&strlist[1],NULL);
		}
	}
	std::string openfile(const char* fname) {
		static std::string wwwroot(WWWROOT);
		std::string data;
		std::ifstream inf;
		
		inf.open((wwwroot+fname), std::ios_base::in);
		if(!inf.is_open()) return "";
		inf.seekg(0, std::ios::end);
		size_t fs = inf.tellg();
		inf.seekg(0, std::ios::beg);

		char _buffer[1025];
		while(!inf.eof())
		{
			inf.read(_buffer, 1024);
			size_t s = inf.gcount();
			if (s == 0) break;
			_buffer[s] = 0;
			data += _buffer;
		}


		inf.close();
		return data;

	}
	static Router& instance() {
		static Router r;
		return r;
	}
	std::map<std::string, routeFunc> matchlist;
};

