#include "data/decoding/mbus_decoder.h"
#include "data/decoding/dlms_decoder.h"
#include <cmath>

bool MBusDecoder::decodeBuffer(const IFrameData& frame, P1Data& p1data) {

    return false;

    size_t frameSize = frame.getFrameSize();
    if (frameSize < 6) {
        return false;
    }

    // MBus frame structure validation
    // Check for start sequence 0x68, length, length, 0x68
    if (frame.getFrameByte(0) != 0x68) {
        return false;
    }

    uint8_t length = frame.getFrameByte(1);
    if (frame.getFrameByte(2) != length || frame.getFrameByte(3) != 0x68) {
        return false;
    }

    // ok we check the first byte not part of the MBus frame and see if it is the dlsm start byte 0x0F
    DLMSDecoder dlmsDecoder;
    if (frame.getFrameByte(11) != 0x0F) {
        return false; // Not a DLMS frame
    }

    // this is likely very specific for the NöNetz P1 meter: https://www.netz-noe.at/Download-(1)/Smart-Meter/218_9_SmartMeter_Kundenschnittstelle_lektoriert_14.aspx
    // MBus header 4 bytes + 7 bytes of MBUS stuff 
    // 8 bytes system title (?) + 3 bytes of something + 4 bytes frame counter  - this is part of the DLSM encryption and security, 
    // actual data
    if (frameSize < 26 + 2) {
        return false; // Not enough data for a complete frame
    }
    return dlmsDecoder.decodeBuffer(frame, p1data, 26);
}

// Helper function for BCD conversion (improved)
uint32_t bcdToInt(uint64_t bcd, size_t length) {
    uint32_t result = 0;
    uint32_t multiplier = 1;
    
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = (bcd >> (i * 8)) & 0xFF;
        uint8_t low = byte & 0x0F;
        uint8_t high = (byte >> 4) & 0x0F;
        
        if (low > 9 || high > 9) {
            return 0; // Invalid BCD
        }
        
        result += low * multiplier;
        multiplier *= 10;
        result += high * multiplier;
        multiplier *= 10;
    }
    
    return result;
}