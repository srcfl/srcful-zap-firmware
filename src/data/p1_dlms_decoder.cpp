#include "p1_dlms_decoder.h"
#include <time.h>
#include <cstdio> 


#ifdef P1_DLMS_SERIAL_DEBUG
    #include <Arduino.h>
#endif

// Define the static OBIS codes https://onemeter.com/docs/device/obis/
const char* P1DLMSDecoder::OBIS_ELECTRICITY_DELIVERED_TARIFF1 = "1-0:1.8.1";
const char* P1DLMSDecoder::OBIS_ELECTRICITY_DELIVERED_TARIFF2 = "1-0:1.8.2";
const char* P1DLMSDecoder::OBIS_ELECTRICITY_RETURNED_TARIFF1 = "1-0:2.8.1";
const char* P1DLMSDecoder::OBIS_ELECTRICITY_RETURNED_TARIFF2 = "1-0:2.8.2";
const char* P1DLMSDecoder::OBIS_CURRENT_POWER_DELIVERY = "1-0:1.7.0";
const char* P1DLMSDecoder::OBIS_CURRENT_POWER_RETURN = "1-0:2.7.0";
const char* P1DLMSDecoder::OBIS_DEVICE_ID = "0-0:96.1.1";
const char* P1DLMSDecoder::OBIS_GAS_DELIVERED = "0-1:24.2.1";
const char* P1DLMSDecoder::OBIS_VOLTAGE_L1 = "1-0:32.7.0";
const char* P1DLMSDecoder::OBIS_VOLTAGE_L2 = "1-0:52.7.0";
const char* P1DLMSDecoder::OBIS_VOLTAGE_L3 = "1-0:72.7.0";
const char* P1DLMSDecoder::OBIS_CURRENT_L1 = "1-0:31.7.0";
const char* P1DLMSDecoder::OBIS_CURRENT_L2 = "1-0:51.7.0";
const char* P1DLMSDecoder::OBIS_CURRENT_L3 = "1-0:71.7.0";

// DLMS/OBIS binary format defines
#define FRAME_FLAG 0x7E
#define DECODER_START_OFFSET 20

// Data types
#define DATA_NULL 0x00
#define DATA_OCTET_STRING 0x09
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


struct CDUnit {
    uint8_t C;
    uint8_t D;
    P1Data::OBISUnit unit;
};

static const CDUnit OBIS_10_CD_UNIT[]= {
    { 1, 8, P1Data::OBISUnit::kWH},    // 1-0:1.8.0*255 kWh Active Energy Plus
    { 2, 8, P1Data::OBISUnit::kWH},    // 1-0:2.8.0*255 kWh Active Energy Minus
    { 3, 8, P1Data::OBISUnit::kVARH}, // 1-0:3.8.0*255 kvarh Reactive Energy Plus
    { 4, 8, P1Data::OBISUnit::kVARH}, // 1-0:4.8.0*255 kvarh Reactive Energy Minus
    { 1, 7, P1Data::OBISUnit::kW},    // 1-0:1.7.0*255 kW Active Power Plus
    { 2, 7, P1Data::OBISUnit::kW},    // 1-0:2.7.0*255 kW Active Power Minus
    { 3, 7, P1Data::OBISUnit::kVAR},  // 1-0:3.7.0*255 kvar Reactive Power Plus
    { 4, 7, P1Data::OBISUnit::kVAR},  // 1-0:4.7.0*255 kvar Reactive Power Minus
    {21, 7, P1Data::OBISUnit::kW},    // 1-0:21.7.0*255 kW Active Power Plus L1
    {41, 7, P1Data::OBISUnit::kW},    // 1-0:41.7.0*255 kW Active Power Plus L2
    {61, 7, P1Data::OBISUnit::kW},    // 1-0:61.7.0*255 kW Active Power Plus L3
    {22, 7, P1Data::OBISUnit::kW},    // 1-0:22.7.0*255 kW Active Power Minus L1
    {42, 7, P1Data::OBISUnit::kW},    // 1-0:42.7.0*255 kW Active Power Minus L2
    {62, 7, P1Data::OBISUnit::kW},    // 1-0:62.7.0*255 kW Active Power Minus L3

    {23, 7, P1Data::OBISUnit::kW},    // 1-0:21.7.0*255 kW Reactive Power Plus L1
    {43, 7, P1Data::OBISUnit::kW},    // 1-0:41.7.0*255 kW Reactive Power Plus L2
    {63, 7, P1Data::OBISUnit::kW},    // 1-0:61.7.0*255 kW Reactive Power Plus L3
    {24, 7, P1Data::OBISUnit::kW},    // 1-0:22.7.0*255 kW Reactive Power Minus L1
    {44, 7, P1Data::OBISUnit::kW},    // 1-0:42.7.0*255 kW Reactive Power Minus L2
    {64, 7, P1Data::OBISUnit::kW},    // 1-0:62.7.0*255 kW Reactive Power Minus L3

    {32, 7, P1Data::OBISUnit::VOLT},  // 1-0:32.7.0*255 V Voltage L1
    {52, 7, P1Data::OBISUnit::VOLT},  // 1-0:52.7.0*255 V Voltage L2
    {72, 7, P1Data::OBISUnit::VOLT},  // 1-0:72.7.0*255 V Voltage L3
    {31, 7, P1Data::OBISUnit::AMP},   // 1-0:31.7.0*255 A Current L1
    {51, 7, P1Data::OBISUnit::AMP},   // 1-0:51.7.0*255 A Current L2
    {71, 7, P1Data::OBISUnit::AMP}    // 1-0:71.7.0*255 A Current L3
};

