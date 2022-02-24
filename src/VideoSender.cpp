#include "VideoSender.h"
#include <seeker/common.h>
long long last2 = 0;

uint8_t* AVFrame2Img(AVFrame* pFrame) {
    int frameHeight = pFrame->height;
    int frameWidth = pFrame->width;
    int channels = 3;

    //反转图像
  //  pFrame->data[0] += pFrame->linesize[0] * (frameHeight - 1);
  //  pFrame->linesize[0] *= -1;
  //  pFrame->data[1] += pFrame->linesize[1] * (frameHeight / 2 - 1);
  //  pFrame->linesize[1] *= -1;
  //  pFrame->data[2] += pFrame->linesize[2] * (frameHeight / 2 - 1);
  //  pFrame->linesize[2] *= -1;

    //创建保存yuv数据的buffer
    uint8_t* pDecodedBuffer = (uint8_t*)malloc(
        frameHeight * frameWidth * sizeof(uint8_t) * channels);

    //从AVFrame中获取yuv420p数据，并保存到buffer
    int i, j, k;
    //拷贝y分量
    for (i = 0; i < frameHeight; i++) {
        memcpy(pDecodedBuffer + frameWidth * i,
            pFrame->data[0] + pFrame->linesize[0] * i,
            frameWidth);
    }
    //拷贝u分量
    for (j = 0; j < frameHeight / 2; j++) {
        memcpy(pDecodedBuffer + frameWidth * i + frameWidth / 2 * j,
            pFrame->data[1] + pFrame->linesize[1] * j,
            frameWidth / 2);
    }
    //拷贝v分量
    for (k = 0; k < frameHeight / 2; k++) {
        memcpy(pDecodedBuffer + frameWidth * i + frameWidth / 2 * j + frameWidth / 2 * k,
            pFrame->data[2] + pFrame->linesize[2] * k,
            frameWidth / 2);
    }
    return pDecodedBuffer;
}

VideoSender::VideoSender(std::shared_ptr<NetManager> _netManager) {
    netManager = _netManager;
    mutex4Encoder = std::make_unique<std::mutex>();
};

VideoSender::~VideoSender() {
    I_LOG("VideoSender destruct ...");
    //stop();
}

void VideoSender::initEncoder(const VideoDefinition& captureSize, int frameRate) {
    std::unique_lock<std::mutex> encoderLk(*mutex4Encoder);
    if (encoder) {
        encoder->stop();
        delete encoder;
        encoder = nullptr;
        I_LOG("re new an encoder... ");
        encoder = new theia::VideoEngine::imp_87::Encoder87();
    }
    else {
        I_LOG("new an encoder...");
        encoder = new theia::VideoEngine::imp_87::Encoder87();
    }
    encoder->setIsRTMP(true);
    encoder->setCodecFmt("h264");
    encoder->setIsCrf(false);
    encoder->setProfile("high");
    encoder->setMaxBFrame(0);
    encoder->setGopSize(30);
    encoder->setPixFmt(AV_PIX_FMT_BGRA);
    this->frameRate = frameRate;
    encoder->setFrameRate(frameRate);
    VideoDefinition encoderSize = VideoSenderUtil::calEncoderSize(captureSize, maxDefinition);
    encoder->setFrameSize(encoderSize.width, encoderSize.height);
    int bitRate = VideoSenderUtil::calBitRate(encoderSize);
    encoder->setBitrate(bitRate);
    encoder->init();
    encoderLk.unlock();
    I_LOG("init encoder finished: size={}, bitRate={}", encoderSize.getString(), bitRate);

    //netManager->newVideoStream(encoder->getEncoderCtx());

    encoder->copyParams(netManager->getVideoStream()->codecpar);
}

int VideoSender::stop() {
    if (encoder) {
        std::unique_lock<std::mutex> encoderLk(*mutex4Encoder);
        encoder->stop();
        encoderLk.unlock();
    }
    return 0;
}

int VideoSender::send(uint8_t* data, int width, int height, AVPixelFormat fmt) {
    std::unique_lock<std::mutex> encoderLk(*mutex4Encoder);
    encoder->pushWatchFmt(data, -1, width, height, (AVPixelFormat)fmt);
    AVPacket* packet = encoder->pollAVPacket();
    if (packet) {
        if (netManager) {
            long long current = seeker::Time::currentTime();
            //I_LOG("actual duration -------- = {}", current - last2);
            last2 = current;
            long long now = seeker::Time::currentTime() - startTime;
            packet->pts = now;
            packet->dts = packet->pts;
            packet->duration = packet->pts - lastPts;
            lastPts = packet->pts;
            count++;
            if (count > 0) {
                I_LOG("video packet index {} pts {} dura {} frameRate {} expectPts {} deltaPts {}", count, packet->pts, packet->duration, count * 1000 / packet->pts, 50 * count, 50 * count - packet->pts);
            }
            packet->stream_index = 0;
            if (packet->pts > now)
            {
                //usleep((packet->pts - now) * 1000);
                std::this_thread::sleep_for(std::chrono::microseconds((packet->pts - now) * 1000));
            }
            //else{
            //  W_LOG("late:{}",now-packet->pts);
            //}
            int ret = netManager->sendRTMPData(packet);
            encoderLk.unlock();
            return ret;
        }
    }
    encoderLk.unlock();
    return -1;
}

int VideoSender::sendFrame(AVFrame* frame) {
    std::unique_lock<std::mutex> encoderLk(*mutex4Encoder);
    encoder->pushWatchFmt(AVFrame2Img(frame), -1, frame->width, frame->height,
        static_cast<AVPixelFormat>(frame->format));
    AVPacket* packet = encoder->pollAVPacket();
    if (packet) {
        packet->duration = ceil(1000 / frameRate);
        count++;
        I_LOG("video packet index {} pts {} dura {}", count, packet->pts, packet->duration);
        packet->stream_index = 0;
        if (netManager) {
            long long now = seeker::Time::currentTime() - startTime;
            if (packet->pts > now)
            {
                std::this_thread::sleep_for(std::chrono::microseconds((packet->pts - now) * 1000));
            }
            int ret = netManager->sendRTMPData(packet);
            encoderLk.unlock();
            return ret;
        }
    }
    encoderLk.unlock();
    return -1;
}

void VideoSender::setStartTime(long long _startTime) {
    startTime = _startTime;
}