#pragma once

#include "data/circular_buffer.h"
#include <vector>

#include "data/decoding/IFrameData.h"
#include "data/decoding/p1data.h"

class MBusDecoder {
private:
   

public:
    /**
     * @brief Decode a buffer containing M-Bus data
     * 
     * @param frame The frame data to decode
     * @param p1data Reference to P1Data to store the decoded values
     * @return true if the buffer was successfully decoded, false otherwise
     */
    bool decodeBuffer(const IFrameData& frame, P1Data& p1data);

};