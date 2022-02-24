#pragma once
#include <string>

//#include "AudioCaptureRender/AudioCaptureRenderImp88.h"
#include "audioEngine/AudioEngine_imp_85.h"
//#include "media/common/MediaCommon.h"

using theia::audioEngine::AudioInfo;
using theia::audioEngine::CodecType;
using theia::audioEngine::MuxType;
using theia::audioEngine::Encoder;
using theia::audioEngine::Decoder;
using theia::audioEngine::ModuleType;
using theia::audioEngine::imp_85::EncoderImpl;
using theia::audioEngine::imp_85::DecoderImpl;
using theia::audioEngine::imp_85::MixerImpl;
using std::string;


//音频采样格式
enum class MyAVSampleFormat {
    AV_SAMPLE_FMT_NONE = -1,
    AV_SAMPLE_FMT_U8,   ///< unsigned 8 bits
    AV_SAMPLE_FMT_S16,  ///< signed 16 bits
    AV_SAMPLE_FMT_S32,  ///< signed 32 bits
    AV_SAMPLE_FMT_FLT,  ///< float
    AV_SAMPLE_FMT_DBL,  ///< double

    AV_SAMPLE_FMT_U8P,   ///< unsigned 8 bits, planar
    AV_SAMPLE_FMT_S16P,  ///< signed 16 bits, planar
    AV_SAMPLE_FMT_S32P,  ///< signed 32 bits, planar
    AV_SAMPLE_FMT_FLTP,  ///< float, planar
    AV_SAMPLE_FMT_DBLP,  ///< double, planar
    AV_SAMPLE_FMT_S64,   ///< signed 64 bits
    AV_SAMPLE_FMT_S64P,  ///< signed 64 bits, planar

    AV_SAMPLE_FMT_NB  ///< Number of sample formats. DO NOT USE if linking dynamically
};

//音频封装信息
struct CoderInfo {
    int inSampleRate;
    int inChannels;
    MyAVSampleFormat inFormate;
    int outSampleRate;
    int outChannels;
    MyAVSampleFormat outFormate;
    CodecType cdtype;                     //编码类型
    theia::audioEngine::MuxType muxType;  //封装类型
};

//类型：编码器还是解码器
enum class CoderType { EnCoder = 0, DeCoder = 1 };

//音频设置信息
typedef struct AudioSettingInfo {
    bool use_ns;  //噪声抑制
    uint8_t ns_mode;
    bool use_vad;  //静音帧判断
    uint8_t vad_mode;
    uint8_t per_ms;
    bool use_aec = false;  //回声消除
    bool use_agc = true;   //自动音量调节
    uint8_t compressionGaindB;
    uint8_t targetLevelDbfs;
    uint8_t limiterEnable;
    bool record = false;  //录音
    string filePath;

    AudioSettingInfo(bool use_ns_ = false, uint8_t ns_mode_ = 0, bool use_vad_ = false,
        uint8_t vad_mode_ = 0, uint8_t per_ms_ = 0, bool use_aec_ = false,
        bool use_agc_ = false, uint8_t compressionGaindB_ = 0,
        uint8_t targetLevelDbfs_ = 0, uint8_t limiterEnable_ = 0,
        bool record_ = false, string filePath_ = "")
        : use_ns(use_ns_),
        ns_mode(ns_mode_),
        use_vad(use_vad_),
        vad_mode(vad_mode_),
        per_ms(per_ms_),
        use_aec(use_aec_),
        use_agc(use_agc_),
        compressionGaindB(compressionGaindB_),
        targetLevelDbfs(targetLevelDbfs_),
        limiterEnable(limiterEnable_),
        record(record_),
        filePath(filePath_) {};

    void setInfo(bool use_ns_, uint8_t ns_mode_, bool use_vad_, uint8_t vad_mode_,
        uint8_t per_ms_, bool use_aec_, bool use_agc_, uint8_t compressionGaindB_,
        uint8_t targetLevelDbfs_, uint8_t limiterEnable_, bool record_,
        string filePath_) {
        use_ns = use_ns_, ns_mode = ns_mode_, use_vad = use_vad_, vad_mode = vad_mode_,
            per_ms = per_ms_, use_aec = use_aec_, use_agc = use_agc_,
            compressionGaindB = compressionGaindB_, targetLevelDbfs = targetLevelDbfs_,
            limiterEnable = limiterEnable_, record = record_, filePath = filePath_;
    }
} AudioSettingInfo;