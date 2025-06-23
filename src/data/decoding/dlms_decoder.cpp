#include "dlms_decoder.h"
#include <time.h>
#include <cstdio> 


// Notes on format
// https://github.com/bmork/obinsect/blob/master/random-notes.md


#ifdef P1_DLMS_SERIAL_DEBUG
    #include <Arduino.h>
#endif

// Define the static OBIS codes https://onemeter.com/docs/device/obis/
const char* DLMSDecoder::OBIS_ELECTRICITY_DELIVERED_TARIFF1 = "1-0:1.8.1";
const char* DLMSDecoder::OBIS_ELECTRICITY_DELIVERED_TARIFF2 = "1-0:1.8.2";
const char* DLMSDecoder::OBIS_ELECTRICITY_RETURNED_TARIFF1 = "1-0:2.8.1";
const char* DLMSDecoder::OBIS_ELECTRICITY_RETURNED_TARIFF2 = "1-0:2.8.2";
const char* DLMSDecoder::OBIS_CURRENT_POWER_DELIVERY = "1-0:1.7.0";
const char* DLMSDecoder::OBIS_CURRENT_POWER_RETURN = "1-0:2.7.0";
const char* DLMSDecoder::OBIS_DEVICE_ID = "0-0:96.1.1";
const char* DLMSDecoder::OBIS_GAS_DELIVERED = "0-1:24.2.1";
const char* DLMSDecoder::OBIS_VOLTAGE_L1 = "1-0:32.7.0";
const char* DLMSDecoder::OBIS_VOLTAGE_L2 = "1-0:52.7.0";
const char* DLMSDecoder::OBIS_VOLTAGE_L3 = "1-0:72.7.0";
const char* DLMSDecoder::OBIS_CURRENT_L1 = "1-0:31.7.0";
const char* DLMSDecoder::OBIS_CURRENT_L2 = "1-0:51.7.0";
const char* DLMSDecoder::OBIS_CURRENT_L3 = "1-0:71.7.0";

// DLMS/OBIS binary format defines
#define FRAME_FLAG 0x7E
#define DECODER_START_OFFSET 20

// Data types
#define DATA_NULL 0x00
#define DATA_OCTET_STRING 0x09
#define DATA_STRING 0x0A
#define DATA_INTEGER 0x10
#define DATA_UNSIGNED 0x11
#define DATA_LONG_UNSIGNED 0x12
#define DATA_LONG_DOUBLE_UNSIGNED 0x06

#define OBIS_CODE_LEN 6 // Standard OBIS code length

// Scaling factors
#define SCALE_TENTHS 0xFF
#define SCALE_HUNDREDTHS 0xFE
#define SCALE_THOUSANDS 0xFD

// Indices within the 6-byte OBIS code
#define OBIS_A 0
#define OBIS_B 1
#define OBIS_C 2
#define OBIS_D 3
#define OBIS_E 4
#define OBIS_F 5

// Known OBIS binary C,D patterns, with unit https://onemeter.com/docs/device/obis/
static const uint8_t OBIS_TIMESTAMP_HEX[]               = {0x01, 0x00}; // 0-0:1.0.0*255

// New struct to map C, D codes to unit strings
struct CDUnitString {
    uint8_t C;
    uint8_t D;
    const char* unit;
};

