#pragma once
#include "json_light/json_light.h"

class Debug {
    public:
        static void addFailedFrame();
        static void addFrame();
        static JsonBuilder& getJsonReport(JsonBuilder& jb);
        static void setDeviceId(const char *szDeviceId);
        static void setDeviceModel(const char *szDeviceModel);

        static void setFaultyFrameData(const uid_t* data, size_t size);
        static void addFaultyFrameData(const uid_t byte);
        static void clearFaultyFrameData() {
            faultyFrameDataSize = 0;
        }
    
    private:
        static int failedFrames;
        static int frames;
        static char deviceId[32];
        static char deviceModel[32];

        static uint8_t faultyFrameData[1024];
        static size_t faultyFrameDataSize;
};