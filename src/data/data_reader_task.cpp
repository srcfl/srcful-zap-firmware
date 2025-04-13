#include "data_reader_task.h"
#include "p1_dlms_decoder.h"

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
        String jwt = createP1JWT(PRIVATE_KEY_HEX, crypto_getId(), p1data);
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


void DataReaderTask::taskFunction(void* parameter) {
    DataReaderTask* task = static_cast<DataReaderTask*>(parameter);
    
    while (task->shouldRun) {


        task->p1Meter.update();
        // Serial.println("Data reader task: P1 meter updated");
        // Serial.println("Data reader task: Available bytes: " + String(task->p1Meter.getBufferIndex()));

        // Check if we have new data
        if (task->p1Meter.getBufferIndex() > 0) {
            // Process the data
            const String buffer(task->p1Meter.getBuffer(), task->p1Meter.getBufferIndex());
            Serial.println("Data reader task: Buffer data: " + buffer);
            
            P1DLMSDecoder decoder;
            P1Data p1data;
            if (decoder.decodeBuffer(task->p1Meter.getBuffer(), task->p1Meter.getBufferIndex(), p1data)) {
                //Serial.println("P1 data decoded successfully");
                Serial.println(String(p1data.currentL1));
                Serial.println(String(p1data.voltageL1));

                // Serial.println(p1data.currentL1);
                // Serial.println(p1data.currentL2);
                // Serial.println(p1data.currentL3);

                task->enqueueData(p1data);
                Serial.println("Data reader task: P1 data enqueued");
                
            } else {
                Serial.println("Failed to decode P1 data");
            }


            task->p1Meter.clearBuffer();
            Serial.println("Data reader task: Buffer cleared");
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}