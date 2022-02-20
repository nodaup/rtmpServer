#include "manager.h"
#include "ConvertH264.hpp"

int Manager::saveRtpPkt(uint8_t* pkt, int pktsize) {

    recvPktMtx.lock();
    auto temp = new uint8_t[pktsize + 1]();
    memcpy(temp, pkt, pktsize);
    pktList.push_back(std::pair<uint8_t*, int>(temp, pktsize));
    recvPktMtx.unlock();
    pktCond.notify_one();
    return 0;
}

int Manager::init() {
    decoder = new Decoder87();
    decoder->setPixFmt(AV_PIX_FMT_YUV420P);
    decoder->init();
    ConvertH264Util* ch = nullptr;
    auto decodeThreadFunc = [&]() {
        while (true)
        {
            list<std::pair<uint8_t*, int>> tempPktList;

            unique_lock<mutex> lock{ recvPktMtx };
            pktCond.wait(lock, [&]() {return pktList.size() > 0; });

            while (!pktList.empty())
            {
                auto pkt = pktList.front();
                pktList.pop_front();
               /* tempPktList.push_back(pkt);*/

                decoder->push(pkt.first, pkt.second, 0);
                AVFrame* frame = av_frame_alloc();
                int rst = decoder->poll(frame);
                I_LOG("decoder poll");
                if (rst != 0 || frame->width <= 0 || frame->height <= 0) {
                    I_LOG("no :{}, width:{}", rst, frame->width);
                    av_frame_free(&frame);
                    continue;
                }
                if (ch == nullptr) {
                    ch = new ConvertH264Util(frame->width, frame->height, SAVEFILENAME);
                }
                I_LOG("write frame 1");
                ch->convertFrame(frame);
                av_frame_free(&frame);

            }
            lock.unlock();

           /* for (auto it = tempPktList.begin(); it != tempPktList.end(); it++) {
                I_LOG("tempPktList:{}", tempPktList.size());
                decoder->push(it->first, it->second, 0);
                AVFrame* frame = av_frame_alloc();
                int rst = decoder->poll(frame);
                I_LOG("decoder poll");
                if (rst != 0 || frame->width <= 0 || frame->height <= 0) {
                    I_LOG("no :{}, width:{}", rst, frame->width);
                    av_frame_free(&frame);
                    continue;
                }
                if (ch == nullptr) {
                    ch = new ConvertH264Util(frame->width, frame->height, SAVEFILENAME);
                }
                I_LOG("write frame 1");
                ch->convertFrame(frame);
                av_frame_free(&frame);
            }*/
        }
    };

    std::thread decodeThread{ decodeThreadFunc };
    decodeThread.detach();
}