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
    WifiStatusTask(uint32_t stackSize = 8192, UBaseType_t priority = 5);
    
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
    
    // Set the JWT interval
    void setJwtInterval(unsigned long interval) { jwtInterval = interval; }
    
    // Set the BLE shutdown time
    void setBleShutdownTime(unsigned long time) { bleShutdownTime = time; }
    
    // Get the BLE shutdown time
    unsigned long getBleShutdownTime() const { return bleShutdownTime; }
    
    // Set the BLE active flag
    void setBleActive(bool active) { isBleActive = active; }
    
    // Get the BLE active flag
    bool getBleActive() const { return isBleActive; }
    
    // Set the last JWT time
    void setLastJWTTime(unsigned long time) { lastJWTTime = time; }
    
    // Get the last JWT time
    unsigned long getLastJWTTime() const { return lastJWTTime; }
    
private:
    // The actual task function
    static void taskFunction(void* parameter);
    
    // Send JWT function
    void sendJWT();
    
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
    
    // JWT related variables
    unsigned long lastJWTTime;
    unsigned long jwtInterval;
    
    // BLE related variables
    unsigned long bleShutdownTime;
    bool isBleActive;
}; 