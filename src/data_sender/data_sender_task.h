#ifndef DATA_SENDER_TASK_H
#define DATA_SENDER_TASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <HTTPClient.h>

#include "wifi/wifi_manager.h"
#include "p1data.h"

class DataSenderTask {
public:
    DataSenderTask(uint32_t stackSize = 4096*2, UBaseType_t priority = 5);
    ~DataSenderTask();
    
    void begin(WifiManager* wifiManager);
    void stop();
    
    // Set the interval for sending data (in milliseconds)
    void setInterval(uint32_t interval);
    
    // Check if BLE is active
    void setBleActive(bool active);
    bool isBleActive() const;
    
private:
    static void taskFunction(void* parameter);
    void sendJWT(HTTPClient &client);
    
    TaskHandle_t taskHandle;
    uint32_t stackSize;
    UBaseType_t priority;
    bool shouldRun;
    
    WifiManager* wifiManager;
    unsigned long lastJWTTime;
    uint32_t jwtInterval;
    bool bleActive;
};

#endif // DATA_SENDER_TASK_H 