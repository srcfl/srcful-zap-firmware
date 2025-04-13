#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "wifi_manager.h"
#include "crypto.h"
#include "graphql.h"
#include "config.h"

class WifiStatusTask {
public:
    // Constructor
    WifiStatusTask(uint32_t stackSize = 1024 * 2, UBaseType_t priority = 5);
    
    // Initialize and start the task
    void begin();
    
    // Stop the task
    void stop();
    
    // Task status
    bool isRunning() const { return taskHandle != nullptr; }
    
    // Set the WiFi manager instance
    void setWifiManager(WifiManager* manager) { wifiManager = manager; }
    
    // Set the LED pin
    void setLedPin(int pin) { ledPin = pin; }
    
private:
    // The actual task function
    static void taskFunction(void* parameter);
    
    // Task handle
    TaskHandle_t taskHandle;
    
    // Task parameters
    uint32_t stackSize;
    UBaseType_t priority;
    
    // Flag to control task execution
    bool shouldRun;
    
    // WiFi manager instance
    WifiManager* wifiManager;
    
    // LED pin
    int ledPin;
    
}; 