P1Data::OBISUnit getObisUnit(uint8_t C, uint8_t D) {
    for (const auto& obis : OBIS_10_CD_UNIT) {
        if (obis.C == C && obis.D == D) {
            return obis.unit;
        }
    }
    return P1Data::OBISUnit::UNKNOWN;
}


// static const uint8_t OBIS_ACTIVE_ENERGY_PLUS_HEX[]      = {0x01, 0x08, }; // 1-0:1.8.0*255 kWh
// static const uint8_t OBIS_ACTIVE_ENERGY_MINUS_HEX[]     = {0x02, 0x08}; // 1-0:2.8.0*255 kWh
// static const uint8_t OBIS_REACTIVE_ENERGY_PLUS_HEX[]    = {0x03, 0x08}; // 1-0:3.8.0*255 kvarh
// static const uint8_t OBIS_REACTIVE_ENERGY_MINUS_HEX[]   = {0x04, 0x08}; // 1-0:4.8.0*255 kvarh

// static const uint8_t OBIS_ACTIVE_POWER_PLUS_HEX[]       = {0x01, 0x07}; // 1-0:1.7.0*255 kW
// static const uint8_t OBIS_ACTIVE_POWER_MINUS_HEX[]      = {0x02, 0x07}; // 1-0:2.7.0*255 kW

// static const uint8_t OBIS_REACTIVE_POWER_PLUS_HEX[]     = {0x03, 0x07}; // 1-0:3.7.0*255 kvar
// static const uint8_t OBIS_REACTIVE_POWER_MINUS_HEX[]    = {0x04, 0x07}; // 1-0:4.7.0*255 kvar


// static const uint8_t OBIS_ACTIVE_POWER_PLUS_L1_HEX[]   = {0x15, 0x07}; // 1-0:21.7.0*255 kW
// static const uint8_t OBIS_ACTIVE_POWER_PLUS_L2_HEX[]   = {0x16, 0x07}; // 1-0:41.7.0*255 kW
// static const uint8_t OBIS_ACTIVE_POWER_PLUS_L3_HEX[]   = {0x17, 0x07}; // 1-0:61.7.0*255 kW
// static const uint8_t OBIS_ACTIVE_POWER_MINUS_L1_HEX[]  = {0x18, 0x07}; // 1-0:22.7.0*255 kW
// static const uint8_t OBIS_ACTIVE_POWER_MINUS_L2_HEX[]  = {0x19, 0x07}; // 1-0:42.7.0*255 kW
// static const uint8_t OBIS_ACTIVE_POWER_MINUS_L3_HEX[]  = {0x1A, 0x07}; // 1-0:62.7.0*255 kW

// static const uint8_t OBIS_VOLTAGE_L1_HEX[]              = {0x20, 0x07}; // 1-0:32.7.0*255
// static const uint8_t OBIS_VOLTAGE_L2_HEX[]              = {0x34, 0x07}; // 1-0:52.7.0*255
// static const uint8_t OBIS_VOLTAGE_L3_HEX[]              = {0x48, 0x07}; // 1-0:72.7.0*255
// static const uint8_t OBIS_CURRENT_L1_HEX[]              = {0x1F, 0x07}; // 1-0:31.7.0*255
// static const uint8_t OBIS_CURRENT_L2_HEX[]              = {0x33, 0x07}; // 1-0:51.7.0*255
// static const uint8_t OBIS_CURRENT_L3_HEX[]              = {0x47, 0x07}; // 1-0:71.7.0*255

P1DLMSDecoder::P1DLMSDecoder() {
    // Constructor implementation - nothing to initialize currently
}

