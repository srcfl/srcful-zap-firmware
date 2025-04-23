#ifndef BACKEND_API_TASK_H
#define BACKEND_API_TASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <HTTPClient.h>

#include "../wifi/wifi_manager.h"

// Default state update interval (5 minutes = 300,000 ms)
const uint32_t DEFAULT_STATE_UPDATE_INTERVAL = 300000;

const uint32_t DEFAULT_CONFIG_FETCH_INTERVAL = 30 * 1000; // 30 seconds

class BackendApiTask {
public:
    BackendApiTask(uint32_t stackSize = 4096*2, UBaseType_t priority = 5);
    ~BackendApiTask();
    
    void begin(WifiManager* wifiManager);
    void stop();
    
    // Set the interval for sending state updates (in milliseconds)
    void setInterval(uint32_t interval);
    
    // Set the interval for fetching configuration (in milliseconds)
    void setConfigFetchInterval(uint32_t interval);
    
    // Check if BLE is active
    void setBleActive(bool active);
    bool isBleActive() const;
    
    // Trigger an immediate state update (if conditions allow)
    void triggerStateUpdate();
    
    // Trigger an immediate configuration fetch (if conditions allow)
    void triggerConfigFetch();
    
private:
    static void taskFunction(void* parameter);
    void sendStateUpdate();
    void fetchConfiguration();
    void processConfiguration(const char* configData);
    
    TaskHandle_t taskHandle;
    uint32_t stackSize;
    UBaseType_t priority;
    bool shouldRun;
    
    WifiManager* wifiManager;
    unsigned long lastUpdateTime;
    unsigned long lastConfigFetchTime;
    uint32_t stateUpdateInterval;
    uint32_t configFetchInterval;
    bool bleActive;
    
    HTTPClient http;  // Reuse HTTPClient instance
};

#endif // BACKEND_API_TASK_H