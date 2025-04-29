#pragma once

#include "endpoint_types.h"

class WifiManager; // Forward declaration

class SystemInfoHandler : public EndpointFunction {
    public:
    SystemInfoHandler(const WifiManager& wifiManager);
        EndpointResponse handle(const zap::Str& contents) override;
        
    private:
        const WifiManager& wifiManager;
};

class SystemRebootHandler : public EndpointFunction {
    public:
        EndpointResponse handle(const zap::Str& contents) override;
};