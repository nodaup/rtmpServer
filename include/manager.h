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
#include "netManager.h"
#include "VideoSender.h"
#include "AudioSender.h"


#define SAVEFILENAME "C:/Users/97017/Desktop/save_parse.h264"

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
    uint8_t* data;
    int len;
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

    int decodeAudioTh();

    int encodeAudioTh();

    void asio_video_thread();

    void asio_audio_thread();

    uint8_t* getYUVData(AVFrame* frame);

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
    condition_variable sendpktCond;

    //audio recv
    rtpServer* as_audio = nullptr;
    std::mutex recvPktMtx_a;
    std::mutex encodePktMtx_a;
    std::list<std::pair<uint8_t*, int>> pktList_a; //接收队列，待解码
    std::queue<audioFrame> sendList_a;
    condition_variable pktCond_a;
    condition_variable sendpktCond_a;
    // 音频封装格式
    AudioInfo info, outInfo;
    CodecType decoderCodecType;
    CoderInfo decoderInfo;
    std::unique_ptr<MixerImpl> mixer = nullptr;


    //send
    std::shared_ptr<NetManager> netManager = nullptr;
    std::unique_ptr<VideoSender> videoSender = nullptr;
    std::unique_ptr<AudioSender> audioSender = nullptr;

    int audioPacketTime;
    int videoPacketTime;

    uint8_t* yuvData;

};