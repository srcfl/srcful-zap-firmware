#pragma once

#include <Arduino.h>

// Define HTTP methods as an enum
enum class HttpMethod {
    GET,
    POST,
    DELETE,
    UNKNOWN
};

// Define all possible endpoints as an enum
enum class Endpoint {
    WIFI_CONFIG,
    SYSTEM_INFO,
    WIFI_RESET,
    CRYPTO_INFO,
    NAME_INFO,
    WIFI_STATUS,
    WIFI_SCAN,
    BLE_STOP,
    CRYPTO_SIGN,
    OTA_UPDATE,
    UNKNOWN
}; 