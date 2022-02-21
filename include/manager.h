#pragma once
#include <functional>
#include <mutex>
#include<iostream>
#include<string>
#include<thread>
#include<math.h>
#include <map>
#include <list>
#include "videoEngine/VideoEngine.h"
#include "videoEngine/VideoEngine_imp_87.h"
#include <seeker/loggerApi.h>
#include <queue>
#include "rtpServer.h"
using theia::VideoEngine::Decoder;
using theia::VideoEngine::Encoder;
using theia::VideoEngine::imp_87::Decoder87;
using theia::VideoEngine::imp_87::Encoder87;

#define SAVEFILENAME "C:/Users/97017/Desktop/save_parse.h264"

using namespace std;
using std::mutex;

struct mixFrame
{
    uint8_t* data;
    int width;
    int height;
};

class Manager
{

public:

    Manager() {
        
    }

    //int saveRtpPkt(uint8_t* pkt, int pktsize);

    int init();

    int decodeTh();

    int encodeTh();

    std::unordered_map<std::string, std::thread> threadMap = {};
    std::list<std::pair<uint8_t*, int>> pktList; //接收队列，待解码
    std::queue<mixFrame> sendList; //处理完成队列，待编码，发送
    std::mutex recvPktMtx;
    std::mutex encodePktMtx;

    bool stopFlag = true;

    Decoder* decoder = nullptr;
    Encoder* encoder = nullptr;
    rtpServer* as = nullptr;
    
    condition_variable pktCond;
    condition_variable sendpktCond;
};