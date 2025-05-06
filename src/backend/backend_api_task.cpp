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

BackendApiTask::BackendApiTask(uint32_t stackSize, UBaseType_t priority) 
    : stackSize(stackSize), priority(priority), shouldRun(false),
      wifiManager(nullptr), lastUpdateTime(0),  
      stateUpdateInterval(DEFAULT_STATE_UPDATE_INTERVAL), 
      bleActive(false),
      requestSubscription("wss://api.srcful.dev/") {
    // Initialize with default intervals
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
    
    // Force immediate state update and config fetch by setting lastUpdateTime to a value that will
    // trigger an immediate update in the task loop
    lastUpdateTime = 0;
    
    // Set interval to 0 to trigger immediate update on connection
    stateUpdateInterval = 0;
    
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
    stateUpdateInterval = interval;
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
        

        unsigned long currentTime = millis();

        if (task->wifiManager && task->wifiManager->isConnected() && !task->bleActive) {
            static bool firstRun = true;

            task->dataSender.loop();

            task->requestSubscription.loop();

            
            // Check if it's time to send a state update
            if (firstRun || task->isTimeForStateUpdate(currentTime)) {
                task->stateUpdateInterval = DEFAULT_STATE_UPDATE_INTERVAL;                
                task->lastUpdateTime = currentTime;
                
                task->sendStateUpdate();
            }

            firstRun = false; // Reset first run flag after the first iteration
        }/* else if (task->isTimeForStateUpdate(currentTime)) {
            if (task->wifiManager && task->wifiManager->isConnected()) {
                LOG_I(TAG, "WiFi connected but BLE is active, not sending state update");
            } else {
                LOG_I(TAG, "WiFi not connected, not sending state update");
            }
        }*/
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if (task->requestSubscription.isConnected()) {
        task->requestSubscription.stop();
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}

bool BackendApiTask::isTimeForStateUpdate(unsigned long currentTime) const {
    return (currentTime - lastUpdateTime > stateUpdateInterval);
}

void BackendApiTask::sendStateUpdate() {
    LOG_I(TAG, "Preparing state update");
    
    // Use JsonBuilder to create JWT header
    JsonBuilder headerBuilder;
    headerBuilder.beginObject()
        .add("alg", "ES256")
        .add("typ", "JWT")
        .add("device", crypto_getId().c_str())
        .add("subKey", "state");
    zap::Str header = headerBuilder.end();
    
    // Get current epoch time in milliseconds
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t epochTimeMs = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
    
    // Use JsonBuilder to create JWT payload
    JsonBuilder payloadBuilder;
    payloadBuilder.beginObject()
        .beginObject("status")
            .add("uptime", millis())
            .add("version", FIRMWARE_VERSION_STRING)
        .endObject()
        .beginObject("network")
            .beginObject("wifi")
                .add("connected", wifiManager->isConnected() ? wifiManager->getConfiguredSSID().c_str() : "")
                .addArray("ssids", wifiManager->getLastScanResults())
            .endObject()
            .beginObject("address")
                .add("ip", wifiManager->isConnected() ? wifiManager->getLocalIP().c_str() : "")
                .add("port", 80)
                .add("wlan0_mac", wifiManager->getMacAddress().c_str())
                .beginObject("interfaces")
                    .add("wlan0", wifiManager->isConnected() ? wifiManager->getLocalIP().c_str() : "")
                .endObject()
            .endObject()
        .endObject()
        .add("timestamp", epochTimeMs);
    zap::Str payload = payloadBuilder.end();
    
    // External signature key (from config.h)
    extern const char* PRIVATE_KEY_HEX;
    
    // Sign and generate JWT
    zap::Str jwt = crypto_create_jwt(header.c_str(), payload.c_str(), PRIVATE_KEY_HEX);
    
    if (jwt.length() == 0) {
        LOG_E(TAG, "Failed to create JWT");
        return;
    }
    
    LOG_I(TAG, "JWT created successfully");
    
    // Send the JWT using GraphQL
    GQL::BoolResponse response = GQL::setConfiguration(jwt);
    
    // Handle the response
    if (response.isSuccess() && response.data) {
        LOG_I(TAG, "State update sent successfully");
    } else {
        // Handle different error cases
        switch (response.status) {
            case GQL::Status::NETWORK_ERROR:
                LOG_E(TAG, "Network error sending state update");
                break;
            case GQL::Status::GQL_ERROR:
                LOG_E(TAG, "GraphQL error in state update");
                break;
            case GQL::Status::OPERATION_FAILED:
                LOG_E(TAG, "Server rejected state update");
                break;
            default:
                LOG_E(TAG, "Failed to send state update: %s", response.error.c_str());
                break;
        }
        
        // Retry sooner on failure (after 1 minute instead of 5)
        lastUpdateTime = millis() - (stateUpdateInterval - 60000);
    }
}

void BackendApiTask::triggerStateUpdate() {
    // we need to set both to 0 to trigger immediate update because of unsigned variables
    lastUpdateTime = 0;
    stateUpdateInterval = 0;
    LOG_I(TAG, "Triggering immediate state update: %s", isTimeForStateUpdate(millis()) ? "true" : "false");
}

void BackendApiTask::_triggerStateUpdate() {
    // Only send state update if WiFi is connected and BLE is not active
    if (wifiManager && wifiManager->isConnected() && !bleActive) {
        LOG_I(TAG, "Triggering immediate state update...");
        sendStateUpdate();
    } else {
        LOG_W(TAG, "Cannot trigger update - WiFi not connected or BLE active");
    }
}