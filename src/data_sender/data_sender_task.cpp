#include "data_sender_task.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "p1data.h"
#include "crypto.h"
#include "config.h"

DataSenderTask::DataSenderTask(uint32_t stackSize, UBaseType_t priority) 
    : taskHandle(nullptr), stackSize(stackSize), priority(priority), shouldRun(false),
      wifiManager(nullptr), lastJWTTime(0), jwtInterval(10000), bleActive(false) {
}

DataSenderTask::~DataSenderTask() {
    stop();
}

void DataSenderTask::begin(WifiManager* wifiManager) {
    if (taskHandle != nullptr) {
        return; // Task already running
    }
    
    this->wifiManager = wifiManager;
    shouldRun = true;
    xTaskCreatePinnedToCore(
        taskFunction,
        "DataSenderTask",
        stackSize,
        this,
        priority,
        &taskHandle,
        0  // Run on core 0
    );
}

void DataSenderTask::stop() {
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

void DataSenderTask::setInterval(uint32_t interval) {
    jwtInterval = interval;
}

void DataSenderTask::setBleActive(bool active) {
    bleActive = active;
}

bool DataSenderTask::isBleActive() const {
    return bleActive;
}

void DataSenderTask::taskFunction(void* parameter) {
    DataSenderTask* task = static_cast<DataSenderTask*>(parameter);
    static unsigned long lastCheck = 0;
    
    while (task->shouldRun) {
        // Check WiFi status every 5 seconds
        if (millis() - lastCheck > 5000) {
            lastCheck = millis();
            
            if (task->wifiManager && task->wifiManager->isConnected() && !task->bleActive) {
                // Send JWT if conditions are met
                if (millis() - task->lastJWTTime >= task->jwtInterval) {
                    Serial.println("Data sender task: Conditions met for sending data");
                    Serial.print("WiFi connected: ");
                    Serial.println(task->wifiManager->isConnected());
                    Serial.print("BLE active: ");
                    Serial.println(task->bleActive);
                    Serial.print("Time since last send: ");
                    Serial.print(millis() - task->lastJWTTime);
                    Serial.print(" ms (interval: ");
                    Serial.print(task->jwtInterval);
                    Serial.println(" ms)");
                    
                    task->sendJWT();
                    task->lastJWTTime = millis();
                }
            } else {
                if (task->wifiManager && task->wifiManager->isConnected()) {
                    Serial.println("Data sender task: WiFi connected but BLE is active, not sending data");
                } else {
                    Serial.println("Data sender task: WiFi not connected, not sending data");
                }
            }
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}

void DataSenderTask::sendJWT() {
    String deviceId = crypto_getId();
    
    Serial.println("Data sender task: Creating JWT...");
    
    // Create JWT using P1 data
    String jwt = createP1JWT(PRIVATE_KEY_HEX, deviceId);
    if (jwt.length() > 0) {
        Serial.println("Data sender task: P1 JWT created successfully");
        Serial.print("Data sender task: JWT length: ");
        Serial.println(jwt.length());
        Serial.println(jwt);
        
        // Create HTTP client and configure it
        HTTPClient http;
        http.setTimeout(10000);  // 10 second timeout
        
        Serial.print("Data sender task: Sending JWT to: ");
        Serial.println(DATA_URL);
        
        // Start the request
        if (http.begin(DATA_URL)) {
            // Add headers
            http.addHeader("Content-Type", "text/plain");
            
            // Send POST request with JWT as body
            int httpResponseCode = http.POST(jwt);
            
            if (httpResponseCode > 0) {
                Serial.print("Data sender task: HTTP Response code: ");
                Serial.println(httpResponseCode);
                String response = http.getString();
                Serial.print("Data sender task: Response: ");
                Serial.println(response);
            } else {
                Serial.print("Data sender task: Error code: ");
                Serial.println(httpResponseCode);
            }
            
            // Clean up
            http.end();
        } else {
            Serial.println("Data sender task: Failed to connect to server");
        }
    } else {
        Serial.println("Data sender task: Failed to create P1 JWT");
    }
} 