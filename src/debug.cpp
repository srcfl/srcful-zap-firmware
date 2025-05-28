#include "debug.h"
#include <Arduino.h>
#include <esp_heap_caps.h> // Include for heap functions
#include <esp_system.h> // Ensure it's included here too


int Debug::failedFrames = 0;
int Debug::frames = 0;
int Debug::p1MeterConfigIndex = -1; // Initialize static member
char Debug::deviceId[32] = {0};
char Debug::deviceModel[32] = {0};
uint8_t Debug::faultyFrameData[1024] = {0};
size_t Debug::faultyFrameDataSize = 0;
CircularBuffer *Debug::pMeterDatabuffer = nullptr;
esp_reset_reason_t Debug::lastResetReason = ESP_RST_UNKNOWN; // Initialize static member


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
    }
}

void Debug::setResetReason(esp_reset_reason_t reason) {
    lastResetReason = reason;
}

esp_reset_reason_t Debug::getResetReason() {
    return lastResetReason;
}

zap::Str toHexString(const CircularBuffer *pBuffer) {
    zap::Str hexString;
    hexString.reserve(pBuffer->available() * 2); // Reserve space for hex string
    char temp[3]; // 2 hex digits + null terminator
    temp[2] = '\0'; // Null terminator for sprintf
    for (size_t i = 0; i < pBuffer->available(); i++) {
        uint8_t byte = pBuffer->getByte(i);
        sprintf(temp, "%02x", byte);
        hexString += temp;
    }

    return hexString;
}

JsonBuilder& Debug::getJsonReport(JsonBuilder& jb) {
    
    jb.beginObject("report")
        .add("uptime_sek", millis() / 1000)
        .add("p1CfgIx", p1MeterConfigIndex)
        .add("failedFrames", failedFrames)
        .add("successFrames", frames)
        .add("totalFrames", failedFrames + frames)
        .add("deviceId", deviceId)
        .add("deviceModel", deviceModel)
        // Add heap information
        .add("freeHeap", esp_get_free_heap_size())
        .add("minFreeHeap", esp_get_minimum_free_heap_size())
        .add("resetReason", (int)lastResetReason); // Add reset reason as integer

    if (faultyFrameDataSize > 0) {
        jb.add("faultyFrameData", faultyFrameData, faultyFrameDataSize);
    }

    if (pMeterDatabuffer) {
        zap::Str hexString = toHexString(pMeterDatabuffer);
        jb.add("meterDataBuffer", hexString);
    }

    jb.endObject();
    return jb;
}
