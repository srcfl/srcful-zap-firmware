#ifndef ENDPOINT_HANDLERS_H
#define ENDPOINT_HANDLERS_H

#include "endpoint_types.h"
#include "wifi/wifi_manager.h"
#include "crypto.h"
#include "graphql.h"
#include "firmware_version.h"
#include <WiFi.h>
#include <esp_system.h>
#include "json_light/json_light.h"

#include "../ble_handler.h"

// Forward declarations
extern const char* PRIVATE_KEY_HEX;
extern const char* AP_SSID;
extern const char* AP_PASSWORD;



// System Info Handler
class SystemInfoHandler : public EndpointFunction {
public:
    EndpointResponse handle(const String& contents) override;
};

// Crypto Info Handler
class CryptoInfoHandler : public EndpointFunction {
public:
    EndpointResponse handle(const String& contents) override;
};

// Name Info Handler
class NameInfoHandler : public EndpointFunction {
public:
    EndpointResponse handle(const String& contents) override;
};

// Initialize Handler
class InitializeHandler : public EndpointFunction {
public:
    EndpointResponse handle(const String& contents) override;
};

// Crypto Sign Handler
class CryptoSignHandler : public EndpointFunction {
public:
    EndpointResponse handle(const String& contents) override;
};

// OTA Update Handler
class OTAUpdateHandler : public EndpointFunction {
public:
    EndpointResponse handle(const String& contents) override;
};

// BLE Stop Handler
class BLEStopHandler : public EndpointFunction {
    private:
        BLEHandler & bleHandler;
    public:
    BLEStopHandler(BLEHandler & bleHandler) : bleHandler(bleHandler) {}
    EndpointResponse handle(const String& contents) override;
};

#endif // ENDPOINT_HANDLERS_H 