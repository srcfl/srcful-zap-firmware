#include "debug.h"


int Debug::failedFrames = 0;
int Debug::frames = 0;
char Debug::deviceId[32] = {0};
char Debug::deviceModel[32] = {0};
uint8_t Debug::faultyFrameData[1024] = {0};
size_t Debug::faultyFrameDataSize = 0;

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

void Debug::setFaultyFrameData(const uid_t* data, size_t size) {
    memcpy(faultyFrameData, data, size);
    faultyFrameDataSize = size;
}
void Debug::addFaultyFrameData(const uid_t byte) {
    if (faultyFrameDataSize < sizeof(faultyFrameData)) {
        faultyFrameData[faultyFrameDataSize++] = byte;
        faultyFrameDataSize++;
    }
}

JsonBuilder& Debug::getJsonReport(JsonBuilder& jb) {
    
    jb.beginObject("report")
        .add("uptime_sek", millis() / 1000)
        .add("failedFrames", failedFrames)
        .add("successFrames", frames)
        .add("totalFrames", failedFrames + frames)
        .add("deviceId", deviceId)
        .add("deviceModel", deviceModel);

    if (faultyFrameDataSize > 0) {
        jb.add("faultyFrameData", faultyFrameData, faultyFrameDataSize);
    }

    jb.endObject();
    return jb;
}
