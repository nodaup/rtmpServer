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
using theia::VideoEngine::Decoder;
using theia::VideoEngine::Encoder;
using theia::VideoEngine::imp_87::Decoder87;
using theia::VideoEngine::imp_87::Encoder87;

#define SAVEFILENAME "C:/Users/97017/Desktop/save_parse.h264"

using namespace std;
using std::mutex;

class Manager
{

public:

    Manager() {
    }

    int saveRtpPkt(uint8_t* pkt, int pktsize);

    int init();

    std::list<std::pair<uint8_t*, int>> pktList;
    std::mutex recvPktMtx;
    Decoder* decoder = nullptr;
    condition_variable pktCond;
};