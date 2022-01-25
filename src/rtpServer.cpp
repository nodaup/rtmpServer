#include "rtpServer.h"

rtpServer::rtpServer(std::string destIp, int destPort, int portbase, 
	uint32_t recvSsrc, uint32_t sendSsrc, io_service& io) : ioService(io)
{
	
	auto time_now = system_clock::now();
	auto durationIn = duration_cast<seconds>(time_now.time_since_epoch());

	I_LOG("create localPort4Rtp");
	localSocket4Rtp = new udp::socket(ioService, udp::endpoint(udp::v4(), localPort4Rtp));
	I_LOG("create localPort4Rtp success");
	//remoteEndpoint4Rtp = udp::endpoint(ip::address_v4::from_string(remoteIp), remotePort);

    stopFlag = true;
	do_data_recv();
	I_LOG("do data recv");

}

void rtpServer::do_data_recv() {
    uint8_t* recv_data;
    size_t length;
    uint32_t ssrc;
    if (!stopFlag)
    {
		localSocket4Rtp->async_receive_from(asio::buffer(recvBuf4Rtp), tempEndpoint4Rtp, std::bind(&rtpServer::handleData4Rtp, this,
			std::placeholders::_1,
			std::placeholders::_2));
    }
    return;
}

void rtpServer::handleData4Rtp(const asio::error_code error, size_t bytes_transferred) {


	if (error.value() != 0) {
		I_LOG("handle client data error:{}, code:{}, rtpPort:{}", error.message(), error.value(), localPort4Rtp);
	}

	if (!stopFlag) {
		localSocket4Rtp->async_receive_from(asio::buffer(recvBuf4Rtp), tempEndpoint4Rtp, std::bind(&rtpServer::handleData4Rtp, this,
			std::placeholders::_1,
			std::placeholders::_2));
	}

}

rtpServer::~rtpServer() {
	I_LOG("~rtpServer");
}