#pragma once

// Firmware version information
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0    // 0.1.1 mbus frame detection. 0.1.2 dlms parsing bugfix 0.1.3 removed some debug logs, 0.1.4 added obis data api endpoint

// Helper macro to create version string
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define FIRMWARE_VERSION_STRING TOSTRING(FIRMWARE_VERSION_MAJOR) "." \
                               TOSTRING(FIRMWARE_VERSION_MINOR) "." \
                               TOSTRING(FIRMWARE_VERSION_PATCH)

// Function to get version as string
inline const char* getFirmwareVersion() {
    return FIRMWARE_VERSION_STRING;
}