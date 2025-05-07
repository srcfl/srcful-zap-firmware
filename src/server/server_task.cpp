#include "server_task.h"
#include "endpoints/endpoint_mapper.h"
#include "zap_log.h" // Added for logging

// Define TAG for logging
static const char* TAG = "server_task";

// Constructor
ServerTask::ServerTask(int port, uint32_t stackSize, UBaseType_t priority)
    : webServer(port), taskHandle(nullptr), port(port), stackSize(stackSize), 
      priority(priority), shouldRun(false) {
}

// Initialize and start the task
void ServerTask::begin() {
    if (taskHandle != nullptr) {
        // Task already running
        return;
    }
    
    // Set up the web server
    webServer.setupEndpoints();
    
    // Set the flag to run the task
    shouldRun = true;
    
    // Create the task
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction,           // Task function
        "ServerTask",          // Task name
        stackSize,             // Stack size
        this,                  // Task parameters
        priority,              // Task priority
        &taskHandle,           // Task handle
        1                      // Run on core 1 (to avoid conflicts with BLE on core 0)
    );
    
    if (result != pdPASS) {
        LOG_E(TAG, "Failed to create server task!");
        shouldRun = false;
    } else {
        LOG_I(TAG, "Server task created successfully");
    }
}

// Stop the task
void ServerTask::stop() {
    if (taskHandle == nullptr) {
        // Task not running
        return;
    }
    
    // Set the flag to stop the task
    shouldRun = false;
    
    // Wait for the task to finish
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
    
    LOG_I(TAG, "Server task stopped");
}

// The actual task function
void ServerTask::taskFunction(void* parameter) {
    // Get the ServerTask instance
    ServerTask* serverTask = static_cast<ServerTask*>(parameter);
    
    // Start the web server
    serverTask->webServer.begin();
    
    LOG_I(TAG, "Server task started");
    
    // Main task loop
    while (serverTask->shouldRun) {
        // Handle client requests
        serverTask->webServer.handleClient();
        
        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // Task cleanup
    LOG_I(TAG, "Server task ending");
    
    // Delete the task
    vTaskDelete(nullptr);
}