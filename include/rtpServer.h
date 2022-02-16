//udp服务

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


typedef std::function<void(uint8_t*, int)> RecvDataCallback;

class rtpServer {

	//参考loki
	asio::io_context ioContext; //提供io核心功能
	udp::socket server;
	//true为音频，false为视频
	bool flag;

//private:
//
//	asio::io_service& ioService;
//	udp::socket* localSocket4Rtp;
//	int remotePort = 1234;
//	int localPort4Rtp = 4321;
//	string remoteIp = "127.0.0.1";
//	int32_t remoteSsrc = -1;
//	uint8_t recvBuf4Rtp[BUFF_SIZE]{ 0 };
//	udp::endpoint tempEndpoint4Rtp;
//
//	bool stopFlag = false;
//
//	void do_data_recv();
//	void handleData4Rtp(const asio::error_code error, size_t bytes_transferred);

public:

	rtpServer(const std::string& ip, int port, bool is_audio = false);

	void start();

	
};