// Map C, D codes to their corresponding unit strings
static const CDUnitString OBIS_10_CD_UNIT_STRINGS[]= {
    { 1, 8, "kWh"},    // 1-0:1.8.0*255 kWh Active Energy Plus
    { 2, 8, "kWh"},    // 1-0:2.8.0*255 kWh Active Energy Minus
    { 3, 8, "kVARh"}, // 1-0:3.8.0*255 kvarh Reactive Energy Plus
    { 4, 8, "kVARh"}, // 1-0:4.8.0*255 kvarh Reactive Energy Minus
    { 1, 7, "kW"},    // 1-0:1.7.0*255 kW Active Power Plus
    { 2, 7, "kW"},    // 1-0:2.7.0*255 kW Active Power Minus
    { 3, 7, "kVAR"},  // 1-0:3.7.0*255 kvar Reactive Power Plus
    { 4, 7, "kVAR"},  // 1-0:4.7.0*255 kvar Reactive Power Minus
    {21, 7, "kW"},    // 1-0:21.7.0*255 kW Active Power Plus L1
    {41, 7, "kW"},    // 1-0:41.7.0*255 kW Active Power Plus L2
    {61, 7, "kW"},    // 1-0:61.7.0*255 kW Active Power Plus L3
    {22, 7, "kW"},    // 1-0:22.7.0*255 kW Active Power Minus L1
    {42, 7, "kW"},    // 1-0:42.7.0*255 kW Active Power Minus L2
    {62, 7, "kW"},    // 1-0:62.7.0*255 kW Active Power Minus L3

    {23, 7, "kVAR"},    // 1-0:23.7.0*255 kVAR Reactive Power Plus L1
    {43, 7, "kVAR"},    // 1-0:43.7.0*255 kVAR Reactive Power Plus L2
    {63, 7, "kVAR"},    // 1-0:63.7.0*255 kVAR Reactive Power Plus L3
    {24, 7, "kVAR"},    // 1-0:24.7.0*255 kVAR Reactive Power Minus L1
    {44, 7, "kVAR"},    // 1-0:44.7.0*255 kVAR Reactive Power Minus L2
    {64, 7, "kVAR"},    // 1-0:64.7.0*255 kVAR Reactive Power Minus L3

    {32, 7, "V"},  // 1-0:32.7.0*255 V Voltage L1
    {52, 7, "V"},  // 1-0:52.7.0*255 V Voltage L2
    {72, 7, "V"},  // 1-0:72.7.0*255 V Voltage L3
    {31, 7, "A"},   // 1-0:31.7.0*255 A Current L1
    {51, 7, "A"},   // 1-0:51.7.0*255 A Current L2
    {71, 7, "A"}    // 1-0:71.7.0*255 A Current L3
};

// Function to get the unit string based on C and D codes
const char* getObisUnitString(uint8_t C, uint8_t D) {
    for (const auto& obis : OBIS_10_CD_UNIT_STRINGS) {
        if (obis.C == C && obis.D == D) {
            return obis.unit;
        }
    }
    return "UNKNOWN"; // Return "UNKNOWN" if no match found
}

DLMSDecoder::DLMSDecoder() {
    // Constructor implementation - nothing to initialize currently
}

