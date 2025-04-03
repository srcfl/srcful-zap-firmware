#pragma once

#include <Arduino.h>
#include "endpoint_types.h"
#include <ArduinoJson.h>

// Response structure that can be used by both BLE and HTTP handlers
struct EndpointResponse {
    int statusCode;      // HTTP-style status code (200, 400, etc.)
    String contentType;  // Content type of the response
    String data;        // Response data
};

// Request structure that normalizes input from both BLE and HTTP
struct EndpointRequest {
    HttpMethod method;
    Endpoint endpoint;
    String content;
    int offset;
};

// Endpoint handler functions
EndpointResponse handleWiFiConfig(const EndpointRequest& request);
EndpointResponse handleSystemInfo(const EndpointRequest& request);
EndpointResponse handleWiFiReset(const EndpointRequest& request);
EndpointResponse handleCryptoInfo(const EndpointRequest& request);
EndpointResponse handleNameInfo(const EndpointRequest& request);
EndpointResponse handleWiFiStatus(const EndpointRequest& request);
EndpointResponse handleWiFiScan(const EndpointRequest& request);
EndpointResponse handleCryptoSign(const EndpointRequest& request); 