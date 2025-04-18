#pragma once
#include "json_light/json_light.h"

class Debug {
    public:
        static void addFailedFrame();
        static void addFrame();
        static JsonBuilder& getJsonReport(JsonBuilder& jb);
        static void setDeviceId(const char *szDeviceId);
        static void setDeviceModel(const char *szDeviceModel);
    
    private:
        static int failedFrames;
        static int frames;
        static char deviceId[32];
        static char deviceModel[32];
};