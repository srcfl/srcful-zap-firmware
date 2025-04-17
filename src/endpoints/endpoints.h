#pragma once

#include "endpoint_types.h"

// EndpointFunction strategy implementations

class WiFiConfigEndpoint : public EndpointFunction {
    EndpointResponse handle(const String& contents) override;
};

class SystemInfoEndpoint : public EndpointFunction {
    EndpointResponse handle(const String& contents) override;
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
EndpointResponse handleInitialize(const EndpointRequest& request);
EndpointResponse handleDebug(const EndpointRequest& request);

#include "endpoint_handlers.h"
#include "wifi_endpoint_handlers.h"
// Create handler instances
extern WiFiConfigHandler g_wifiConfigHandler;
extern SystemInfoHandler g_systemInfoHandler;
extern WiFiResetHandler g_wifiResetHandler;
extern CryptoInfoHandler g_cryptoInfoHandler;
extern NameInfoHandler g_nameInfoHandler;
extern WiFiStatusHandler g_wifiStatusHandler;
extern WiFiScanHandler g_wifiScanHandler;
extern InitializeHandler g_initializeHandler;
extern CryptoSignHandler g_cryptoSignHandler;
extern OTAUpdateHandler g_otaUpdateHandler;
extern BLEStopHandler g_bleStopHandler;
extern DebugHandler g_debugHandler;