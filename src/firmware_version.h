#pragma once

// Firmware version information
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 3

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

// Function to get version as integer (for easy comparison)
inline uint32_t getFirmwareVersionInt() {
    return (FIRMWARE_VERSION_MAJOR << 16) | 
           (FIRMWARE_VERSION_MINOR << 8) | 
           FIRMWARE_VERSION_PATCH;
} 