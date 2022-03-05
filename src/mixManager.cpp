#include "mixManager.h"
#include <seeker/common.h>

MixManager::MixManager()
{
}

MixManager::~MixManager()
{
}


uint8_t* MixManager::getYUVData(AVFrame* frame) {
    int frame_height = frame->height;
    int frame_width = frame->width;
    int i = 0;
    int j = 0;
    int k = 0;
    uint8_t* yuvData;
    int size = avpicture_get_size(AV_PIX_FMT_YUV420P, frame_width, frame_height);
    int video_decode_size = avpicture_get_size(AV_PIX_FMT_YUV420P, frame_width, frame_height);
    yuvData = (uint8_t*)malloc(size * 3 * sizeof(char));
    uint8_t* video_decode_buf = (uint8_t*)malloc(video_decode_size * 3 * sizeof(char));
    if (frame) {
        for (i = 0; i < frame_height; i++)
        {
            memcpy(video_decode_buf + frame_width * i,
                frame->data[0] + frame->linesize[0] * i,
                frame_width);
        }
        for (j = 0; j < frame_height / 2; j++)
        {
            memcpy(video_decode_buf + frame_width * i + frame_width / 2 * j,
                frame->data[1] + frame->linesize[1] * j,
                frame_width / 2);
        }
        for (k = 0; k < frame_height / 2; k++)
        {
            memcpy(video_decode_buf + frame_width * i + frame_width / 2 * j + frame_width / 2 * k,
                frame->data[2] + frame->linesize[2] * k,
                frame_width / 2);
        }

        if (video_decode_buf)
        {
            memcpy(yuvData, video_decode_buf, video_decode_size);
            free(video_decode_buf);
            video_decode_buf = nullptr;
        }
        else {
            free(yuvData);
            yuvData = nullptr;
        }

    }
    return yuvData;
}

void MixManager::mixVideo() {
    int video_num = 0;
    
    while (true) {
        unique_lock<mutex> lock{ mixPktMtx };
        mixCond.wait(lock, [&]() {return mixList.size() > 0; });
        int mix_index = 1;
        video_num = mixList.size();
        if (video_num == 1) {
            //不混流，直接编码，发送
            for (auto iter = mixList.begin(); iter != mixList.end(); iter++) {
                if (iter->second.data) {
                    auto yuvData = getYUVData(iter->second.data);
                    videoSender->send(yuvData, iter->second.width, iter->second.height, AV_PIX_FMT_YUV420P);
                    free(yuvData);
                    av_frame_free(&iter->second.data);
                }
            }
            I_LOG("1111");
        }
        else if (video_num == 2) {
            int id = 1;
            for (auto iter = mixList.begin(); iter != mixList.end(); iter++) {
                if (iter->second.data) {
                    auto yuvData = getYUVData(iter->second.data);
                    if (id%2 == 1) {
                        mixer_file.setPicture(id, 0, 0, 320, 180);
                        mixer_file.push(mix_index, yuvData, iter->second.width, iter->second.height, AV_PIX_FMT_YUV420P);
                    }
                    else if(id%2 == 0) {
                        mixer_file.setPicture(id, 320, 0, 320, 180);
                        mixer_file.push(mix_index, yuvData, iter->second.width, iter->second.height, AV_PIX_FMT_YUV420P);
                    }      
                    av_frame_free(&iter->second.data);
                    id++;
                }
            }
            auto dstFrame = av_frame_alloc();
            dstFrame->format = AV_PIX_FMT_RGB24;
            dstFrame->width = mixer_file.getCanvasWidth();
            dstFrame->height = mixer_file.getCanvasHeight();
            uint8_t* frameData = mixer_file.getCanvas();
            av_image_fill_arrays(dstFrame->data, dstFrame->linesize, frameData, AV_PIX_FMT_RGB24, dstFrame->width,
                dstFrame->height, 1);
            //放入编码器编码 发送
            videoSender->send(getYUVData(dstFrame), dstFrame->width, dstFrame->height, AV_PIX_FMT_YUV420P);
            av_frame_free(&dstFrame);
            I_LOG("2222");
        }
    }
}

int MixManager::init() {

    //init rtmp server
    netManager = std::make_shared<NetManager>();
    netManager->setRtmpUrl("C:/Users/97017/Desktop/output.flv");

    if (netManager->rtmpInit(0) == -1) {
        return 0;
    }

    //videoSender / audioSender init encoder
    videoSender = std::make_unique<VideoSender>(netManager);
    VideoDefinition vd = VideoDefinition(640, 360);
    videoSender->initEncoder(vd, 20);
    videoPacketTime = 1000 / 20;
    long long startTime = seeker::Time::currentTime();
    videoSender->setStartTime(startTime);

    //encoder info
    CoderInfo encoderinfo;
    encoderinfo.inChannels = 1;
    encoderinfo.inSampleRate = 22050;
    encoderinfo.inFormate = MyAVSampleFormat::AV_SAMPLE_FMT_S16;
    encoderinfo.outChannels = 1;
    encoderinfo.outSampleRate = 22050;
    encoderinfo.outFormate = MyAVSampleFormat::AV_SAMPLE_FMT_FLTP;
    encoderinfo.cdtype = CodecType::AAC;
    encoderinfo.muxType = theia::audioEngine::MuxType::ADTS;
    //1024 * 1000 / samplerate (ms
    //video 1000/ 帧数
    audioSender = std::make_unique<AudioSender>(netManager);
    audioSender->initAudioEncoder(encoderinfo);
    audioSender->setStartTime(startTime);

    if (netManager->rtmpInit(1) == -1) {
        return 0;
    }


    Manager* m1 = new Manager("127.0.0.1", "127.0.0.1", 30502, 1234);
    auto mixcallBack = [&](int32_t ssrc, AVFrame* data, int width, int height) {
        I_LOG("mix recv");
        mixPktMtx.lock();
        auto it = mixList.find(ssrc);
        if (it != mixList.end()) {
            it->second = mixFrame{ data, width, height };
        }
        else {
            mixList.insert(std::pair<int32_t, mixFrame>(ssrc, mixFrame{ data, width, height }));
        }
        mixPktMtx.unlock();
        mixCond.notify_one();
    };
    m1->setDecodeCB(mixcallBack);

    //init
    mixer_file.setCanvasSize(640, 360);
    mixer_file.setCanvasColor(0, 0, 0, 1);
    mixer_file.init();

    std::thread mixVideoThread{ &MixManager::mixVideo, this };
    map["mixVideo"] = std::move(mixVideoThread);
}