// Helper functions for binary decoding
uint16_t DLMSDecoder::swap_uint16(uint16_t val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

uint32_t DLMSDecoder::swap_uint32(uint32_t val) {
    return ((val & 0xFF) << 24) | 
           (((val >> 8) & 0xFF) << 16) |
           (((val >> 16) & 0xFF) << 8) |
           ((val >> 24) & 0xFF);
}

bool is_obis_cd(const uint8_t* obisCode, const uint8_t* pattern) {
    return (obisCode[OBIS_C] == pattern[0] && obisCode[OBIS_D] == pattern[1]);
}

// Helper function to extract numeric value with appropriate scaling
float DLMSDecoder::extractNumericValue(const IFrameData& frame, int position, uint8_t dataType) {
    float result = 0.0f;
    int8_t scale = 0;
    uint8_t unit = 0;
    const float scaleFactors[10] = { 0.0001, 0.001, 0.01, 0.1, 1.0, 
        10.0, 100.0, 1000.0, 10000.0, 100000.0 };

    
    switch (dataType) {

        case DATA_INTEGER: { // 0x10 - 16-bit signed integer
            int16_t val = frame.getFrameByte(position) << 8 | frame.getFrameByte(position + 1);            
            result = static_cast<float>(val);
            position += 2;
            break;
        }
            
        case DATA_UNSIGNED: // 0x11 - 8-bit unsigned
        case DATA_LONG_UNSIGNED: { // 0x12 - 16-bit unsigned
            int16_t val = frame.getFrameByte(position) << 8 | frame.getFrameByte(position + 1);   
            result = static_cast<float>(val);
            position += 2;
            break;
        }
            
        case DATA_LONG_DOUBLE_UNSIGNED: { // 0x06 - 32-bit unsigned
            uint32_t val = frame.getFrameByte(position) << 24| 
                  (frame.getFrameByte(position + 1) << 16) | 
                  (frame.getFrameByte(position + 2) << 8) | 
                  (frame.getFrameByte(position + 3) << 0);
            result = static_cast<float>(val);
            position += 4;
            break;
        }
    }

    // For debugging - print several bytes after the value
    P1_DLMS_LOG(printf("    Bytes after value: "));
    for (int i = 0; i < 8 && position + i < frame.getFrameSize(); i++) {
        P1_DLMS_LOG(printf("%02X ", frame.getFrameByte(position + i)));
    }
    P1_DLMS_LOG(println());
   
    // Check if we have a structure tag followed by scale factor
    // Parse the structure after the value
    if (position + 7 < frame.getFrameSize() && frame.getFrameByte(position) == 0x02) {
        int structElements = frame.getFrameByte(position + 1);
        position += 2;  // Move past structure tag and element count
        
        // Parse inner structure elements
        for (int i = 0; i < structElements && position < frame.getFrameSize(); i++) {
            uint8_t tag = frame.getFrameByte(position++);
            
            switch (tag) {
                case 0x0F:  // Scale factor
                    scale = (int8_t)frame.getFrameByte(position++);
                    P1_DLMS_LOG(printf("    Scale factor tag found: %d\n", scale));
                    break;
                    
                case 0x16:  // Unit
                    unit = frame.getFrameByte(position++);
                    P1_DLMS_LOG(printf("    Unit tag found: 0x%02X\n", unit));
                    break;
                    
                default:
                    // Skip unknown tag and its value
                    position++;
                    P1_DLMS_LOG(printf("    Unknown tag: 0x%02X\n", tag));
                    break;
            }
        }
        
        // Apply scaling
        int scaleIndex = scale + 4;
        if (scaleIndex < 0) scaleIndex = 0;
        if (scaleIndex > 9) scaleIndex = 9;
        
        // Special case for kilowatt-hour and similar units
        if (scale == 0 && unit != 0x21 && unit != 0x23) {
            scale = -3;  // KILO prefix adjustment
            scaleIndex = scale + 4;
        }
        
        result = result * scaleFactors[scaleIndex];
        
        P1_DLMS_LOG(printf("    Final scale: %d (multiplier: %.5f)\n", scale, scaleFactors[scaleIndex]));
    }
    
    return result;
}

// Helper to get value size based on data type
int DLMSDecoder::getDataTypeSize(uint8_t dataType) {
    switch (dataType) {
        case DATA_NULL: return 0;
        case DATA_UNSIGNED: return 1;
        case DATA_INTEGER:
        case DATA_LONG_UNSIGNED: return 2;
        case DATA_LONG_DOUBLE_UNSIGNED: return 4;
        case DATA_OCTET_STRING: return -1; // Variable length
        default: return 0;
    }
}

// Process OBIS value and update P1Data accordingly
bool DLMSDecoder::processObisValue(const uint8_t* obisCode, const IFrameData& frame, int position, 
                                     uint8_t dataType, P1Data& p1data) {
    bool known = false;
    
    // Print debug info
    P1_DLMS_LOG(printf("  [%04d] Data Type: 0x%02X\n", position-1, dataType));
    
    // Handle numeric values
    if (dataType == DATA_INTEGER || dataType == DATA_UNSIGNED || 
        dataType == DATA_LONG_UNSIGNED || dataType == DATA_LONG_DOUBLE_UNSIGNED) {
        
        int dataSize = getDataTypeSize(dataType);
        if (position + dataSize > frame.getFrameSize()) {
            P1_DLMS_LOG(printf("    Error: Not enough data for type 0x%02X\n", dataType));
            return false;
        }
        
        float value = extractNumericValue(frame, position, dataType);
        P1_DLMS_LOG(printf("    Value: %f\n", value));
        
        // Process based on OBIS code
        if (obisCode[OBIS_A] == 1 && obisCode[OBIS_B] == 0) {
            // Handle OBIS codes with C,D values
            const char* unitStr = getObisUnitString(obisCode[OBIS_C], obisCode[OBIS_D]);
            if (p1data.addObisString(obisCode[OBIS_C], obisCode[OBIS_D], value, unitStr)) {
                known = true; // Successfully added
            } else {
                P1_DLMS_LOG(println("    Error: Could not add OBIS string to P1Data (buffer full?)"));
            }
        }
    }
    // Handle octet strings
    else if (dataType == DATA_OCTET_STRING) {
        if (position >= frame.getFrameSize()) {
            P1_DLMS_LOG(println("    Error: Not enough data for octet string"));
            return false;
        }
        
        int dataLength = frame.getFrameByte(position++);
        if (position + dataLength > frame.getFrameSize()) {
            P1_DLMS_LOG(printf("    Error: Not enough data for octet string of length %d\n", dataLength));
            return false;
        }
        
        // Handle timestamp
        if (dataLength == 12 && obisCode[OBIS_A] == 0 && obisCode[OBIS_B] == 0 && 
            obisCode[OBIS_C] == 1 && obisCode[OBIS_D] == 0) {
            uint16_t year = frame.getFrameByte(position) << 8 | frame.getFrameByte(position+1);
            uint8_t month = frame.getFrameByte(position+2);
            uint8_t day = frame.getFrameByte(position+3);
            uint8_t hour = frame.getFrameByte(position+5);
            uint8_t minute = frame.getFrameByte(position+6);
            uint8_t second = frame.getFrameByte(position+7);
            
            struct tm timeinfo = {0};
            timeinfo.tm_year = year - 1900;
            timeinfo.tm_mon = month - 1;
            timeinfo.tm_mday = day;
            timeinfo.tm_hour = hour;
            timeinfo.tm_min = minute;
            timeinfo.tm_sec = second;
            
            // convert to obis string time stamp string
            // TODO: investigate the W at the end of the string
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "0-0:1.0.0(%02d%02d%02d%02d%02d%02dW)",    // the W is not correct here
                    timeinfo.tm_year % 100, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            p1data.addObisString(buffer);

            P1_DLMS_LOG(printf("    Timestamp: %04d-%02d-%02d %02d:%02d:%02d\n", 
                          year, month, day, hour, minute, second));
            known = true;
        }
        // Handle device ID
        else if (obisCode[OBIS_A]==0 && obisCode[OBIS_B]==0 && obisCode[OBIS_C]==96 && obisCode[OBIS_D]==1) {
            if (dataLength < P1Data::DEVICE_ID_LEN) {
                // Copy bytes, ensuring null termination within the buffer limit
                int copyLen = (dataLength < P1Data::DEVICE_ID_LEN - 1) ? dataLength : P1Data::DEVICE_ID_LEN - 1;
                for (int i = 0; i < copyLen; i++) {
                    if (frame.getFrameByte(position + i) == 0x00) {
                        copyLen = i;
                        break;
                    }
                    p1data.szDeviceId[i] = frame.getFrameByte(position + i);
                }
                p1data.szDeviceId[copyLen] = '\0'; // Null terminate
                P1_DLMS_LOG(printf("    Device ID: %s\n", p1data.szDeviceId));
                known = true;
            } else {
                P1_DLMS_LOG(printf("    Warning: Device ID length (%d) exceeds buffer size (%d)\n", dataLength, P1Data::DEVICE_ID_LEN));
            }
        }
    } else if (dataType == DATA_STRING) {
        // Handle string values (if needed)
        known = false;
        P1_DLMS_LOG(println("    String value found, skipping..."));
    } else {
        known = false;
        P1_DLMS_LOG(printf("    Unknown data type: 0x%02X\n", dataType));
    }
    
    return known;
}

uint16_t crc16_x25(const IFrameData& frame, int position, int len)
{
    uint16_t crc = UINT16_MAX;

    for (int bytePos = 0; bytePos < len; bytePos++) {
        uint8_t d = frame.getFrameByte(position + bytePos);
        for (uint16_t i = 0; i < 8; i++, d >>= 1) {
            crc = ((crc & 1) ^ (d & 1)) ? (crc >> 1) ^ 0x8408 : (crc >> 1);
        }
    }

    return (~crc << 8) | (~crc >> 8 & 0xff);
}

uint16_t crc16(const IFrameData& frame, int position, int len) {
    uint16_t crc = 0;

    for (int bytePos = 0; bytePos < len; bytePos++) {
        uint8_t p = frame.getFrameByte(position + bytePos);
        crc ^= p;
        for (uint8_t i = 0; i < 8; ++i) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xa001;
            else
                crc = (crc >> 1);
        }
    }
    
    return crc;
}

