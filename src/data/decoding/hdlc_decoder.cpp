#include "hdlc_decoder.h"

// DLMS/OBIS binary format defines
#define HDLC_FRAME_FLAG 0x7E
#define HDLC_DECODER_START_OFFSET 20

#include "../../zap_log.h" // Added for logging

// Define TAG for logging
static const char* TAG = "hdlc_decoder";

#include "dlms_decoder.h"

// Constructor
HDLCDecoder::HDLCDecoder() {
    // Nothing to initialize currently
}


uint16_t HDLCDecoder::_ntohs(uint16_t netshort) {
    // Split into bytes and reconstruct in reverse order
    return ((netshort & 0xFF00) >> 8) | 
           ((netshort & 0x00FF) << 8);
}

bool HDLCDecoder::decodeBuffer(const IFrameData& frame, P1Data& p1data) {
    // Validate frame
    if (frame.getFrameByte(0) != HDLC_FRAME_FLAG || frame.getFrameByte(frame.getFrameSize() -1) != HDLC_FRAME_FLAG) {
        LOG_E(TAG, "Invalid frame start/end");
        return false;
    }
    if (frame.getFrameSize() < HDLC_DECODER_START_OFFSET) {
        LOG_E(TAG, "Frame too short");
        return false;
    }

    bool dataFound = false;
    int currentPos = HDLC_DECODER_START_OFFSET;

    
    HDLCHeader header;
    header.bytes[0] = frame.getFrameByte(0);
    header.bytes[1] = frame.getFrameByte(1);
    header.bytes[2] = frame.getFrameByte(2);

    // check that it is type 3 frame
    if((header.format & 0xF0) != 0xA0) {
        LOG_E(TAG, "Invalid frame format");
        return false; // Invalid frame format
    }

    int len = (_ntohs(header.format) & 0x7FF) + 2;
    if(len > frame.getFrameSize()) {
        LOG_E(TAG, "Invalid frame length");
        return false; // Invalid frame length
    }

    currentPos = 3; // Skip the first byte (frame start) and header 3 bytes
    // Skip destination and source address, LSB marks last byte
    while((frame.getFrameByte(currentPos) & 0x01) == 0x00) {
        currentPos++;
    }
    currentPos++;
    while((frame.getFrameByte(currentPos) & 0x01) == 0x00) {
        currentPos++;
    }
    currentPos++;

    // Skip confrol field, HCS abd LLC
    currentPos += 3 + 3;

    DLMSDecoder dlmsDecoder;

    return dlmsDecoder.decodeBuffer(frame, p1data, currentPos);
}