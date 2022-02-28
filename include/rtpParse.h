#pragma once
#include <functional>


class RtpParse
{

public:

    uint32_t ts;
    uint32_t ssrc;
    uint32_t seqnum;
    uint32_t pt;

    int parsingRTPPacket(uint8_t* data, std::size_t size, int* payload_offset, int* payloadType);
    
};