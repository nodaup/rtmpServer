#pragma once

extern "C" {
#include <libavformat/avformat.h>
};

#include <string>
#include <thread>
#include <functional>
#include "seeker/logger.h"

class NetManager {
public:
    NetManager();
    ~NetManager();

    //todo rtmp
    int sendRTMPData(AVPacket* packet);
    int setVideoStream(AVCodecContext* encodeCtx);
    int setAudioStream(AVCodecContext* encodeCtx);
    int stopRTMP();

    //rtmp
    int rtmpInit(int step);
    void setRtmpUrl(std::string _rtmpUrl);

    AVStream* getAudioStream() {
        return audio_st;
    }
    AVStream* getVideoStream() {
        return video_st;
    }
    AVFormatContext* getOutFormatCtx() {
        return pFormatCtx;
    }

    int newAudioStream(AVCodecContext* encodeCtx);

    int newVideoStream(AVCodecContext* encodeCtx);


private:

    AVFormatContext* pFormatCtx = nullptr;
    AVOutputFormat* fmt = nullptr;
    AVStream* video_st = nullptr;
    AVStream* audio_st = nullptr;
    std::unique_ptr<std::mutex> mutex4Sender = nullptr;

    std::string rtmpUrl = "rtmp://10.28.197.125:1935";
};