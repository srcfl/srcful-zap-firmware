#pragma once

#include "data/circular_buffer.h"
#include <vector>
#include <cstdint> // For uint8_t

#include "data/decoding/IFrameData.h"
#include "data/frame_info.h"

class MBusDecoder {
private:
    bool _frameInProgress = false;
    size_t _frameStartIndex = 0;
    unsigned long _interFrameTimeout = 0;

    bool findNextFrameStart(const CircularBuffer& buffer, size_t& startPos);
    bool extractCompleteFrame(const CircularBuffer& buffer, FrameInfo& frameInfo);
    bool extractVariableLengthFrame(const CircularBuffer& buffer, FrameInfo& frameInfo);
    uint8_t getByteAtOffset(const CircularBuffer& buffer, size_t startIndex, size_t offset);
    bool validateChecksum(const CircularBuffer& buffer, size_t startIndex, size_t dataLength, uint8_t expectedFcs);

public:
    bool detect(const CircularBuffer& buffer, unsigned long currentTime, FrameInfo& frameInfo);
    void reset();
    
    void setInterFrameTimeout(unsigned long timeout) { _interFrameTimeout = timeout; }
};