// Helper functions for binary decoding
uint16_t P1DLMSDecoder::swap_uint16(uint16_t val) {
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

uint32_t P1DLMSDecoder::swap_uint32(uint32_t val) {
    return ((val & 0xFF) << 24) | 
           (((val >> 8) & 0xFF) << 16) |
           (((val >> 16) & 0xFF) << 8) |
           ((val >> 24) & 0xFF);
}

// For text format decoding (kept for backward compatibility)
// String P1DLMSDecoder::extractValue(const String& line) {
//     int startPos = line.indexOf('(');
//     int endPos = line.indexOf(')');
//     if (startPos != -1 && endPos != -1 && endPos > startPos) {
//         return line.substring(startPos + 1, endPos);
//     }
//     return "";
// }

// float P1DLMSDecoder::stringToFloat(const String& valueStr) {
//     if (valueStr.length() == 0) {
//         return 0.0f;
//     }
    
//     return valueStr.toFloat();
// }

bool is_obis_cd(const uint8_t* obisCode, const uint8_t* pattern) {
    return (obisCode[OBIS_C] == pattern[0] && obisCode[OBIS_D] == pattern[1]);
}

// For debugging - get readable description of OBIS code
// const char* P1DLMSDecoder::getObisDescription(const uint8_t* obisCode) {
//     // Check for known OBIS codes by comparing C,D values
//     if (obisCode[OBIS_A] == 0 && obisCode[OBIS_B] == 0 && 
//         obisCode[OBIS_C] == 0x01 && obisCode[OBIS_D] == 0x00)
//         return "Timestamp";
        
//     if (obisCode[OBIS_A] == 1 && obisCode[OBIS_B] == 0) {
//         if (is_obis_cd(obisCode, OBIS_ACTIVE_ENERGY_PLUS_HEX))
//             return "Active Energy Plus";
//         if (is_obis_cd(obisCode, OBIS_ACTIVE_ENERGY_MINUS_HEX))
//             return "Active Energy Minus";
//         if (is_obis_cd(obisCode, OBIS_REACTIVE_ENERGY_PLUS_HEX))
//             return "Reactive Energy Plus";
//         if (is_obis_cd(obisCode, OBIS_REACTIVE_ENERGY_MINUS_HEX))
//             return "Reactive Energy Minus";
//         if (is_obis_cd(obisCode, OBIS_ACTIVE_POWER_PLUS_HEX))
//             return "Active Power Plus";
//         if (is_obis_cd(obisCode, OBIS_ACTIVE_POWER_MINUS_HEX))
//             return "Active Power Minus";
//         if (is_obis_cd(obisCode, OBIS_REACTIVE_POWER_PLUS_HEX))
//             return "Reactive Power Plus";
//         if (is_obis_cd(obisCode, OBIS_REACTIVE_POWER_MINUS_HEX))
//             return "Reactive Power Minus";
//         if (is_obis_cd(obisCode, OBIS_VOLTAGE_L1_HEX))
//             return "Voltage L1";
//         if (is_obis_cd(obisCode, OBIS_VOLTAGE_L2_HEX))
//             return "Voltage L2";
//         if (is_obis_cd(obisCode, OBIS_VOLTAGE_L3_HEX))
//             return "Voltage L3";
//         if (is_obis_cd(obisCode, OBIS_CURRENT_L1_HEX))
//             return "Current L1";
//         if (is_obis_cd(obisCode, OBIS_CURRENT_L2_HEX))
//             return "Current L2";
//         if (is_obis_cd(obisCode, OBIS_CURRENT_L3_HEX))
//             return "Current L3";
//         if (is_obis_cd(obisCode, OBIS_ACTIVE_POWER_PLUS_L1_HEX))
//             return "Active Power L1";
//     }
    
//     // If not found, return NULL
//     return NULL;
// }

// Helper function to extract numeric value with appropriate scaling
float P1DLMSDecoder::extractNumericValue(const uint8_t* buffer, int position, uint8_t dataType, size_t length) {
    float result = 0.0f;
    int8_t scale = 0;
    uint8_t unit = 0;
    const float scaleFactors[10] = { 0.0001, 0.001, 0.01, 0.1, 1.0, 
        10.0, 100.0, 1000.0, 10000.0, 100000.0 };

    
    switch (dataType) {

        case DATA_INTEGER: { // 0x10 - 16-bit signed integer
            int16_t val;
            memcpy(&val, &buffer[position], 2);
            val = swap_uint16(val);
            result = static_cast<float>(val);
            position += 2;
            break;
        }
            
        case DATA_UNSIGNED: // 0x11 - 8-bit unsigned
        case DATA_LONG_UNSIGNED: { // 0x12 - 16-bit unsigned
            uint16_t val;
            memcpy(&val, &buffer[position], 2);
            val = swap_uint16(val);
            result = static_cast<float>(val);
            position += 2;
            break;
        }
            
        case DATA_LONG_DOUBLE_UNSIGNED: { // 0x06 - 32-bit unsigned
            uint32_t val;
            memcpy(&val, &buffer[position], 4);
            val = swap_uint32(val);
            result = static_cast<float>(val);
            position += 4;
            break;
        }
    }

    // For debugging - print several bytes after the value
    P1_DLMS_LOG(printf("    Bytes after value: "));
    for (int i = 0; i < 8 && position + i < length; i++) {
        P1_DLMS_LOG(printf("%02X ", buffer[position + i]));
    }
    P1_DLMS_LOG(println());
   
    // Check if we have a structure tag followed by scale factor
    // Parse the structure after the value
    if (position + 7 < length && buffer[position] == 0x02) {
        int structElements = buffer[position + 1];
        position += 2;  // Move past structure tag and element count
        
        // Parse inner structure elements
        for (int i = 0; i < structElements && position < length; i++) {
            uint8_t tag = buffer[position++];
            
            switch (tag) {
                case 0x0F:  // Scale factor
                    scale = (int8_t)buffer[position++];
                    P1_DLMS_LOG(printf("    Scale factor tag found: %d\n", scale));
                    break;
                    
                case 0x16:  // Unit
                    unit = buffer[position++];
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
int P1DLMSDecoder::getDataTypeSize(uint8_t dataType) {
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
bool P1DLMSDecoder::processObisValue(const uint8_t* obisCode, const uint8_t* buffer, int position, 
                                     uint8_t dataType, size_t length, P1Data& p1data) {
    bool known = false;
    
    // Print debug info
    P1_DLMS_LOG(printf("  [%04d] Data Type: 0x%02X\n", position-1, dataType));
    
    // Handle numeric values
    if (dataType == DATA_INTEGER || dataType == DATA_UNSIGNED || 
        dataType == DATA_LONG_UNSIGNED || dataType == DATA_LONG_DOUBLE_UNSIGNED) {
        
        int dataSize = getDataTypeSize(dataType);
        if (position + dataSize > length) {
            P1_DLMS_LOG(printf("    Error: Not enough data for type 0x%02X\n", dataType));
            return false;
        }
        
        float value = extractNumericValue(buffer, position, dataType, length);
        P1_DLMS_LOG(printf("    Value: %f\n", value));
        
        // Process based on OBIS code
        if (obisCode[OBIS_A] == 1 && obisCode[OBIS_B] == 0) {

            // Handle OBIS codes with C,D values
            p1data.obisValues[p1data.obisCount].C = obisCode[OBIS_C];
            p1data.obisValues[p1data.obisCount].D = obisCode[OBIS_D];
            p1data.obisValues[p1data.obisCount].value = value;
            p1data.obisValues[p1data.obisCount].unit = getObisUnit(obisCode[OBIS_C], obisCode[OBIS_D]);
            p1data.obisCount++;

            
        }
    }
    // Handle octet strings
    else if (dataType == DATA_OCTET_STRING) {
        if (position >= length) {
            P1_DLMS_LOG(println("    Error: Not enough data for octet string"));
            return false;
        }
        
        int dataLength = buffer[position++];
        if (position + dataLength > length) {
            P1_DLMS_LOG(printf("    Error: Not enough data for octet string of length %d\n", dataLength));
            return false;
        }
        
        // Handle timestamp
        if (dataLength == 12 && obisCode[OBIS_A] == 0 && obisCode[OBIS_B] == 0 && 
            obisCode[OBIS_C] == 1 && obisCode[OBIS_D] == 0) {
            uint16_t year = swap_uint16(*(uint16_t*)&buffer[position]);
            uint8_t month = buffer[position+2];
            uint8_t day = buffer[position+3];
            uint8_t hour = buffer[position+5];
            uint8_t minute = buffer[position+6];
            uint8_t second = buffer[position+7];
            
            struct tm timeinfo = {0};
            timeinfo.tm_year = year - 1900;
            timeinfo.tm_mon = month - 1;
            timeinfo.tm_mday = day;
            timeinfo.tm_hour = hour;
            timeinfo.tm_min = minute;
            timeinfo.tm_sec = second;
            
            p1data.timestamp = mktime(&timeinfo);
            P1_DLMS_LOG(printf("    Timestamp: %04d-%02d-%02d %02d:%02d:%02d\n", 
                          year, month, day, hour, minute, second));
            known = true;
        }
        // Handle device ID
        else if (obisCode[OBIS_A]==0 && obisCode[OBIS_B]==0 && obisCode[OBIS_C]==96 && obisCode[OBIS_D]==1) {
            if (dataLength < P1Data::BUFFER_SIZE) {
                memcpy(p1data.szDeviceId, &buffer[position], dataLength);
                p1data.szDeviceId[dataLength] = 0; // Null terminate
                P1_DLMS_LOG(printf("    Device ID: %s\n", p1data.szDeviceId));
                known = true;
            }
        }
        // Handle gas reading
        // else if (obisCode[OBIS_A]==0 && obisCode[OBIS_B]==1 && obisCode[OBIS_C]==24 && obisCode[OBIS_D]==2) {
        //     if (dataLength >= 6) {
        //         // Extract gas timestamp
        //         snprintf(p1data.szGasTimestamp, P1Data::BUFFER_SIZE, "%02x%02x%02x%02x%02x%02x", 
        //             buffer[position], buffer[position+1],
        //             buffer[position+2], buffer[position+3],
        //             buffer[position+4], buffer[position+5]);
        //         P1_DLMS_LOG(printf("    Gas Timestamp: %s\n", p1data.szGasTimestamp));
                
        //         // In a real implementation, you'd extract the actual gas value here
        //         // For now we'll just set a dummy value
        //         p1data.gasDelivered = 0.0;
        //         known = true;
        //     }
        // }
    }
    
    return known;
}

bool P1DLMSDecoder::decodeBinaryBuffer(const uint8_t* buffer, size_t length, P1Data& p1data) {
    // Validate frame
    if (buffer[0] != FRAME_FLAG || buffer[length -1] != FRAME_FLAG) {
        return false; // Invalid frame start/end
    }
    if (length < DECODER_START_OFFSET) {
        return false; // Buffer too short
    }

    bool dataFound = false;
    int currentPos = DECODER_START_OFFSET;
    
    P1_DLMS_LOG(println("\n--- Debug Decoding DLMS Frame ---"));
    
    while (currentPos < length - 10) {
        int startPos = currentPos;
        
        // Look for OBIS code marker (octet string of length 6)
        if (buffer[currentPos] == DATA_OCTET_STRING && buffer[currentPos + 1] == OBIS_CODE_LEN) {
            uint8_t obisCode[OBIS_CODE_LEN];
            memcpy(obisCode, &buffer[currentPos + 2], OBIS_CODE_LEN);
            const char* description = nullptr;
            
            P1_DLMS_LOG(printf("[%04d] OBIS: %d-%d:%d.%d.%d*%d (%s)\n", 
                currentPos, 
                obisCode[OBIS_A], obisCode[OBIS_B], obisCode[OBIS_C],
                obisCode[OBIS_D], obisCode[OBIS_E], obisCode[OBIS_F],
                description ? description : "Unknown"));
                
            // Move past OBIS code
            currentPos += 2 + OBIS_CODE_LEN;
            
            // Process the value if there's enough data
            if (currentPos < length) {
                uint8_t dataType = buffer[currentPos++];
                int dataSize = getDataTypeSize(dataType);
                
                // Process the value based on its type
                bool known = processObisValue(obisCode, buffer, currentPos, dataType, length, p1data);
                
                // Move position based on data type and length
                if (dataType == DATA_OCTET_STRING) {
                    if (currentPos < length) {
                        uint8_t strLen = buffer[currentPos];
                        currentPos += 1 + strLen;
                    }
                } else {
                    currentPos += dataSize;
                }
                
                if (known) dataFound = true;
                
                // Skip potential separator bytes
                if (currentPos < length - 1 && 
                    (buffer[currentPos] == 0x02 || buffer[currentPos] == 0x0F)) {
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
    
    // Set meter model if we have a device ID
    // if (strlen(p1data.szDeviceId) > 0 && strlen(p1data.szMeterModel) == 0) {
    //     strncpy(p1data.szMeterModel, "DLMS/COSEM", P1Data::BUFFER_SIZE);
    // }
    
    P1_DLMS_LOG(println("--- Decoding Frame Done ---"));
    return dataFound;
}

bool P1DLMSDecoder::decodeBuffer(const uint8_t* buffer, size_t length, P1Data& p1data) {
    // Try binary decoding first
    if (decodeBinaryBuffer(buffer, length, p1data)) {
        return true;
    }
    
    // Fall back to text decoding if binary fails
    return false; // decodeTextBuffer(buffer, length, p1data);
}