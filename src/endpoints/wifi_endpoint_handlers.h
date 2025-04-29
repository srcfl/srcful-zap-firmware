#pragma once
#include "endpoint_types.h"
#include "wifi/wifi_manager.h"
#include "zap_str.h"


class WifiHandler {
    protected:
        WifiManager& wifiManager;
    public:
        WifiHandler(WifiManager& wifiManager) : wifiManager(wifiManager) {}
};


// WiFi Config Handler
class WiFiConfigHandler : public EndpointFunction, protected WifiHandler {
    public:
        WiFiConfigHandler(WifiManager& wifiManager) : WifiHandler(wifiManager) {}
        EndpointResponse handle(const zap::Str& contents) override;
};

// WiFi Reset Handler
class WiFiResetHandler : public EndpointFunction, protected WifiHandler {
    public:
        WiFiResetHandler(WifiManager& wifiManager) : WifiHandler(wifiManager) {}
        EndpointResponse handle(const zap::Str& contents) override;
};


// WiFi Status Handler
class WiFiStatusHandler : public EndpointFunction, protected WifiHandler {
    public:
        WiFiStatusHandler(WifiManager& wifiManager) : WifiHandler(wifiManager) {}
        EndpointResponse handle(const zap::Str& contents) override;

};


class WiFiScanHandler : public EndpointFunction, protected WifiHandler {
    public:
        WiFiScanHandler(WifiManager& wifiManager) : WifiHandler(wifiManager) {}
        EndpointResponse handle(const zap::Str& contents) override;

};

