#pragma once

#include "data/circular_buffer.h"
#include <vector>
#include <cstdint> // For uint8_t

#include "data/decoding/IFrameData.h"
#include "data/frame_info.h"

class MbusFrameDetector {
public:

    /**
     * @brief Process a chunk of data and check if it completes a frame
     * 
     * @param buffer The circular buffer containing the data
     * @param currentTime Current time in milliseconds
     * @param frameInfo Output parameter to store detected frame information
     * @return true if a complete frame was detected
     */
    bool detect(
        const CircularBuffer& buffer,
        unsigned long currentTime,
        FrameInfo& frameInfo
    );

};