#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "endpoints/endpoint_types.h"
#include "ble_constants.h"  // Include this to use the UUIDs defined in ble_constants.cpp

// Forward declaration
class BLEHandler;

// Custom server callbacks to detect disconnections
class SrcfulBLEServerCallbacks: public NimBLEServerCallbacks {
    private:
        BLEHandler* handler;
    public:
        SrcfulBLEServerCallbacks(BLEHandler* h) : handler(h) {}
        void onConnect(NimBLEServer* pServer);
        void onDisconnect(NimBLEServer* pServer);
};

// Custom characteristic callbacks for request handling
class BLERequestCallback: public NimBLECharacteristicCallbacks {
    private:
        BLEHandler* handler;
    public:
        BLERequestCallback(BLEHandler* h) : handler(h) {}
        void onWrite(NimBLECharacteristic* pCharacteristic);
};

// Custom characteristic callbacks for response handling
class BLEResponseCallback: public NimBLECharacteristicCallbacks {
    private:
        BLEHandler* handler;
    public:
        BLEResponseCallback(BLEHandler* h) : handler(h) {}
        void onRead(NimBLECharacteristic* pCharacteristic);
};

// Main BLE handler class
class BLEHandler {
    private:
        NimBLEServer* pServer;
        NimBLEService* pService;
        NimBLECharacteristic* pRequestChar;
        NimBLECharacteristic* pResponseChar;
        SrcfulBLEServerCallbacks* pServerCallbacks;
        BLERequestCallback* pRequestCallback;
        BLEResponseCallback* pResponseCallback;
        bool isAdvertising;
        QueueHandle_t _requestQueue;
        
        // Private helper methods
        bool parseRequest(const String& request, String& method, String& path, String& content, int& offset);
        void handleRequestInternal(const String& method, const String& path, const String& content, int offset);
        String constructResponse(const String& location, const String& method, const String& data, int offset);
        
    public:
        BLEHandler();
        void init();
        void stop();
        void checkAdvertising();
        bool sendResponse(const String& location, const String& method, const String& data, int offset);
        void handleRequest(const String& request);
        void handlePendingRequest();
        void enqueueRequest(const String& requestStr);
        void processRequest(const String& request);
        
        // Friend classes to allow callbacks to access private methods
        friend class SrcfulBLEServerCallbacks;
        friend class BLERequestCallback;
        friend class BLEResponseCallback;
}; 