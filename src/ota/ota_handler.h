#pragma once

#include "endpoints/endpoint_types.h"
#include "ota/ota_task.h"



class OTAHandler {
public:
    OTAHandler();
    ~OTAHandler();
    
    void begin();
    void stop();
    
    // Endpoint handlers
    EndpointResponse handleOTAUpdate(const zap::Str& contents);
    EndpointResponse handleOTAStatus(const zap::Str& contents);

    // Direct update request to the task
    bool requestOTAUpdate(const zap::Str& url, const zap::Str& version);
    
private:
    OTATask otaTask;
};

class OTAUpdateHandler : public EndpointFunction {
    public:
        explicit OTAUpdateHandler(OTAHandler& handler) : handler(handler) {}
        EndpointResponse handle(const zap::Str& contents) override {
            return handler.handleOTAUpdate(contents);
        }
    private:
        OTAHandler& handler;    // TODO: wonky that the handler has the task. Separate into manager and task as the others?
};

class OTAStatusHandler : public EndpointFunction {
    public:
        explicit OTAStatusHandler(OTAHandler& handler) : handler(handler) {}
        EndpointResponse handle(const zap::Str& contents) override {
            return handler.handleOTAStatus(contents);
        }
    private:
        OTAHandler& handler;
};