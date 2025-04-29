#ifndef ENDPOINT_HANDLERS_H
#define ENDPOINT_HANDLERS_H

#include "endpoint_types.h"
#include "wifi/wifi_manager.h"
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "../backend/graphql.h"
#include "../ble_handler.h"
#include "ota_handler.h"
#include "zap_str.h"

// Crypto Info Handler
class CryptoInfoHandler : public EndpointFunction {
public:
    EndpointResponse handle(const zap::Str& contents) override;
};

// Name Info Handler
class NameInfoHandler : public EndpointFunction {
public:
    EndpointResponse handle(const zap::Str& contents) override;
};

// Initialize Handler
class InitializeHandler : public EndpointFunction {
public:
    EndpointResponse handle(const zap::Str& contents) override;
};

// Crypto Sign Handler
class CryptoSignHandler : public EndpointFunction {
public:
    EndpointResponse handle(const zap::Str& contents) override;
};

// OTA Update Handler
class OTAUpdateHandler : public EndpointFunction {
public:
    EndpointResponse handle(const zap::Str& contents) override;
};

class DebugHandler : public EndpointFunction {
public:
    EndpointResponse handle(const zap::Str& contents) override;
};

// Echo Handler - returns the data it received
class EchoHandler : public EndpointFunction {
public:
    EndpointResponse handle(const zap::Str& contents) override;
};

// BLE Stop Handler
class BLEStopHandler : public EndpointFunction {
    private:
        BLEHandler & bleHandler;
    public:
    BLEStopHandler(BLEHandler & bleHandler) : bleHandler(bleHandler) {}
    EndpointResponse handle(const zap::Str& contents) override;
};

#endif // ENDPOINT_HANDLERS_H