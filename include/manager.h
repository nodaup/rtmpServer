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

struct audioFrame
{
    AVFrame* data;
    int len;
    int32_t ssrc;
};

class Manager
{

public:

    Manager(std::string videoIP, std::string audioIP, int videoPort, int audioPort);

    //int saveRtpPkt(uint8_t* pkt, int pktsize);

    int init();

    int decodeTh();

    //int encodeTh();

    //int encodeAudioTh();

    void asio_audio_thread();

    //uint8_t* getYUVData(AVFrame* frame);

    //video recv
    std::unordered_map<std::string, std::thread> threadMap = {};
    std::list<std::pair<uint8_t*, int>> pktList; //接收队列，待解码
    std::queue<mixFrame> sendList; //处理完成队列，待编码，发送
    std::mutex recvPktMtx;
    std::mutex encodePktMtx;
    bool stopFlag = false;
    theia::VideoEngine::Decoder* decoder = nullptr;
    rtpServer* as = nullptr;
    condition_variable pktCond;
    //condition_variable sendpktCond;

    //audio recv
    rtpServer* as_audio = nullptr;
    std::mutex encodePktMtx_a;
    std::queue<audioFrame> sendList_a;
    //condition_variable sendpktCond_a;
    // 音频封装格式
    CodecType decoderCodecType;
    CoderInfo decoderInfo;
    DecoderImpl adecoder;
    std::ofstream writeRecv;

    //send
    //std::shared_ptr<NetManager> netManager = nullptr;
    //std::unique_ptr<VideoSender> videoSender = nullptr;
   // std::unique_ptr<AudioSender> audioSender = nullptr;

    int audioPacketTime;
    int videoPacketTime;

    //uint8_t* yuvData;

    std::string audioIP;
    std::string videoIP;
    int audioPort;
    int videoPort;

};