uint16_t DLMSDecoder::_ntohs(uint16_t netshort) {
    // Split into bytes and reconstruct in reverse order
    return ((netshort & 0xFF00) >> 8) | 
           ((netshort & 0x00FF) << 8);
}


bool DLMSDecoder::decodeHDLCBuffer(const IFrameData& frame, P1Data& p1data) {
    // Validate frame
    if (frame.getFrameByte(0) != FRAME_FLAG || frame.getFrameByte(frame.getFrameSize() -1) != FRAME_FLAG) {
        return false; // Invalid frame start/end
    }
    if (frame.getFrameSize() < DECODER_START_OFFSET) {
        return false; // Buffer too short
    }

    bool dataFound = false;
    int currentPos = DECODER_START_OFFSET;

    
    HDLCHeader header;
    header.bytes[0] = frame.getFrameByte(0);
    header.bytes[1] = frame.getFrameByte(1);
    header.bytes[2] = frame.getFrameByte(2);

    // check that it is type 3 frame
    if((header.format & 0xF0) != 0xA0) {
        P1_DLMS_LOG(println("Invalid frame format"));
        return false; // Invalid frame format
    }

    int len = (_ntohs(header.format) & 0x7FF) + 2;
    if(len > frame.getFrameSize()) {
        P1_DLMS_LOG(println("Invalid frame length"));
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

    
    P1_DLMS_LOG(println("\n--- Debug Decoding DLMS Frame ---"));
    return decodeDLSM(frame, p1data, currentPos);
}
    

 bool DLMSDecoder::decodeDLSM(const IFrameData& frame, P1Data& p1data, const int startPos) {
    int currentPos = startPos;
    bool dataFound = false;
    while (currentPos < frame.getFrameSize() - 10) {
        int startPos = currentPos;
        
        // Look for OBIS code marker (octet string of length 6)
        if (frame.getFrameByte(currentPos) == DATA_OCTET_STRING && frame.getFrameByte(currentPos + 1) == OBIS_CODE_LEN) {
            uint8_t obisCode[OBIS_CODE_LEN];
            for (int i = 0; i < OBIS_CODE_LEN; i++) {
                obisCode[i] = frame.getFrameByte(currentPos + 2 + i);
            }
            const char* description = nullptr;
            
            P1_DLMS_LOG(printf("[%04d] OBIS: %d-%d:%d.%d.%d*%d (%s)\n", 
                currentPos, 
                obisCode[OBIS_A], obisCode[OBIS_B], obisCode[OBIS_C],
                obisCode[OBIS_D], obisCode[OBIS_E], obisCode[OBIS_F],
                description ? description : "Unknown"));
                
            // Move past OBIS code
            currentPos += 2 + OBIS_CODE_LEN;
            
            // Process the value if there's enough data
            if (currentPos < frame.getFrameSize()) {
                uint8_t dataType = frame.getFrameByte(currentPos++);
                int dataSize = getDataTypeSize(dataType);
                
                // Process the value based on its type
                bool known = processObisValue(obisCode, frame, currentPos, dataType, p1data);
                
                // Move position based on data type and length
                if (dataType == DATA_OCTET_STRING) {
                    if (currentPos < frame.getFrameSize()) {
                        uint8_t strLen = frame.getFrameByte(currentPos);
                        currentPos += 1 + strLen;
                    }
                } else {
                    currentPos += dataSize;
                }
                
                if (known) dataFound = true;
                
                // Skip potential separator bytes
                if (currentPos < frame.getFrameSize() - 1 && 
                    (frame.getFrameByte(currentPos) == 0x02 || frame.getFrameByte(currentPos) == 0x0F)) {
                    currentPos += 2;
                }
            }
        } else {
            // Not found OBIS structure, advance by one byte
            currentPos++;
        }
        
        // Safeguard against infinite loops
        if (currentPos == startPos) currentPos++;
    }
    
    P1_DLMS_LOG(println("--- Decoding Frame Done ---"));
    return dataFound;
}

bool DLMSDecoder::decodeBuffer(const IFrameData& frame, P1Data& p1data) {
    // Try binary decoding first
    if (decodeHDLCBuffer(frame, p1data)) {
        return true;
    }
    
    // Fall back to text decoding if binary fails
    return false; // decodeTextBuffer(buffer, length, p1data);
}