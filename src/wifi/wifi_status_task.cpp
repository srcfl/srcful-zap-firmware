#include "wifi_status_task.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "main_actions.h"

#include "zap_log.h"

static const char* TAG = "wifi_status_task";

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

    // sleep for a bit to allow other tasks to initialize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    while (task->shouldRun) {
        // Check WiFi status every 5 seconds
        if (millis() - lastCheck > 5000) {
            lastCheck = millis();
            
            if (task->wifiManager && task->wifiManager->isConnected()) {
                if (!wasConnected) {
                    LOG_I(TAG, "WiFi connected");
                    LOG_D(TAG, "IP address: %s", task->wifiManager->getLocalIP().c_str());
                    wasConnected = true;
                    task->connectionAttempts = 0; // Reset connection attempts
                    MainActions::triggerAction(MainActions::Type::SEND_STATE, 500);
                }
                
            } else {
                if (wasConnected) {
                    LOG_I(TAG, "WiFi connection lost!");
                    wasConnected = false;
                }

                // Attempt to reconnect TODO: incremental backoff and max attempts
                task->connectionAttempts++;
                task->wifiManager->autoConnect();
                LOG_D(TAG, "Connection attempt: %i", task->connectionAttempts);
            }
            
            if (task->wifiManager) {
                if (task->wifiManager->getScanWiFiNetworks()) {
                    task->wifiManager->setScanWiFiNetworks(false);
                    task->wifiManager->scanWiFiNetworks();
                    MainActions::triggerAction(MainActions::Type::SEND_STATE, 500);
                }
            }

            // Print some debug info
            LOG_D(TAG, "Free heap: %i", ESP.getFreeHeap());
            LOG_D(TAG, "WiFi status: %i", task->wifiManager ? task->wifiManager->getStatus() : -1);
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}