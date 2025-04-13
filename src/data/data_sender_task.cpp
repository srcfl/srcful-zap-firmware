// filepath: /home/h0bb3/projects/github/srcful-zap-firmware/src/data/data_sender_task.cpp
#include "data_sender_task.h"
#include <WiFiClientSecure.h>
#include "p1data.h"
#include "../crypto.h"
#include "../config.h"

DataSenderTask::DataSenderTask(uint32_t stackSize, UBaseType_t priority) 
    : taskHandle(nullptr), stackSize(stackSize), priority(priority), shouldRun(false),
      wifiManager(nullptr), lastCheckTime(0), checkInterval(5000), bleActive(false) {
    
    // Create the queue for data packages (store up to 3 packages)
    p1DataQueue = xQueueCreate(3, sizeof(DataPackage));
    if (p1DataQueue == nullptr) {
        Serial.println("Data sender task: Failed to create queue");
    }
}

DataSenderTask::~DataSenderTask() {
    stop();
    http.end(); // Clean up the HTTP client connection
    
    // Delete queue if it exists
    if (p1DataQueue != nullptr) {
        vQueueDelete(p1DataQueue);
        p1DataQueue = nullptr;
    }
}

void DataSenderTask::begin(WifiManager* wifiManager) {
    if (taskHandle != nullptr) {
        return; // Task already running
    }
    
    this->wifiManager = wifiManager;
    shouldRun = true;
    
    // Configure HTTP client once
    http.setTimeout(10000);  // 10 second timeout
    
    xTaskCreatePinnedToCore(
        taskFunction,
        "DataSenderTask",
        stackSize,
        this,
        priority,
        &taskHandle,
        0  // Run on core 0
    );
}

void DataSenderTask::stop() {
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

void DataSenderTask::setInterval(uint32_t interval) {
    checkInterval = interval;
}

void DataSenderTask::setBleActive(bool active) {
    bleActive = active;
}

bool DataSenderTask::isBleActive() const {
    return bleActive;
}

QueueHandle_t DataSenderTask::getQueueHandle() {
    return p1DataQueue;
}

void DataSenderTask::taskFunction(void* parameter) {
    DataSenderTask* task = static_cast<DataSenderTask*>(parameter);
    
    while (task->shouldRun) {
        // Check WiFi status periodically
        if (millis() - task->lastCheckTime > task->checkInterval) {
            task->lastCheckTime = millis();
            
            if (task->wifiManager && task->wifiManager->isConnected() && !task->bleActive) {
                // Check if there are packages in the queue
                if (uxQueueMessagesWaiting(task->p1DataQueue) > 0) {
                    Serial.println("Data sender task: Found data in queue to send");
                    
                    // Get the oldest package from the queue (FIFO behavior)
                    DataPackage package;
                    if (xQueueReceive(task->p1DataQueue, &package, 0) == pdTRUE) {
                        Serial.println("Data sender task: Retrieved package from queue");
                        
                        // Convert char array back to String for sending
                        String dataStr(package.data);
                        
                        // Send the data from the package
                        task->sendJWT(dataStr);
                    }
                } else {
                    Serial.println("Data sender task: No data in queue to send");
                }
            } else {
                if (task->wifiManager && task->wifiManager->isConnected()) {
                    Serial.println("Data sender task: WiFi connected but BLE is active, not sending data");
                } else {
                    Serial.println("Data sender task: WiFi not connected, not sending data");
                }
            }
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}

void DataSenderTask::sendJWT(const String& jwt) {
    if (jwt.length() == 0) {
        Serial.println("Data sender task: Empty JWT, not sending");
        return;
    }
    
    Serial.println("Data sender task: Sending JWT...");
    Serial.print("Data sender task jwt:");
    Serial.println(jwt);
    
    Serial.print("Data sender task: Sending JWT to: ");
    Serial.println(DATA_URL);
    
    // Close any previous connections and start a new one
    http.end();
    
    // Start the request
    if (http.begin(DATA_URL)) {
        // Add headers
        http.addHeader("Content-Type", "text/plain");
        
        // Send POST request with JWT as body
        int httpResponseCode = http.POST(jwt);
        
        if (httpResponseCode > 0) {
            Serial.print("Data sender task: HTTP Response code: ");
            Serial.println(httpResponseCode);
            String response = http.getString();
            Serial.print("Data sender task: Response: ");
            Serial.println(response);
        } else {
            Serial.print("Data sender task: Error code: ");
            Serial.println(httpResponseCode);
        }
        
        // Note: Don't call http.end() here to reuse the connection
        // The connection will be closed when needed or in the destructor
    } else {
        Serial.println("Data sender task: Failed to connect to server");
    }
}