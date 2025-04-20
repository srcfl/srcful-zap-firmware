#include "data_reader_task.h"
#include "p1_dlms_decoder.h"
#include "p1data_funcs.h"
#include "debug.h"


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

    // Set up the frame callback before initializing
    p1Meter.setFrameCallback([this](const IFrameData& frame) {
        this->handleFrame(frame);
    });

    if (!p1Meter.begin()) {
        Serial.println("Data reader task: Failed to initialize P1 meter");
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

void DataReaderTask::enqueueData(const P1Data& p1data) {
    if (p1DataQueue != nullptr) {
        // Create a data package with char array, this is ok as the xQueueSendToBack function will copy the data
        DataPackage package;
        
        // Clear the data buffer first
        memset(package.data, 0, MAX_DATA_SIZE);
        zap::Str jwt = createP1JWT(PRIVATE_KEY_HEX, crypto_getId(), p1data);
        // Check if the JWT was created successfully
        if (jwt.length() == 0) {
            Serial.println("Data reader task: Failed to create JWT");
            return;
        }
        // Copy the JWT to the data object
        strncpy(package.data, jwt.c_str(), jwt.length());
        package.timestamp = millis();
        if (uxQueueSpacesAvailable(p1DataQueue) == 0) {
            // Queue is full, remove the oldest item first
            DataPackage oldPackage;
            xQueueReceive(p1DataQueue, &oldPackage, 0);
            Serial.println("Data reader task: Queue full, removed oldest item");
        }
        
        // Add the new package to the back (FIFO behavior)
        BaseType_t result = xQueueSendToBack(p1DataQueue, &package, pdMS_TO_TICKS(100));
        if (result == pdPASS) {
            Serial.println("Data reader task: Added data package to queue");
        } else {
            Serial.println("Data reader task: Failed to add data package to queue");
        }            
    }
}

// New method to handle complete frames received from P1Meter
void DataReaderTask::handleFrame(const IFrameData& frame) {
    
    const size_t size = frame.getFrameSize();

    // Debug output print first 15 and last 15 bytes of the frame
    Serial.print("Data reader task: Received P1 frame ("); Serial.print(size); Serial.println(" bytes)");
    for (size_t i = 0; i < min(size, size_t(15)); i++) {
        Serial.print((char)frame.getFrameByte(i));
    }
    if (size > 30) Serial.print("...");
    for (size_t i = max(size - 15, size_t(15)); i < size; i++) {
        Serial.print((char)frame.getFrameByte(i));
    }
    Serial.println();
    
    // Decode the frame
    P1DLMSDecoder decoder;
    P1Data p1data;
    
    if (decoder.decodeBuffer(frame, p1data)) {
        Debug::addFrame();
        Serial.println("P1 data decoded successfully");
        if (p1data.szDeviceId[0] != '\0') {
           Debug::setDeviceId(p1data.szDeviceId);
        }
        enqueueData(p1data);
    } else {
        Debug::addFailedFrame();
        Debug::clearFaultyFrameData();
        for (size_t i = 0; i < size; i++) {
            Debug::addFaultyFrameData(frame.getFrameByte(i));
        }
        Serial.println("Failed to decode P1 data frame");
    }
}

void DataReaderTask::taskFunction(void* parameter) {
    DataReaderTask* task = static_cast<DataReaderTask*>(parameter);
    
    while (task->shouldRun) {
        // Update P1 meter - this will read available data and call our frame callback
        // when complete frames are detected
        task->p1Meter.update();
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}