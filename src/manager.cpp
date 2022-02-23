#include "manager.h"
#include "ConvertH264.hpp"

//int Manager::saveRtpPkt(uint8_t* pkt, int pktsize) {
//
//    as->recvPktMtx.lock();
//    auto temp = new uint8_t[pktsize + 1]();
//    memcpy(temp, pkt, pktsize);
//    as->pktList.push_back(std::pair<uint8_t*, int>(temp, pktsize));
//    as->recvPktMtx.unlock();
//    pktCond.notify_one();
//    return 0;
//}

int Manager::init() {
    decoder = new Decoder87();
    decoder->setPixFmt(AV_PIX_FMT_YUV420P);
    decoder->init();
    ConvertH264Util* ch = nullptr;

    netManager = std::make_shared<NetManager>();

    if (netManager->rtmpInit(0) == -1) {
        I_LOG("net manager error");
    }

    encoder = new Encoder87();
    //encoder->setIsCrf(false);
    encoder->setCodecFmt("h264");
    encoder->setBitrate(1000000);
    encoder->setFrameRate(20);
    encoder->setFrameSize(640, 360);
    encoder->setIsRTMP(true);
    encoder->setIsCrf(false);
    encoder->setProfile("high");
    encoder->setMaxBFrame(0);
    encoder->setGopSize(30);
    encoder->setPixFmt(AV_PIX_FMT_BGRA);
    encoder->init();
    encoder->copyParams(netManager->getVideoStream()->codecpar);

    if (netManager->rtmpInit(1) == -1) {
        I_LOG("net manager error");
    }

    std::thread decodeThread{ &Manager::decodeTh, this };
    threadMap["decodeTh"] = std::move(decodeThread);

    std::thread encodeThread{ &Manager::encodeTh, this };
    threadMap["encodeTh"] = std::move(encodeThread);

    as = new rtpServer("127.0.0.1", 30502, false);  

    auto recvCallBack = [&](uint8_t* pkt, int pktsize) {
        recvPktMtx.lock();
        auto temp = new uint8_t[pktsize + 1]();
        memcpy(temp, pkt, pktsize);
        pktList.push_back(std::pair<uint8_t*, int>(temp, pktsize));
        recvPktMtx.unlock();
        pktCond.notify_one();
    };
    as->setCallBack(recvCallBack);
    as->start();

}


int Manager::decodeTh() {
    ConvertH264Util* ch = nullptr;

    while (true)
    {
        list<std::pair<uint8_t*, int>> tempPktList;

        unique_lock<mutex> lock{ recvPktMtx };
        pktCond.wait(lock, [&]() {return pktList.size() > 0; });

        auto pkt = pktList.front();
        pktList.pop_front();
        lock.unlock();
        //I_LOG("PKT:{}", pkt.second);
        decoder->push(pkt.first, pkt.second, 0);
        AVFrame* frame = av_frame_alloc();
        int rst = decoder->poll(frame);
        //I_LOG("decoder poll");
        if (rst != 0 || frame->width <= 0 || frame->height <= 0) {
            //I_LOG("no :{}, width:{}", rst, frame->width);
            av_frame_free(&frame);
            continue;
        }
         /*if (rst == 0) {
             if (ch == nullptr) {
                 I_LOG("rst :{}", rst);
                 ch = new ConvertH264Util(frame->width, frame->height, SAVEFILENAME);
             }
             I_LOG("write frame 1");
             ch->convertFrame(frame);
         }*/

        if (rst == 0) {
            //push to encode list
            std::unique_lock<std::mutex> lk1(encodePktMtx);
            sendList.push(mixFrame{ frame, frame->width, frame->height });
            sendpktCond.notify_one();
            lk1.unlock();
        }

        free(pkt.first);
    }
}

int Manager::encodeTh() {
    I_LOG("encoder thread start");
    while (true)
    {
        std::unique_lock<std::mutex> lk(encodePktMtx);
        sendpktCond.wait(lk, [this]() {return sendList.size() > 0; });
        if (stopFlag)
        {
            lk.unlock();
            break;
        }
        auto dstData = sendList.front();
        sendList.pop();
        auto input = AVFrame2Img(dstData.data);
        encoder->pushWatchFmt(input, -1, dstData.width, dstData.height, AV_PIX_FMT_YUV420P);
        AVPacket* packet = encoder->pollAVPacket();
        I_LOG("video packet pts {} dura {}", packet->pts, packet->duration);
        if (packet) {
            I_LOG("///");
            packet->duration = ceil(1000 / 30);
            packet->stream_index = 0;
            if (netManager) {
                I_LOG("???");
                long long now = GetTickCount64();
                if (packet->pts > now)
                {
                    std::this_thread::sleep_for(std::chrono::microseconds((packet->pts - now) * 1000));
                }
                int ret = netManager->sendRTMPData(packet);
                I_LOG("send !!");
            }
        }
        lk.unlock();
        delete input;
    }
}

uint8_t* Manager::AVFrame2Img(AVFrame* pFrame) {
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