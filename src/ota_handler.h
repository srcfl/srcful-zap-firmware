#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "endpoint_types.h"
#include "endpoints.h"

class OTAHandler {
public:
    static EndpointResponse handleOTAUpdate(const EndpointRequest& request);
    
private:
    static bool verifyFirmware(const uint8_t* data, size_t len);
    static void updateProgress(size_t current, size_t total);
}; 