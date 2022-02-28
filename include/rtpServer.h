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
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
};

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

#define BUFF_SIZE 10240 *2


#include <functional>

typedef std::function<void(uint8_t*, int, int32_t, int32_t, int32_t, int32_t)> RecvDataCallback;

class rtpServer {

private:

	//参考loki
	asio::io_context ioContext; //提供io核心功能
	udp::socket server;
	//true为音频，false为视频
	bool flag;
	int frameRate = 20;
	RecvDataCallback callBack;

public:

	rtpServer(const std::string& ip, int port, bool is_audio = false);

	void start();

	void send(uint8_t* pkt, int len);

	void setCallBack(RecvDataCallback _callBack);
	
};