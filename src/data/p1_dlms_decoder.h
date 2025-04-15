#ifndef P1_DLMS_DECODER_H
#define P1_DLMS_DECODER_H

// #include <Arduino.h>
#include <cstring>
#include <cstdint>
#include "p1data.h"

// Debug logging control
// Comment out to disable all debug logs
//#define P1_DLMS_SERIAL_DEBUG

#ifdef P1_DLMS_SERIAL_DEBUG
  #define P1_DLMS_LOG(x) Serial.x
  #define P1_DLMS_LOG_FUNCTION(x) x
#else
  #define P1_DLMS_LOG(x)
  #define P1_DLMS_LOG_FUNCTION(x)
#endif

class P1DLMSDecoder {
public:
    // Constructor
    P1DLMSDecoder();
    
    // Main decode function - decodes a complete P1 frame
    bool decodeBuffer(const uint8_t* buffer, size_t length, P1Data& p1data);
    
    // Static OBIS codes (text format)
    static const char* OBIS_ELECTRICITY_DELIVERED_TARIFF1;
    static const char* OBIS_ELECTRICITY_DELIVERED_TARIFF2;
    static const char* OBIS_ELECTRICITY_RETURNED_TARIFF1;
    static const char* OBIS_ELECTRICITY_RETURNED_TARIFF2;
    static const char* OBIS_CURRENT_POWER_DELIVERY;
    static const char* OBIS_CURRENT_POWER_RETURN;
    static const char* OBIS_DEVICE_ID;
    static const char* OBIS_GAS_DELIVERED;
    static const char* OBIS_VOLTAGE_L1;
    static const char* OBIS_VOLTAGE_L2;
    static const char* OBIS_VOLTAGE_L3;
    static const char* OBIS_CURRENT_L1;
    static const char* OBIS_CURRENT_L2;
    static const char* OBIS_CURRENT_L3;
    
private:
    // Binary DLMS format decoding
    bool decodeBinaryBuffer(const uint8_t* buffer, size_t length, P1Data& p1data);
    
    // Text format decoding
    bool decodeTextBuffer(const uint8_t* buffer, size_t length, P1Data& p1data);
    // bool processTextLine(const String& line, P1Data& p1data);
    
    // Helper functions
    // String extractValue(const String& line);
    // float stringToFloat(const String& valueStr);
    
    // Binary format helpers
    uint16_t swap_uint16(uint16_t val);
    uint32_t swap_uint32(uint32_t val);
    const char* getObisDescription(const uint8_t* obisCode);
    float extractNumericValue(const uint8_t* buffer, int position, uint8_t dataType, size_t length);
    int getDataTypeSize(uint8_t dataType);
    bool processObisValue(const uint8_t* obisCode, const uint8_t* buffer, int position, 
                          uint8_t dataType, size_t length, P1Data& p1data);
};

#endif // P1_DLMS_DECODER_H
