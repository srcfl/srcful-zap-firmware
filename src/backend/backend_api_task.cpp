#include "backend_api_task.h"
#include <WiFiClientSecure.h>
#include "../crypto.h"
#include "../config.h"
#include "graphql.h"  // Include local graphql.h

BackendApiTask::BackendApiTask(uint32_t stackSize, UBaseType_t priority) 
    : taskHandle(nullptr), stackSize(stackSize), priority(priority), shouldRun(false),
      wifiManager(nullptr), lastUpdateTime(0), updateInterval(300000), bleActive(false) {
    // Initialize with 5-minute interval (300,000 ms)
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
    updateInterval = interval;
}

void BackendApiTask::setBleActive(bool active) {
    bleActive = active;
}

bool BackendApiTask::isBleActive() const {
    return bleActive;
}

void BackendApiTask::taskFunction(void* parameter) {
    BackendApiTask* task = static_cast<BackendApiTask*>(parameter);
    
    while (task->shouldRun) {
        // Check if it's time to send a state update
        if (millis() - task->lastUpdateTime > task->updateInterval) {
            task->lastUpdateTime = millis();
            
            // Only send state update if WiFi is connected and BLE is not active
            if (task->wifiManager && task->wifiManager->isConnected() && !task->bleActive) {
                Serial.println("Backend API task: Sending state update...");
                task->sendStateUpdate();
            } else {
                if (task->wifiManager && task->wifiManager->isConnected()) {
                    Serial.println("Backend API task: WiFi connected but BLE is active, not sending state update");
                } else {
                    Serial.println("Backend API task: WiFi not connected, not sending state update");
                }
            }
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}

void BackendApiTask::sendStateUpdate() {
    Serial.println("Backend API task: Preparing state update");
    
    // TODO: Collect state information and prepare JSON payload
    // For now, just send a simple state message
    
    // Create a simple JSON state object
    String stateJson = "{\"device_status\":\"online\",\"firmware_version\":\"";
    
    // Check if we have firmware version information
    #ifdef FIRMWARE_VERSION
    stateJson += FIRMWARE_VERSION;
    #else
    stateJson += "unknown";
    #endif
    
    stateJson += "\",\"timestamp\":";
    stateJson += millis();
    stateJson += "}";
    
    Serial.print("Backend API task: State data: ");
    Serial.println(stateJson);
    
    // Close any previous connections and start a new one
    http.end();
    
    // Create the backend API endpoint URL
    String apiUrl = String(BACKEND_API_URL) + "/state";
    
    Serial.print("Backend API task: Sending state to: ");
    Serial.println(apiUrl);
    
    // Start the request
    if (http.begin(apiUrl)) {
        // Add headers
        http.addHeader("Content-Type", "application/json");
        
        // Send POST request with state data as body
        int httpResponseCode = http.POST(stateJson);
        
        if (httpResponseCode > 0) {
            Serial.print("Backend API task: HTTP Response code: ");
            Serial.println(httpResponseCode);
            String response = http.getString();
            Serial.print("Backend API task: Response: ");
            Serial.println(response);
        } else {
            Serial.print("Backend API task: Error code: ");
            Serial.println(httpResponseCode);
        }
        
        // Note: Don't call http.end() here to reuse the connection
        // The connection will be closed when needed or in the destructor
    } else {
        Serial.println("Backend API task: Failed to connect to server");
    }
}