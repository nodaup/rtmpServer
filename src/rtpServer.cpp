#include "rtpServer.h"

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
    while (loop) {
        udp::endpoint remote_endpoint;
        recvLen = server.receive_from(asio::buffer(recv_buf), remote_endpoint); //×èÈû

        T_LOG("here: {}", recvLen);

        if (recvLen > 0) {

            I_LOG("GET!");
            //todo parsing rtp
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