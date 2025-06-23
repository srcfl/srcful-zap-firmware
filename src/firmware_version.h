#pragma once

// Firmware version information
#define FIRMWARE_VERSION_MAJOR 0
#define FIRMWARE_VERSION_MINOR 1
#define FIRMWARE_VERSION_PATCH 1    // 0.1.1 mbus frame detection.

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