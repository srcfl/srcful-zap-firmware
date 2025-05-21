#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <HTTPClient.h>

#include "../wifi/wifi_manager.h"
#include "json_light/json_light.h"

#include "config_subscription.h"
#include "data_sender.h"
#include "state_handler.h" // Include the new StateHandler header
#include "ota_checker.h"   // Include the OtaChecker header

class BackendApiTask {
public:
    explicit BackendApiTask(uint32_t stackSize = 1024*8, UBaseType_t priority = 5);
    ~BackendApiTask();
    
    void begin(WifiManager* wifiManager);
    void stop();
    
    // Set the interval for sending state updates (in milliseconds)
    void setInterval(uint32_t interval); // Will now delegate to StateHandler
    
    // Check if BLE is active
    void setBleActive(bool active);
    bool isBleActive() const;
    
    // Trigger an immediate state update
    void triggerStateUpdate(); // Will now delegate to StateHandler

    QueueHandle_t getQueueHandle() {
        return dataSender.getQueueHandle();
    }
    
private:
    static void taskFunction(void* parameter);
    
    DataSenderTask dataSender;
    
    TaskHandle_t taskHandle;
    uint32_t stackSize;
    UBaseType_t priority;
    bool shouldRun;
    
    WifiManager* wifiManager; // Kept for general WiFi status checks
    
    bool bleActive;


    HTTPClient http;  // Reuse HTTPClient instance

    GraphQLSubscriptionClient requestSubscription;

    StateHandler stateHandler; // Instance of StateHandler for managing state updates
    OtaChecker otaChecker;     // Instance of OtaChecker for OTA updates
};
