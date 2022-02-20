#include "rtpServer.h"
#include "rtpParse.h"
#include <thread>
#include <seeker/loggerApi.h>
#include "manager.h"
using namespace std;
using std::mutex;


rtpServer::rtpServer(const std::string& ip, int port, bool is_audio)
    : server{ ioContext, udp::endpoint(udp::v4(), port) } {
    I_LOG("asio server inits, ip is {} , udp port is {}", ip, port);
    flag = is_audio;
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

    Manager*m = new Manager();
    m->init();

    while (loop) {
        udp::endpoint remote_endpoint;
        recvLen = server.receive_from(asio::buffer(recv_buf), remote_endpoint); //×èÈû

        I_LOG("here: {}", recvLen);

        if (recvLen > 0) {

            I_LOG("GET!");

            //todo parsing rtp
            ssrc = RtpParse::parsingRTPPacket(recv_buf, recvLen, &payload_offset, &payload_type);
            if (ssrc < 0) {
                W_LOG("parsingRTPPacket ssrc wrong...");
                continue;
            }

            I_LOG("recv data");   
            auto temp = new uint8_t[recvLen - payload_offset + 1]();
            memcpy(temp, &recv_buf[payload_offset], recvLen - payload_offset);  
            m->saveRtpPkt(temp, recvLen - payload_offset);


        }
    }

    
}