#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <Winsock2.h>
#include <ws2ipdef.h>

#include "rtpParse.h"


int RtpParse::parsingRTPPacket(uint8_t* data, std::size_t size, int* payload_offset, int* payloadType)
{
    if (size < 12) {
        //Too short to be a valid RTP header.
        printf("1\n");
        return -1;
    }

    if ((data[0] >> 6) != 2) {
        //Currently, the version is 2, if is not 2, unsupported.
        printf("2\n");
        return -1;
    }

    if (data[0] & 0x20) {
        // Padding present.
        size_t paddingLength = data[size - 1];
        if (paddingLength + 12 > size) {
            printf("3\n");
            return -1;
        }
        size -= paddingLength;
    }

    int numCSRCs = data[0] & 0x0f;
    size_t payloadOffset = 12 + 4 * numCSRCs;

    if (size < payloadOffset) {
        // Not enough data to fit the basic header and all the CSRC entries.
        printf("4\n");
        return -1;
    }

    if (data[0] & 0x10) {
        // Header extension present.
        if (size < payloadOffset + 4) {
            printf("5\n");
            // Not enough data to fit the basic header, all CSRC entries and the first 4 bytes of the extension header.
            return -1;
        }

        const uint8_t* extensionData = &data[payloadOffset];
        size_t extensionLength = 4 * (extensionData[2] << 8 | extensionData[3]);

        if (size < payloadOffset + 4 + extensionLength) {
            printf("6\n");
            return -1;
        }
        payloadOffset += (4 + extensionLength);
    }


    *payloadType = data[1] & 0x7f;

    //    T_LOG("payload type is {}",*payloadType);

    *payload_offset = payloadOffset;

    uint32_t rtpTime = data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7];
    uint32_t srcId = data[8] << 24 | data[9] << 16 | data[10] << 8 | data[11];
    uint32_t seqNum = data[2] << 8 | data[3];

    return srcId;
}

