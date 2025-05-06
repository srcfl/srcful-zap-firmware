#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <HTTPClient.h>
#include "../zap_str.h"

#include "wifi/wifi_manager.h"

class DataSenderTask {
public:
    DataSenderTask();
    ~DataSenderTask();
        
    
    // Get the queue handle to be used by the DataReaderTask
    QueueHandle_t getQueueHandle();

    void loop();

    
private:
    void sendJWT(const zap::Str& payload);


    bool bleActive;
    
    HTTPClient http;  // Reuse HTTPClient instance
    QueueHandle_t p1DataQueue;  // Queue for P1 data packages
};
