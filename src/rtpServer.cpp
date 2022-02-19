#include "rtpServer.h"
#include "rtpParse.h"
#include "ConvertH264.hpp"
#include <thread>

using namespace std;

#define SAVEFILENAME "C:/Users/97017/Desktop/save_parse.h264"

rtpServer::rtpServer(const std::string& ip, int port, bool is_audio)
    : server{ ioContext, udp::endpoint(udp::v4(), port) } {
    I_LOG("asio server inits, ip is {} , udp port is {}", ip, port);
    flag = is_audio;
    flag = is_audio;


	//auto time_now = system_clock::now();
	//auto durationIn = duration_cast<seconds>(time_now.time_since_epoch());

	//I_LOG("create localPort4Rtp");
	//localSocket4Rtp = new udp::socket(ioService, udp::endpoint(udp::v4(), localPort4Rtp));
	//I_LOG("create localPort4Rtp success");
	////remoteEndpoint4Rtp = udp::endpoint(ip::address_v4::from_string(remoteIp), remotePort);

 //   stopFlag = true;
	//do_data_recv();
	//I_LOG("do data recv");
}

void rtpServer::start() {

    uint8_t recv_buf[BUFF_SIZE]{ 0 };
    bool loop = true;
    int aac_frame_size = 0;
    int count = 0;
    int payload_offset = 0;
    int payload_type = 0;

    int32_t ssrc;
    std::size_t recvLen;

    RtpParse *parser = new RtpParse();
    ConvertH264Util* ch = nullptr;
    list<std::pair<uint8_t*, int>> pktList;
    condition_variable pktCond;
    mutex recvPktMtx;

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
                tempPktList.push_back(pkt);
            }
            lock.unlock();

            for (auto it = tempPktList.begin(); it != tempPktList.end(); ++it) {

                AVFrame* frame = av_frame_alloc();
                I_LOG("it len:{}", it->second);
                frame->width = 858;
                frame->height = 480;
                frame->format = AV_PIX_FMT_RGB24;
                uint8_t* frame_buffer_out = (uint8_t*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB24, 858, 480, 1));
                memcpy(frame_buffer_out, it->first, it->second);
                int ret = av_image_fill_arrays(frame->data, frame->linesize, frame_buffer_out, AV_PIX_FMT_RGB24, 858, 480, 1);
                if (ret != 0 || frame->width <= 0 || frame->height <= 0) {
                    I_LOG("no :{}, width:{}", ret, frame->width);
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
        }
    };

    std::thread decodeThread{ decodeThreadFunc };
    decodeThread.detach();

    while (loop) {
        udp::endpoint remote_endpoint;
        recvLen = server.receive_from(asio::buffer(recv_buf), remote_endpoint); //×èÈû

        I_LOG("here: {}", recvLen);

        if (recvLen > 0) {

            I_LOG("GET!");


            //todo parsing rtp

            int ret = parser->ParseH264Packet(recv_buf, recvLen);
            I_LOG("RET 1:{}", ret);

            auto parseCallBack = [&](uint8_t* EsFrame, int len, void* userdata) {
                I_LOG("recv data");
                recvPktMtx.lock();
                uint8_t* temp = (uint8_t*)malloc(len);
                memcpy(temp, EsFrame, len);
                pktList.push_back(std::pair<uint8_t*, int>(temp, len));
                recvPktMtx.unlock();
                pktCond.notify_one();

            };
            parser->RtpSetDataCallCallback(parseCallBack, parser);

        }


        /*ssrc = Utils::parsingRTPPacket(recv_buf, recvLen, &payload_offset, &payload_type);
        if (ssrc < 0) {
            W_LOG("parsingRTPPacket ssrc wrong...");
            continue;
        }*/

        //if(flag)
        //    RM.putUserAudioBuf(ssrc, &recv_buf[payload_offset], recvLen - payload_offset, seeker::Time::currentTime());
        //else 
        //    RM.putUserVideoBuf(ssrc, &recv_buf[payload_offset], recvLen - payload_offset, seeker::Time::currentTime());

    }

    
}

//void rtpServer::do_data_recv() {
//    uint8_t* recv_data;
//    size_t length;
//    uint32_t ssrc;
//    if (!stopFlag)
//    {
//		localSocket4Rtp->async_receive_from(asio::buffer(recvBuf4Rtp), tempEndpoint4Rtp, std::bind(&rtpServer::handleData4Rtp, this,
//			std::placeholders::_1,
//			std::placeholders::_2));
//    }
//    return;
//}
//
//void rtpServer::handleData4Rtp(const asio::error_code error, size_t bytes_transferred) {
//
//
//	if (error.value() != 0) {
//		I_LOG("handle client data error:{}, code:{}, rtpPort:{}", error.message(), error.value(), localPort4Rtp);
//	}
//
//	if (!stopFlag) {
//		localSocket4Rtp->async_receive_from(asio::buffer(recvBuf4Rtp), tempEndpoint4Rtp, std::bind(&rtpServer::handleData4Rtp, this,
//			std::placeholders::_1,
//			std::placeholders::_2));
//	}
//
//}