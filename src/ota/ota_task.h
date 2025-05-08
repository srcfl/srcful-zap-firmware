#ifndef OTA_TASK_H
#define OTA_TASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>

#include "zap_str.h"

// these structs are used to send data to the task via the queue this seems like a bit of overkill...
// Define a struct for the OTA update request
typedef struct {
    char url[256];
    char version[64];
    bool hasVersion;
} OTAUpdateRequest;

// Define a struct for the OTA update result
typedef struct {
    bool success;
    char message[128];
    int statusCode;
} OTAUpdateResult;

class OTATask {
public:
    OTATask(uint32_t stackSize = 1024 * 8, UBaseType_t priority = 3);
    ~OTATask();
    
    // Start the OTA task
    void begin();
    
    // Stop the task
    void stop();
    
    // Queue an update request
    bool requestUpdate(const zap::Str& url, const zap::Str& version);
    
    // Check if an update is in progress
    bool isUpdateInProgress() const;
    
    // Check if an update has completed and get the result
    bool getUpdateResult(OTAUpdateResult* result);
    
    // Get update progress in percent (0-100)
    int getUpdateProgress() const;
    
private:
    static void taskFunction(void* parameter);
    bool performUpdate(const OTAUpdateRequest& request, OTAUpdateResult& result);
    void updateProgress(size_t current, size_t total);
    bool verifyFirmware(const uint8_t* data, size_t len);
    
    TaskHandle_t taskHandle;
    uint32_t stackSize;
    UBaseType_t priority;
    bool shouldRun;
    
    // Queue for update requests
    QueueHandle_t updateQueue;
    
    // Status variables protected by mutex
    SemaphoreHandle_t statusMutex;
    volatile bool updateInProgress;
    volatile int progressPercent;
    OTAUpdateResult lastResult;
    bool resultAvailable;
};

#endif // OTA_TASK_H