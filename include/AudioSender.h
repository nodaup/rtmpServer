#pragma once

#include <netManager.h>
#include "audioEngine/AudioEngine_imp_85.h"
#include "AudioUtil.hpp"

class AudioSender {
public:
    //相关参数通过构造函数传入
    AudioSender(std::shared_ptr<NetManager> _netManager);
    ~AudioSender();
    int stop();
    void initAudioEncoder(CoderInfo encoderInfo);
    int send(uint8_t* buf, int len);
    void setStartTime(long long _startTime);
private:
    //netManager
    std::shared_ptr<NetManager> netManager = nullptr;

    // encoder
    AudioInfo info, outInfo;
    std::unique_ptr<theia::audioEngine::imp_85::EncoderImpl> encoder;  // 音频编码器
    CodecType encoderCodecType;        // 编码封装格式
    int count = 0;
    long long startTime;
    AudioInfo out;
    AudioInfo in;
    int lastPts = 0;
};
