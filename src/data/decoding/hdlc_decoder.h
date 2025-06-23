#pragma once
// HDLC frame decoding, will hand off the frame data to the dlms decoder for the final processing

#include "p1data.h"
#include "IFrameData.h"


class HDLCDecoder {
public:
    // Constructor
    HDLCDecoder();
    
    // Main decode function - decodes a complete P1 frame
    bool decodeBuffer(const IFrameData& frame, P1Data& p1data);
    
private:

    struct HDLCHeader {
        union {
            struct {
                uint8_t  flag;
                uint16_t format;
            } __attribute__((packed));
            uint8_t bytes[3];
        };
        
    };
    
    uint16_t _ntohs(uint16_t netshort);
    
};