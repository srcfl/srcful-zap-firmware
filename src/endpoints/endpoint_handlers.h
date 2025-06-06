#ifndef ENDPOINT_HANDLERS_H
#define ENDPOINT_HANDLERS_H

#include "endpoint_types.h"
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
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
    public:
        BLEStopHandler() {}
        EndpointResponse handle(const zap::Str& contents) override;
};

#endif // ENDPOINT_HANDLERS_H