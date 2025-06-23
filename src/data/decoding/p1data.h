#pragma once

#include <time.h>
#include <cstdint>
#include <cstdio>
#include <cstring> // Added for strncpy

// P1Data class to store P1 meter data
class P1Data {
public:
    static const uint8_t MAX_OBIS_STRINGS = 36;
    static const uint8_t MAX_OBIS_STRING_LEN = 36;
    char obisStrings[MAX_OBIS_STRINGS][MAX_OBIS_STRING_LEN]; // Array to store OBIS strings TODO: We could pack the strings into a single buffer instead
    uint8_t obisStringCount; // Number of OBIS strings stored

    // Meter identification
    // TODO: Are these really needed?
    static const uint8_t DEVICE_ID_LEN = 32;
    static const uint8_t METER_MODEL_LEN = 32;
    char szDeviceId[DEVICE_ID_LEN];             // Device ID
    char szMeterModel[METER_MODEL_LEN];           // Meter model/manufacturer

    void setDeviceId(const char *szDeviceId);

    // Constructor
    P1Data() :
        obisStringCount(0) {
            szDeviceId[0] = '\0';
            szMeterModel[0] = '\0';
            for (int i = 0; i < MAX_OBIS_STRINGS; ++i) {
                obisStrings[i][0] = '\0';
            }
        }

    // Method to add OBIS string based on components
    bool addObisString(uint8_t obis_c, uint8_t obis_d, float value, const char* unit) {
        if (obisStringCount >= MAX_OBIS_STRINGS) {
            return false; // Buffer full
        }
        int written = snprintf(obisStrings[obisStringCount], MAX_OBIS_STRING_LEN, "1-0:%d.%d.0(%f*%s)", obis_c, obis_d, value, unit);
        if (written > 0 && written < MAX_OBIS_STRING_LEN) {
            obisStringCount++;
            return true;
        }
        return false; // Error during formatting or buffer too small
    }

    // Method to add a raw OBIS string
    bool addObisString(const char* szObisString) {
        if (obisStringCount >= MAX_OBIS_STRINGS) {
            return false; // Buffer full
        }
        strncpy(obisStrings[obisStringCount], szObisString, MAX_OBIS_STRING_LEN - 1);
        obisStrings[obisStringCount][MAX_OBIS_STRING_LEN - 1] = '\0'; // Ensure null termination
        obisStringCount++;
        return true;
    }
};
