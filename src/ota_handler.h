#pragma once

#include <Arduino.h>
#include "endpoints/endpoint_types.h"
#include "ota/ota_task.h"

class OTAHandler {
public:
    OTAHandler();
    ~OTAHandler();
    
    void begin();
    void stop();
    
    EndpointResponse handleOTAUpdate(const EndpointRequest& request);
    EndpointResponse handleOTAStatus(const EndpointRequest& request);
    
private:
    OTATask otaTask;
};