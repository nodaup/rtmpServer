#include "mixManager.h"
#include <seeker/common.h>

MixManager::MixManager()
{
}

MixManager::~MixManager()
{
}

int MixManager::addManager(Manager* m) {
    managerList.push_back(m);
    return 0;
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

int MixManager::mixVideo() {
    //todo 将list中的sendlist放进mix里
    int video_num = 0;
    while (true) {
        int mix_index = 1;
        video_num = managerList.size();
        if (video_num == 1) {
            //不混流，直接编码，发送
            auto m = managerList[0];
            std::unique_lock<std::mutex> lk(m->encodePktMtx);
            auto dstData = m->sendList.front();
            m->sendList.pop();
            lk.unlock();
            if (dstData.data != NULL) {
                auto yuvData = getYUVData(dstData.data);
                videoSender->send(getYUVData(dstData.data), dstData.width, dstData.height, AV_PIX_FMT_YUV420P);
                free(yuvData);
                av_frame_free(&dstData.data);
            }
        }
        else if (video_num == 2) {
            //left right
            auto leftM = managerList[0];
            auto rightM = managerList[1];
            std::unique_lock<std::mutex> lk(leftM->encodePktMtx);
            auto frame_left = leftM->sendList.front();
            leftM->sendList.pop();
            lk.unlock();

            std::unique_lock<std::mutex> lk2(rightM->encodePktMtx);
            auto frame_right = rightM->sendList.front();
            rightM->sendList.pop();
            lk2.unlock();

            auto yuvData1 = getYUVData(frame_left.data);
            auto yuvData2 = getYUVData(frame_right.data);
            mixer_file->setPicture(1, 0, 180, 640, 360);
            mixer_file->push(1, yuvData1, frame_left.width, frame_left.height, AV_PIX_FMT_YUV420P);
            mixer_file->setPicture(2, 640, 180, 640, 360);
            mixer_file->push(1, yuvData2, frame_right.width, frame_right.height, AV_PIX_FMT_YUV420P);
            auto dstFrame = av_frame_alloc();
            dstFrame->format = AV_PIX_FMT_RGB24;
            dstFrame->width = mixer_file->getCanvasWidth();
            dstFrame->height = mixer_file->getCanvasHeight();
            uint8_t* frameData = mixer_file->getCanvas();
            av_image_fill_arrays(dstFrame->data, dstFrame->linesize, frameData, AV_PIX_FMT_RGB24, dstFrame->width,
                dstFrame->height, 1);
            //放入编码器编码 发送
            videoSender->send(getYUVData(dstFrame), dstFrame->width, dstFrame->height, AV_PIX_FMT_YUV420P);
            free(yuvData1);
            free(yuvData2);
            av_frame_free(&dstFrame);
        }
        else if (video_num > 2) {
            //混多路流
            uint8_t* yuvData;
            for (auto iter : managerList) {
                std::unique_lock<std::mutex> lk(iter->encodePktMtx);
                auto frame = iter->sendList.front();
                iter->sendList.pop();
                lk.unlock();
                yuvData = getYUVData(frame.data);
                if (mix_index == 1) {
                    mixer_file->setPicture(mix_index, 0, 0, 640, 360);
                    mixer_file->push(mix_index, yuvData, frame.width, frame.height, AV_PIX_FMT_YUV420P);
                }
                else if (mix_index == 2) {
                    mixer_file->setPicture(mix_index, 640, 0, 640, 360);
                    mixer_file->push(mix_index, yuvData, frame.width, frame.height, AV_PIX_FMT_YUV420P);
                }
                else if (mix_index == 3) {
                    mixer_file->setPicture(mix_index, 0, 360, 640, 360);
                    mixer_file->push(mix_index, yuvData, frame.width, frame.height, AV_PIX_FMT_YUV420P);
                }
                else if (mix_index == 4) {
                    mixer_file->setPicture(mix_index, 360, 640, 640, 360);
                    mixer_file->push(mix_index, yuvData, frame.width, frame.height, AV_PIX_FMT_YUV420P);
                }
                mix_index++;
            }
            auto dstFrame = av_frame_alloc();
            dstFrame->format = AV_PIX_FMT_RGB24;
            dstFrame->width = mixer_file->getCanvasWidth();
            dstFrame->height = mixer_file->getCanvasHeight();
            uint8_t* frameData = mixer_file->getCanvas();
            av_image_fill_arrays(dstFrame->data, dstFrame->linesize, frameData, AV_PIX_FMT_RGB24, dstFrame->width,
                dstFrame->height, 1);
            //放入编码器编码 发送
            videoSender->send(getYUVData(dstFrame), dstFrame->width, dstFrame->height, AV_PIX_FMT_YUV420P);
            free(yuvData);
            av_frame_free(&dstFrame);
        }
    }
}

int MixManager::init() {

    //init rtmp server
    netManager = std::make_shared<NetManager>();
    netManager->setRtmpUrl("rtmp://10.1.120.211:1935");

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

    //init 
    mixer_file->setCanvasSize(1280, 720);
    mixer_file->setCanvasColor(0, 0, 0, 1);
    mixer_file->init();
}