#include "wifi_status_task.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "p1data.h" // Include for createP1JWT function

#if defined(USE_BLE_SETUP)
    #include "ble_handler.h"
    extern BLEHandler bleHandler;
#endif

WifiStatusTask::WifiStatusTask(uint32_t stackSize, UBaseType_t priority) 
    : taskHandle(nullptr), stackSize(stackSize), priority(priority), shouldRun(false),
      wifiManager(nullptr), ledPin(-1), lastJWTTime(0), jwtInterval(10000), 
      bleShutdownTime(0), isBleActive(false) {
}

void WifiStatusTask::begin() {
    if (taskHandle != nullptr) {
        return; // Task already running
    }
    
    shouldRun = true;
    xTaskCreatePinnedToCore(
        taskFunction,
        "WifiStatusTask",
        stackSize,
        this,
        priority,
        &taskHandle,
        0  // Run on core 0
    );
}

void WifiStatusTask::stop() {
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

void WifiStatusTask::taskFunction(void* parameter) {
    WifiStatusTask* task = static_cast<WifiStatusTask*>(parameter);
    static unsigned long lastCheck = 0;
    static bool wasConnected = false;
    
    while (task->shouldRun) {
        // Check WiFi status every 5 seconds
        if (millis() - lastCheck > 5000) {
            lastCheck = millis();
            
            if (task->wifiManager && task->wifiManager->isConnected()) {
                if (!wasConnected) {
                    Serial.println("WiFi connected");
                    Serial.print("IP address: ");
                    Serial.println(task->wifiManager->getLocalIP());
                    wasConnected = true;
                }
                
                // Send JWT if conditions are met
                #if defined(USE_BLE_SETUP)
                if (!task->isBleActive && (millis() - task->lastJWTTime >= task->jwtInterval)) {
                    task->sendJWT();
                    task->lastJWTTime = millis();
                }
                #else
                if (millis() - task->lastJWTTime >= task->jwtInterval) {
                    task->sendJWT();
                    task->lastJWTTime = millis();
                }
                #endif
                
            } else {
                if (wasConnected) {
                    Serial.println("WiFi connection lost!");
                    wasConnected = false;
                }
                #if defined(DIRECT_CONNECT)
                    // Only try to reconnect automatically in direct connect mode
                    if (task->ledPin >= 0) {
                        digitalWrite(task->ledPin, millis() % 1000 < 500); // Blink LED when disconnected
                    }
                    WiFi.begin(WIFI_SSID, WIFI_PSK);
                    Serial.println("WiFi disconnected, attempting to reconnect...");
                #endif
            }
            
            // Print some debug info
            Serial.print("Free heap: ");
            Serial.println(ESP.getFreeHeap());
            Serial.print("WiFi status: ");
            Serial.println(task->wifiManager ? task->wifiManager->getStatus() : -1);
        }
        
        // Handle BLE tasks
        #if defined(USE_BLE_SETUP)
            static unsigned long lastBLECheck = 0;
            if (millis() - lastBLECheck > 1000) {
                lastBLECheck = millis();
                bleHandler.handlePendingRequest();
            }
            if (task->bleShutdownTime > 0 && millis() >= task->bleShutdownTime) {
                Serial.println("Executing scheduled BLE shutdown");
                bleHandler.stop();
                task->isBleActive = false;
                task->bleShutdownTime = 0;  // Reset the timer
            }
        #endif
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}

void WifiStatusTask::sendJWT() {
    String deviceId = crypto_getId();
    
    // Create JWT using P1 data
    String jwt = createP1JWT(PRIVATE_KEY_HEX, deviceId);
    if (jwt.length() > 0) {
        Serial.println("P1 JWT created successfully");
        Serial.println("JWT: " + jwt);  // Add JWT logging
        
        // Create HTTP client and configure it
        HTTPClient http;
        http.setTimeout(10000);  // 10 second timeout
        
        Serial.print("Sending JWT to: ");
        Serial.println(DATA_URL);
        
        // Start the request
        if (http.begin(DATA_URL)) {
            // Add headers
            http.addHeader("Content-Type", "text/plain");
            
            // Send POST request with JWT as body
            int httpResponseCode = http.POST(jwt);
            
            if (httpResponseCode > 0) {
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                String response = http.getString();
                Serial.println("Response: " + response);
            } else {
                Serial.print("Error code: ");
                Serial.println(httpResponseCode);
            }
            
            // Clean up
            http.end();
        } else {
            Serial.println("Failed to connect to server");
        }
    } else {
        Serial.println("Failed to create P1 JWT");
    }
} 