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
      wifiManager(nullptr), ledPin(-1) {
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
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task cleanup
    vTaskDelete(NULL);
} 