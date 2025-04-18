#ifndef P1DATA_H
#define P1DATA_H

#include <time.h>
#include <cstdint>
#include <cstdio> 

// P1Data class to store P1 meter data
class P1Data {
public:

    enum OBISUnit {
        kWH = 0, // kWh
        kVARH,
        kVAR,
        VOLT,
        AMP,
        kW,
        UNKNOWN
    };

    struct OBISValue {

        uint8_t C;
        uint8_t D;
        float value;
        OBISUnit unit;

        const char * unitToString(OBISUnit unit) const {
            const char * unitStrings[] = {"kWh","kVArh","kVAr", "V", "A", "kW", "unknown"};

            return unitStrings[unit];
        }

        int toString(char * char_buffer, size_t size) const {
            return snprintf(char_buffer, size, "1-0:%d.%d.0(%f*%s)", C, D, value, unitToString(unit));
        }
    };



    static const char BUFFER_SIZE = 32;
    OBISValue obisValues[BUFFER_SIZE]; // Array to store OBIS values
    uint8_t obisCount; // Number of OBIS values stored

    // Meter identification
    char szDeviceId[BUFFER_SIZE];             // Device ID
    char szMeterModel[BUFFER_SIZE];           // Meter model/manufacturer

    void setDeviceId(const char *szDeviceId);
    
    
    // Timestamp
    time_t timestamp;                    // Timestamp of the reading


    
    // Constructor
    P1Data() : 
        timestamp(0),
        obisCount(0) {
            szDeviceId[0] = '\0';
            szMeterModel[0] = '\0';
        }
};



#endif