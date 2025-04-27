#ifndef P1_ASCII_DECODER_H
#define P1_ASCII_DECODER_H

#include "p1data.h"
#include "IFrameData.h"

class P1AsciiDecoder {
public:
    P1AsciiDecoder();

    /**
     * @brief Decodes a buffer containing a P1 ASCII telegram.
     * 
     * Parses the ASCII lines, extracts OBIS data and timestamp, and populates the P1Data object.
     * 
     * @param frame An IFrameData object containing the raw ASCII frame data.
     * @param p1data A P1Data object to populate with the decoded data.
     * @return true if the buffer was successfully decoded (at least partially), false otherwise.
     */
    bool decodeBuffer(const IFrameData& frame, P1Data& p1data);

private:
    // Helper function to parse the timestamp line (e.g., 0-0:1.0.0(YYMMDDhhmmssX))
    bool parseTimestamp(const char* line, P1Data& p1data);

    // Helper function to parse a standard OBIS data line
    bool parseObisLine(const char* line, P1Data& p1data);
    
    // Helper function to calculate and verify the CRC checksum (optional)
    // bool verifyChecksum(const IFrameData& frame); 
};

#endif // P1_ASCII_DECODER_H
