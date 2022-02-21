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

    encoder = new Encoder87();
    encoder->setIsCrf(false);
    encoder->setPixFmt(AV_PIX_FMT_RGB24);
    encoder->setCodecFmt("h264");
    encoder->setBitrate(1000000);
    encoder->setFrameRate(30);
    encoder->setFrameSize(1280, 720);
    encoder->setProfile("baseline");
    encoder->init();

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

        while (!pktList.empty())
        {
            auto pkt = pktList.front();
            pktList.pop_front();
            I_LOG("PKT:{}", pkt.second);
            decoder->push(pkt.first, pkt.second, 0);
            AVFrame* frame = av_frame_alloc();
            int rst = decoder->poll(frame);
            I_LOG("decoder poll");
            if (rst != 0 || frame->width <= 0 || frame->height <= 0) {
                I_LOG("no :{}, width:{}", rst, frame->width);
                av_frame_free(&frame);
                continue;
            }
            if (rst == 0) {
                if (ch == nullptr) {
                    I_LOG("rst :{}", rst);
                    ch = new ConvertH264Util(frame->width, frame->height, SAVEFILENAME);
                }
                I_LOG("write frame 1");
                ch->convertFrame(frame);
            }

            //push to encode list
            uint8_t* dstBuf = (uint8_t*)malloc(frame->width * frame->height * 3);
            memset(dstBuf, 0, frame->width * frame->height * 3);
            uint8_t* frameData = frame->data[0];
            for (int i = 0; i < frame->height; ++i) {
                memcpy(dstBuf + i * frame->width * 3, frameData, frame->width * 3);
                frameData += frame->linesize[0];
            }
            std::unique_lock<std::mutex> lk1(encodePktMtx);
            sendList.push(mixFrame{ dstBuf, frame->width, frame->height });
            sendpktCond.notify_all();

            av_frame_free(&frame);
            free(pkt.first);
        }
        lock.unlock();
    }
}

int Manager::encodeTh() {
    I_LOG("encoder thread start");
    int encode_count = 0;
    while (!stopFlag)
    {
        std::unique_lock<std::mutex> lk(encodePktMtx);
        sendpktCond.wait(lk, [this]() {return sendList.size() > 0 || stopFlag; });
        if (stopFlag)
        {
            lk.unlock();
            break;
        }

        auto dstData = sendList.front();
        sendList.pop();

        lk.unlock();
        //I_LOG("encode:{}", encode_count);
        encoder->push(dstData.data, dstData.width * dstData.height * 3, dstData.width, dstData.height);

        std::vector<std::pair<uint8_t*, int>> outData{};
        int ret = encoder->poll(outData);
        I_LOG("encode !!");
        if (ret == 0) {
            for (auto iter = outData.begin(); iter != outData.end(); iter++) {
                //·¢ËÍ
                as->send(iter->first, iter->second);
                free(iter->first);
                I_LOG("send !!");

            }
        }
        free(dstData.data);
    }
}