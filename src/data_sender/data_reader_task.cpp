#include "data_reader_task.h"

DataReaderTask::DataReaderTask(uint32_t stackSize, UBaseType_t priority) 
    : taskHandle(nullptr), stackSize(stackSize), priority(priority), shouldRun(false),
      p1DataQueue(nullptr), readInterval(10000), lastReadTime(0) {
}

DataReaderTask::~DataReaderTask() {
    stop();
}

void DataReaderTask::begin(QueueHandle_t dataQueue) {
    if (taskHandle != nullptr) {
        return; // Task already running
    }
    
    this->p1DataQueue = dataQueue;
    shouldRun = true;
    xTaskCreatePinnedToCore(
        taskFunction,
        "DataReaderTask",
        stackSize,
        this,
        priority,
        &taskHandle,
        0  // Run on core 0
    );
}

void DataReaderTask::stop() {
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

void DataReaderTask::setInterval(uint32_t interval) {
    readInterval = interval;
}

String DataReaderTask::generateP1JWT() {
    String deviceId = crypto_getId();
    
    Serial.println("Data reader task: Creating JWT...");
    
    // Create JWT using P1 data
    String jwt = createP1JWT(PRIVATE_KEY_HEX, deviceId);
    if (jwt.length() > 0) {
        Serial.println("Data reader task: P1 JWT created successfully");
        Serial.print("Data reader task: JWT length: ");
        Serial.println(jwt.length());
    } else {
        Serial.println("Data reader task: Failed to create P1 JWT");
    }
    
    return jwt;
}

void DataReaderTask::taskFunction(void* parameter) {
    DataReaderTask* task = static_cast<DataReaderTask*>(parameter);
    
    while (task->shouldRun) {
        // Check if it's time to generate a new P1 data package
        if (millis() - task->lastReadTime >= task->readInterval) {
            task->lastReadTime = millis();
            
            // Generate new P1 JWT
            String jwt = task->generateP1JWT();
            
            if (jwt.length() > 0 && task->p1DataQueue != nullptr) {
                // Create a P1 data package with char array
                P1DataPackage package;
                
                // Clear the jwt buffer first
                memset(package.jwt, 0, MAX_JWT_SIZE);
                
                // Check if the JWT fits in our buffer
                if (jwt.length() < MAX_JWT_SIZE - 1) {
                    // Copy the JWT string to the char array
                    strncpy(package.jwt, jwt.c_str(), jwt.length());
                    package.timestamp = millis();
                    
                    // Send to queue (FIFO mode)
                    // If queue is full, overwrite the oldest item
                    if (uxQueueSpacesAvailable(task->p1DataQueue) == 0) {
                        // Queue is full, remove the oldest item first
                        P1DataPackage oldPackage;
                        xQueueReceive(task->p1DataQueue, &oldPackage, 0);
                        Serial.println("Data reader task: Queue full, removed oldest item");
                    }
                    
                    // Add the new package to the back (FIFO behavior)
                    BaseType_t result = xQueueSendToBack(task->p1DataQueue, &package, pdMS_TO_TICKS(100));
                    if (result == pdPASS) {
                        Serial.println("Data reader task: Added P1 data package to queue");
                    } else {
                        Serial.println("Data reader task: Failed to add P1 data package to queue");
                    }
                } else {
                    Serial.println("Data reader task: JWT too large for buffer");
                }
            }
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}