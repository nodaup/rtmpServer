//udp����

#pragma once

#include<iostream>
#include<string>
#include <asio.hpp>
#include "seeker/loggerApi.h"
#include<thread>
#include<mutex>
#include<math.h>
#include <map>

using std::thread;
using std::string;
using asio::ip::udp;
using std::mutex;
using std::unique_lock;
using std::pair;
using std::map;
using std::max;
using namespace asio;
using namespace std::chrono;
using asio::io_context;

#define BUFF_SIZE 10240


#include <functional>

typedef std::function<void(uint8_t*, int)> RecvDataCallback;

class rtpServer {

	//�ο�loki
	asio::io_context ioContext; //�ṩio���Ĺ���
	udp::socket server;
	//trueΪ��Ƶ��falseΪ��Ƶ
	bool flag;

public:

	rtpServer(const std::string& ip, int port, bool is_audio = false);

	void start();
	
	
};