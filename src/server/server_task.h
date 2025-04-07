#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "webserver.h"

class ServerTask {
public:
    // Constructor
    ServerTask(int port = 80, uint32_t stackSize = 8192, UBaseType_t priority = 5);
    
    // Initialize and start the task
    void begin();
    
    // Stop the task
    void stop();
    
    // Get the web server instance
    WebServerHandler& getWebServer() { return webServer; }
    
    // Task status
    bool isRunning() const { return taskHandle != nullptr; }
    
private:
    // The actual task function
    static void taskFunction(void* parameter);
    
    // Web server instance
    WebServerHandler webServer;
    
    // Task handle
    TaskHandle_t taskHandle;
    
    // Task parameters
    int port;
    uint32_t stackSize;
    UBaseType_t priority;
    
    // Flag to control task execution
    bool shouldRun;
}; 