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
#include <fstream>
#include "videoEngine/VideoEngine_imp_87.h"
using PictureMixer = theia::VideoEngine::imp_87::PictureMixer7;


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
    AVFrame* data;
    int len;
    int32_t ssrc;
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

    int encodeAudioTh();

    void asio_audio_thread();

    void video_recv_2();

    int decodeTh2();

    uint8_t* getYUVData(AVFrame* frame);

    //video recv
    std::unordered_map<std::string, std::thread> threadMap = {};
    std::list<std::pair<int32_t, std::pair<uint8_t*, int>>> pktList; //���ն��У�������
    std::queue<mixFrame> sendList; //������ɶ��У������룬����
    std::queue<mixFrame> sendList_;
    std::mutex recvPktMtx;
    std::mutex encodePktMtx;
    bool stopFlag = false;
    theia::VideoEngine::Decoder* decoder = nullptr;
    rtpServer* as = nullptr;
    condition_variable pktCond;
    condition_variable sendpktCond;

    //video recv2
    rtpServer* as2 = nullptr;
    condition_variable pktCond2;
    std::mutex recvPktMtx2;
    std::list<std::pair< int32_t, std::pair<uint8_t*, int>>> pktList2;
    theia::VideoEngine::Decoder* decoder2 = nullptr;
    std::queue<mixFrame> sendList2; //������ɶ��У������룬����
    std::queue<mixFrame> sendList2_;
    PictureMixer mixer_file;

    //audio recv
    rtpServer* as_audio = nullptr;
    std::mutex encodePktMtx_a;
    std::queue<audioFrame> sendList_a;
    condition_variable sendpktCond_a;
    // ��Ƶ��װ��ʽ
    AudioInfo info, outInfo;
    CodecType decoderCodecType;
    CoderInfo decoderInfo;
    DecoderImpl adecoder;
    std::ofstream writeRecv;

    //send
    std::shared_ptr<NetManager> netManager = nullptr;
    std::unique_ptr<VideoSender> videoSender = nullptr;
    std::unique_ptr<AudioSender> audioSender = nullptr;

    int audioPacketTime;
    int videoPacketTime;

    uint8_t* yuvData;

};