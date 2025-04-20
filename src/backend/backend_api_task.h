#ifndef BACKEND_API_TASK_H
#define BACKEND_API_TASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <HTTPClient.h>

#include "../wifi/wifi_manager.h"

class BackendApiTask {
public:
    BackendApiTask(uint32_t stackSize = 4096*2, UBaseType_t priority = 5);
    ~BackendApiTask();
    
    void begin(WifiManager* wifiManager);
    void stop();
    
    // Set the interval for sending state updates (in milliseconds)
    void setInterval(uint32_t interval);
    
    // Check if BLE is active
    void setBleActive(bool active);
    bool isBleActive() const;
    
private:
    static void taskFunction(void* parameter);
    void sendStateUpdate();
    
    TaskHandle_t taskHandle;
    uint32_t stackSize;
    UBaseType_t priority;
    bool shouldRun;
    
    WifiManager* wifiManager;
    unsigned long lastUpdateTime;
    uint32_t updateInterval;
    bool bleActive;
    
    HTTPClient http;  // Reuse HTTPClient instance
};

#endif // BACKEND_API_TASK_H