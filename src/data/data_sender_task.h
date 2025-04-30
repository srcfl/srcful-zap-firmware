#ifndef DATA_SENDER_TASK_H
#define DATA_SENDER_TASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <HTTPClient.h>
#include "../zap_str.h"

#include "wifi/wifi_manager.h"
#include "data_package.h"  // Local include for data package header

class DataSenderTask {
public:
    DataSenderTask(uint32_t stackSize = 4096*2, UBaseType_t priority = 5);
    ~DataSenderTask();
    
    void begin(WifiManager* wifiManager);
    void stop();
    
    // Set the interval for checking queue (in milliseconds)
    void setInterval(uint32_t interval);
    
    // Check if BLE is active
    void setBleActive(bool active);
    bool isBleActive() const;
    
    // Get the queue handle to be used by the DataReaderTask
    QueueHandle_t getQueueHandle();
    
private:
    static void taskFunction(void* parameter);
    void sendJWT(const zap::Str& payload);
    
    TaskHandle_t taskHandle;
    uint32_t stackSize;
    UBaseType_t priority;
    bool shouldRun;
    
    WifiManager* wifiManager;
    unsigned long lastCheckTime;
    uint32_t checkInterval;
    bool bleActive;
    
    HTTPClient http;  // Reuse HTTPClient instance
    QueueHandle_t p1DataQueue;  // Queue for P1 data packages
};

#endif // DATA_SENDER_TASK_H