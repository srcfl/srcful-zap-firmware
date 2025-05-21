#include "backend_api_task.h"
#include "crypto.h"
#include "config.h"
#include "./graphql.h"
#include "json_light/json_light.h"
#include "firmware_version.h"
#include <time.h>
#include "../zap_log.h" // Added for logging

// Define TAG for logging
static const char* TAG = "backend_api_task";

// DEFAULT_STATE_UPDATE_INTERVAL is now in state_handler.cpp

BackendApiTask::BackendApiTask(uint32_t stackSize, UBaseType_t priority) 
    : stackSize(stackSize), priority(priority), shouldRun(false),
      wifiManager(nullptr), // Removed lastUpdateTime
      // Removed stateUpdateInterval
      bleActive(false),
      requestSubscription("wss://api.srcful.dev/") {
    // stateHandler is default constructed
}

BackendApiTask::~BackendApiTask() {
    stop();
    http.end(); // Clean up the HTTP client connection
}

void BackendApiTask::begin(WifiManager* wifiManager) {
    if (taskHandle != nullptr) {
        return; // Task already running
    }
    
    this->wifiManager = wifiManager;
    shouldRun = true;
    
    // Configure HTTP client once
    http.setTimeout(10000);  // 10 second timeout
    
    // Initialize StateHandler
    stateHandler.begin(wifiManager);
    // stateHandler.setInterval(0); // Ensure immediate update, handled by stateHandler.begin()
    
    // Initialize OtaChecker
    otaChecker.begin();
        
    xTaskCreatePinnedToCore(
        taskFunction,
        "BackendApiTask",
        stackSize,
        this,
        priority,
        &taskHandle,
        0  // Run on core 0
    );
}

void BackendApiTask::stop() {
    if (taskHandle == nullptr) {
        return; // Task not running
    }
    
    shouldRun = false;
    vTaskDelay(pdMS_TO_TICKS(100)); // Give the task time to exit

    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
}

void BackendApiTask::setInterval(uint32_t interval) {
    stateHandler.setInterval(interval);
}

void BackendApiTask::setBleActive(bool active) {
    bleActive = active;
}

bool BackendApiTask::isBleActive() const {
    return bleActive;
}

void BackendApiTask::taskFunction(void* parameter) {
    BackendApiTask* task = static_cast<BackendApiTask*>(parameter);

    // sleep for a bit to allow other tasks to initialize
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    while (task->shouldRun) {
        const unsigned long currentTime = millis(); // Keep for other potential uses or remove if only for state update timing

        if (task->wifiManager && task->wifiManager->isConnected() && !task->bleActive) {
            task->dataSender.loop();
            task->requestSubscription.loop(currentTime);
            task->stateHandler.loop(currentTime); // Call StateHandler's loop
            task->otaChecker.loop(currentTime);   // Call OtaChecker's loop
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if (task->requestSubscription.isConnected()) {
        task->requestSubscription.stop();
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}

void BackendApiTask::triggerStateUpdate() {
    stateHandler.triggerStateUpdate(); // Delegate to StateHandler
    LOG_I(TAG, "Triggering immediate state update via BackendApiTask -> StateHandler");
}

// void BackendApiTask::_triggerStateUpdate() {
//     // Only send state update if WiFi is connected and BLE is not active
//     if (wifiManager && wifiManager->isConnected() && !bleActive) {
//         LOG_I(TAG, "Triggering immediate state update (internal)...");
//         stateHandler.sendStateUpdate(); // Directly call sendStateUpdate on handler, or use triggerStateUpdate
//     } else {
//         LOG_W(TAG, "Cannot trigger update - WiFi not connected or BLE active");
//     }
// }