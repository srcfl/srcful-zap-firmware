#pragma once
#include "json_light/json_light.h"

class Debug {
    public:
        static void addFailedFrame();
        static void addFrame();
        static JsonBuilder& getJsonReport(JsonBuilder& jb);
    
    private:
        static int failedFrames;
        static int frames;
};