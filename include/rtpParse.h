#pragma once
#include <functional>


class RtpParse
{

public:

    static int parsingRTPPacket(uint8_t* data, std::size_t size, int* payload_offset, int* payloadType);
    
};