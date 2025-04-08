#pragma once
#include "endpoint_types.h"
#include "wifi/wifi_manager.h"


extern WifiManager wifiManager;


// WiFi Config Handler
class WiFiConfigHandler : public EndpointFunction {
    public:
        EndpointResponse handle(const String& contents) override;
    };


    // WiFi Reset Handler
class WiFiResetHandler : public EndpointFunction {
    public:
        EndpointResponse handle(const String& contents) override;
    };

// WiFi Status Handler
class WiFiStatusHandler : public EndpointFunction {
    public:
        EndpointResponse handle(const String& contents) override;
    };