#include "debug.h"


int Debug::failedFrames = 0;
int Debug::frames = 0;
char Debug::deviceId[32] = {0};
char Debug::deviceModel[32] = {0};

void Debug::addFailedFrame() {
    failedFrames++;
}
void Debug::addFrame() {
    frames++;
}

void Debug::setDeviceId(const char *szDeviceId) {
    strncpy(deviceId, szDeviceId, sizeof(deviceId) - 1);
    deviceId[sizeof(deviceId) - 1] = '\0'; // Ensure null termination
}
void Debug::setDeviceModel(const char *szDeviceModel) {
    strncpy(deviceModel, szDeviceModel, sizeof(deviceModel) - 1);
    deviceModel[sizeof(deviceModel) - 1] = '\0'; // Ensure null termination
}

JsonBuilder& Debug::getJsonReport(JsonBuilder& jb) {
    jb.beginObject("report")
        .add("failedFrames", failedFrames)
        .add("successFrames", frames)
        .add("totalFrames", failedFrames + frames)
        .add("deviceId", deviceId)
        .add("deviceModel", deviceModel);
    jb.endObject();
    return jb;
}
