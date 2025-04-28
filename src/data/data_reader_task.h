// filepath: /home/h0bb3/projects/github/srcful-zap-firmware/src/data/data_reader_task.h
#ifndef DATA_READER_TASK_H
#define DATA_READER_TASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "../zap_str.h"

#include "decoding/p1data.h" // Local include since p1data is now in the data directory
#include "../crypto.h"
#include "../config.h"
#include "data_package.h"  // Include the new data package header

#include "p1_meter.h"  // Include P1Meter class for reading data
#include "decoding/IFrameData.h"  // Include IFrameData interface for frame data handling

class DataReaderTask {
public:
    DataReaderTask(uint32_t stackSize = 1024 * 10, UBaseType_t priority = 4);
    ~DataReaderTask();
    
    void begin(QueueHandle_t dataQueue);
    void stop();
    
    // Set the interval for reading data (in milliseconds)
    void setInterval(uint32_t interval);
    
private:
    static void taskFunction(void* parameter);
    zap::Str generateP1JWT();
    void enqueueData(const P1Data& p1data);
    
    // Handle a complete frame from P1 meter
    void handleFrame(const IFrameData& frame);

    TaskHandle_t taskHandle;
    uint32_t stackSize;
    UBaseType_t priority;
    bool shouldRun;
    
    QueueHandle_t p1DataQueue;
    uint32_t readInterval;
    unsigned long lastReadTime;

    P1Meter p1Meter;  // Pointer to the P1Meter instance for reading data
};

#endif // DATA_READER_TASK_H