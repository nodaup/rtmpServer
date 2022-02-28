#include "rtpServer.h"
#include "rtpParse.h"
#include <thread>
#include <seeker/loggerApi.h>
using namespace std;
using std::mutex;


rtpServer::rtpServer(const std::string& ip, int port, bool is_audio)
    : server{ ioContext, udp::endpoint(udp::v4(), port) } {
    I_LOG("asio server inits, ip is {} , udp port is {}", ip, port);
    flag = is_audio;
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

    RtpParse* parser = new RtpParse();

    while (loop) {
        udp::endpoint remote_endpoint;
        recvLen = server.receive_from(asio::buffer(recv_buf), remote_endpoint); //×èÈû

        //I_LOG("here: {}", recvLen);

        if (recvLen > 0) {

            //I_LOG("GET!");

            //todo parsing rtp
            ssrc = parser->parsingRTPPacket(recv_buf, recvLen, &payload_offset, &payload_type);
            if (ssrc < 0) {
                I_LOG("parsingRTPPacket ssrc wrong...");
                continue;
            }

            //I_LOG("recv data");
            
            auto temp = new uint8_t[recvLen - payload_offset + 1]();
            memcpy(temp, &recv_buf[payload_offset], recvLen - payload_offset);
            callBack(temp, recvLen - payload_offset, ssrc, parser->ts, parser->seqnum, parser->pt);
        
        }
    }

    
}


void rtpServer::send(uint8_t* packet, int len) {

    if (packet) {
        
        uint8_t temp[1024]{ 0 };
        memcpy(temp, packet, len);
        //remote ip port rtmp://192.168.31.154:1935
        udp::endpoint remote_endpoint(ip::address_v4::from_string("10.28.197.125"), 1935);
        server.send_to(asio::buffer(temp), remote_endpoint);
        free(temp);
    }
}

void rtpServer::setCallBack(RecvDataCallback _callBack) {
    callBack = _callBack;
}
