#pragma once
#include <functional>
#include <mutex>
#include<iostream>
#include<string>
#include<thread>
#include<math.h>
#include <map>
#include <list>
#include <seeker/loggerApi.h>
#include <queue>
#include "rtpServer.h"
#include <fstream>
#include <videoEngine/VideoEngine_imp_87.h>
#include "audioEngine/AudioEngine_imp_85.h"
#include "VideoUtil.h"
#include "AudioUtil.hpp"

#define SAVEFILENAME "C:/Users/97017/Desktop/save_parse"+std::to_string(seeker::Time::currentTime()) +".h264"

using namespace std;
using std::mutex;

struct mixFrame
{
    AVFrame* data;
    int width;
    int height;
};

struct recvFrame
{
    uint8_t* data;
    int len;
};

struct audioFrame
{
    AVFrame* data;
    int len;
    int32_t ssrc;
};

typedef std::function<void(int32_t, AVFrame* data, int width, int height)> decodeCB;

class Manager
{

public:

    Manager(std::string videoIP, std::string audioIP, int videoPort, int audioPort);

    //int saveRtpPkt(uint8_t* pkt, int pktsize);

    int init();

    int decodeTh();


    void asio_audio_thread();

    void setDecodeCB(decodeCB _callBack);

    //video recv
    std::unordered_map<std::string, std::thread> threadMap = {};
    std::map<int32_t, recvFrame> pktList = {}; //接收队列，待解码
    std::mutex recvPktMtx;
    bool stopFlag = false;
    theia::VideoEngine::Decoder* decoder = nullptr;
    rtpServer* as = nullptr;
    condition_variable pktCond;

    //audio recv
    rtpServer* as_audio = nullptr;
    std::mutex encodePktMtx_a;
    std::queue<audioFrame> sendList_a;
    // 音频封装格式
    CodecType decoderCodecType;
    CoderInfo decoderInfo;
    DecoderImpl adecoder;
    std::ofstream writeRecv;

    int audioPacketTime;

    std::string audioIP;
    std::string videoIP;
    int audioPort;
    int videoPort;

    decodeCB cb;

    bool flag = false;
};