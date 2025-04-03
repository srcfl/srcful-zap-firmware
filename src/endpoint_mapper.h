#pragma once

#include <Arduino.h>
#include "endpoint_types.h"
#include "endpoints.h"

class EndpointMapper {
public:
    // Define path constants
    static const char* WIFI_CONFIG_PATH;
    static const char* SYSTEM_INFO_PATH;
    static const char* WIFI_RESET_PATH;
    static const char* CRYPTO_INFO_PATH;
    static const char* NAME_INFO_PATH;
    static const char* WIFI_STATUS_PATH;
    static const char* WIFI_SCAN_PATH;
    static const char* BLE_STOP_PATH;
    static const char* CRYPTO_SIGN_PATH;

    // Mapping functions
    static Endpoint pathToEndpoint(const String& path);
    static String endpointToPath(Endpoint endpoint);
    static HttpMethod stringToMethod(const String& method);
    static String methodToString(HttpMethod method);
    static EndpointResponse route(const EndpointRequest& request);
    static void printPaths();
}; 