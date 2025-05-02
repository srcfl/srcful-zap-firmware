#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <HTTPClient.h>

#include "../wifi/wifi_manager.h"
#include "json_light/json_light.h"

#include "config_subscription.h"

// Default state update interval (5 minutes = 300,000 ms)
const uint32_t DEFAULT_STATE_UPDATE_INTERVAL = 300000;

class DataSenderTask;  // Forward declaration

class BackendApiTask {
public:
    BackendApiTask(DataSenderTask& dataSender, uint32_t stackSize = 4096*2, UBaseType_t priority = 5);
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
    
private:
    bool isTimeForStateUpdate(unsigned long currentTime) const;
    static void taskFunction(void* parameter);
    void sendStateUpdate();
    
    DataSenderTask& dataSender;
    
    TaskHandle_t taskHandle;
    uint32_t stackSize;
    UBaseType_t priority;
    bool shouldRun;
    
    WifiManager* wifiManager;
    unsigned long lastUpdateTime;
    uint32_t stateUpdateInterval;
    bool bleActive;
    
    HTTPClient http;  // Reuse HTTPClient instance

    GraphQLSubscriptionClient requestSubscription;
};
