#pragma once

#include <Arduino.h>
#include "endpoint_types.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string>
#include <ArduinoJson.h>
#include "ble_constants.h"
#include <WebServer.h>

// Include FreeRTOS queue headers
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class BLERequestCallback;
class BLEResponseCallback;

// Custom server callbacks to handle connection events
class SrcfulBLEServerCallbacks: public BLEServerCallbacks {
public:
    void onConnect(BLEServer* pServer) override {
        Serial.println("BLE client connected");
    }
    
    void onDisconnect(BLEServer* pServer) override {
        Serial.println("BLE client disconnected");
        // Restart advertising when disconnected to allow new connections
        BLEDevice::startAdvertising();
    }
};

class BLEHandler {
public:
    BLEHandler(WebServer* server);
    void init();
    void stop();
    bool sendResponse(const String& location, const String& method, const String& data, int offset = 0);
    void handleRequest(const String& request);
    void checkAdvertising();
    void handlePendingRequest();
    void enqueueRequest(const String& requestStr);

private:
    WebServer* webServer;
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pRequestChar;
    BLECharacteristic* pResponseChar;
    BLERequestCallback* pRequestCallback;
    BLEResponseCallback* pResponseCallback;
    SrcfulBLEServerCallbacks* pServerCallbacks;
    bool isAdvertising;
    QueueHandle_t _requestQueue = nullptr;
    
    String constructResponse(const String& location, const String& method, 
                           const String& data, int offset);
    bool parseRequest(const String& request, String& method, String& path, 
                     String& content, int& offset);
    void handleRequestInternal(const String& method, const String& path, 
                             const String& content, int offset);

};

class BLERequestCallback : public BLECharacteristicCallbacks {
public:
    BLERequestCallback(BLEHandler* handler) : handler(handler) {}
    void onWrite(BLECharacteristic* pCharacteristic);
private:
    BLEHandler* handler;
};

class BLEResponseCallback : public BLECharacteristicCallbacks {
public:
    BLEResponseCallback(BLEHandler* handler) : handler(handler) {}
    void onRead(BLECharacteristic* pCharacteristic);
private:
    BLEHandler* handler;
}; 