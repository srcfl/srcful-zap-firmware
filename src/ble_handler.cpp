#include "ble_handler.h"
#include "endpoints/endpoint_types.h"
#include "ble_constants.h"  // Include this to use the UUIDs defined in ble_constants.cpp
#include <Arduino.h>
#include <HTTPClient.h>
#include <esp_system.h>
#include "json_light/json_light.h"
#include "crypto.h"
#include "endpoints/endpoint_mapper.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "config.h"

// Define Queue properties
#define REQUEST_QUEUE_LENGTH 5
#define REQUEST_QUEUE_ITEM_SIZE sizeof(char*) // Size of the pointer to the data
#define REQUEST_QUEUE_RECEIVE_TIMEOUT_MS 10 // Timeout for waiting on the queue

BLEHandler::BLEHandler() {
    pServer = nullptr;
    pService = nullptr;
    pRequestChar = nullptr;
    pResponseChar = nullptr;
    pServerCallbacks = nullptr;
    isAdvertising = false;

    // Create the queue
    _requestQueue = xQueueCreate(REQUEST_QUEUE_LENGTH, REQUEST_QUEUE_ITEM_SIZE);
    if (_requestQueue == nullptr) {
        Serial.println("Error creating BLE request queue!");
        // Handle error appropriately - maybe halt or signal failure
    } else {
        Serial.println("BLE request queue created successfully.");
    }
}

void BLEHandler::init() {
    // Initialize NimBLE
    NimBLEDevice::init("Sourceful Zippy Zap");
    
    // Set the MTU size to maximum supported (517 bytes)
    NimBLEDevice::setMTU(512);
    
    // Create server
    pServer = NimBLEDevice::createServer();
    
    // Add server connection callbacks to detect disconnections
    pServerCallbacks = new SrcfulBLEServerCallbacks(this);
    pServer->setCallbacks(pServerCallbacks);
    
    // Create service with extended attribute table size for iOS
    pService = pServer->createService(SRCFUL_SERVICE_UUID);
    
    // Create characteristics with notification support
    pRequestChar = pService->createCharacteristic(
        SRCFUL_REQUEST_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE |
        NIMBLE_PROPERTY::WRITE_NR |
        NIMBLE_PROPERTY::NOTIFY
    );
    
    // For response char, ensure both NOTIFY and INDICATE are available for iOS
    pResponseChar = pService->createCharacteristic(
        SRCFUL_RESPONSE_CHAR_UUID,
        NIMBLE_PROPERTY::READ |
        NIMBLE_PROPERTY::NOTIFY |
        NIMBLE_PROPERTY::INDICATE
    );
    
    // Set callbacks
    pRequestCallback = new BLERequestCallback(this);
    pResponseCallback = new BLEResponseCallback(this);
    pRequestChar->setCallbacks(pRequestCallback);
    pResponseChar->setCallbacks(pResponseCallback);
    
    // Start service
    pService->start();

    // Improved advertising configuration for iOS compatibility
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SRCFUL_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    
    // These settings significantly improve iOS/macOS compatibility
    pAdvertising->setMinPreferred(0x06);  // Settings that help with iPhone connections
    pAdvertising->setMaxPreferred(0x12);
    
    // Set specific advertising intervals for better iOS/macOS discovery
    // Apple devices prefer faster advertising intervals
    pAdvertising->setMinInterval(0x20);   // 20ms
    pAdvertising->setMaxInterval(0x30);   // 30ms
    
    // Start advertising
    NimBLEDevice::startAdvertising();
    isAdvertising = true;
    Serial.println("NimBLE service started and advertising with iOS-optimized settings");
    stopTimer = 0;
}

void BLEHandler::hardStop() {
    if (pServer != nullptr) {
        NimBLEDevice::stopAdvertising();
        isAdvertising = false;
        NimBLEDevice::deinit(true);
        Serial.println("NimBLE stopped and resources released");
        stopTimer = 0;
    }
    digitalWrite(LED_PIN, HIGH); // Turn off
}

void BLEHandler::stop() {
    stopTimer = millis();
}

bool BLEHandler::shouldHardStop(unsigned long timeout) const {
    return stopTimer > 0 && millis() - stopTimer > timeout;
}

bool BLEHandler::isActive() {
    return isAdvertising;
}

void BLEHandler::checkAdvertising() {
    if (!isAdvertising) {
        Serial.println("NimBLE advertising stopped - restarting");
        NimBLEDevice::startAdvertising();
        isAdvertising = true;
    }
}

// Optimize string literals by making them PROGMEM
static const char RESPONSE_OK[] PROGMEM = "EGWTP/1.1 200 OK\r\n";
static const char CONTENT_TYPE[] PROGMEM = "Content-Type: text/json\r\n";
static const char CONTENT_LENGTH[] PROGMEM = "Content-Length: ";
static const char LOCATION[] PROGMEM = "Location: ";
static const char METHOD[] PROGMEM = "Method: ";
static const char OFFSET[] PROGMEM = "Offset: ";

// Error messages
static const char ERROR_INVALID_JSON[] PROGMEM = "{\"status\":\"error\",\"message\":\"Invalid JSON\"}";
static const char ERROR_MISSING_CREDS[] PROGMEM = "{\"status\":\"error\",\"message\":\"Missing credentials\"}";
static const char ERROR_INVALID_REQUEST[] PROGMEM = "{\"status\":\"error\",\"message\":\"Invalid request format\"}";
static const char ERROR_NOT_FOUND[] PROGMEM = "{\"status\":\"error\",\"message\":\"Endpoint not found\"}";
static const char SUCCESS_WIFI_UPDATE[] PROGMEM = "{\"status\":\"success\",\"message\":\"WiFi credentials updated\"}";
static const char SUCCESS_WIFI_RESET[] PROGMEM = "{\"status\":\"success\",\"message\":\"WiFi reset successful\"}";

// Modify constructResponse to use PROGMEM strings
zap::Str BLEHandler::constructResponse(const zap::Str& location, const zap::Str& method, 
                                   const zap::Str& data, int offset) {
    zap::Str header;
    
    // Add each part separately to avoid concatenation issues
    header += RESPONSE_OK;
    header += LOCATION;
    header += location;
    header += "\r\n";
    
    header += METHOD;
    header += method;
    header += "\r\n";
    
    header += CONTENT_TYPE;
    
    header += CONTENT_LENGTH;
    header += zap::Str(data.length());
    header += "\r\n";
    
    if (offset > 0) {
        header += OFFSET;
        header += zap::Str(offset);
        header += "\r\n";
    }
    
    header += "\r\n";
    zap::Str response = header + data.substring(offset);
    
    if (response.length() > MAX_BLE_PACKET_SIZE) {
        response = response.substring(0, MAX_BLE_PACKET_SIZE);
    }
    
    return response;
}

bool BLEHandler::sendResponse(const zap::Str& location, const zap::Str& method, 
                            const zap::Str& data, int offset) {
    zap::Str response = constructResponse(location, method, data, offset);
    pResponseChar->setValue((uint8_t*)response.c_str(), response.length());
    pResponseChar->notify();
    return true;
}

void BLERequestCallback::onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();    // TODO: Propbably not optimal to go via std::string
    if (value.length() > 0) {
        handler->enqueueRequest(zap::Str(value.c_str()));
    }
}

void BLEResponseCallback::onRead(NimBLECharacteristic* pCharacteristic) {
    // Handle read if needed
}

void BLEHandler::enqueueRequest(const zap::Str& requestStr) {
    if (_requestQueue == nullptr) return;
    
    // Allocate memory for the string
    char* buffer = (char*)malloc(requestStr.length() + 1);
    if (buffer == nullptr) {
        Serial.println("Failed to allocate memory for BLE request");
        return;
    }
    
    // Copy the string to the allocated memory
    strcpy(buffer, requestStr.c_str());
    
    // Send the pointer to the queue
    if (xQueueSend(_requestQueue, &buffer, pdMS_TO_TICKS(100)) != pdPASS) {
        Serial.println("Failed to send request to queue");
        free(buffer);
    }
}

void BLEHandler::handlePendingRequest() {
    if (_requestQueue == nullptr) return;

    // Receive the pointer to the data
    char* buffer = nullptr;
    if (xQueueReceive(_requestQueue, &buffer, pdMS_TO_TICKS(REQUEST_QUEUE_RECEIVE_TIMEOUT_MS)) == pdPASS) {
        if (buffer != nullptr) {
            Serial.printf("Dequeued request (%d bytes)\n", strlen(buffer));

            // Create a String object from the buffer
            zap::Str receivedRequest(buffer);

            // Process the request
            handleRequest(receivedRequest);

            // Free the buffer that was allocated in enqueueRequest
            free(buffer);
        }
    }
}

void BLEHandler::handleRequest(const zap::Str& request) {
    zap::Str method, path, content;
    int offset = 0;
    
    if (!parseRequest(request, method, path, content, offset)) {
        Serial.println("Failed to parse request");
        return;
    }
    
    handleRequestInternal(method, path, content, offset);
}

bool BLEHandler::parseRequest(const zap::Str& request, zap::Str& method, zap::Str& path, 
                                zap::Str& content, int& offset) {
    int headerEnd = request.indexOf("\r\n\r\n");
    if (headerEnd == -1) return false;
    
    zap::Str header = request.substring(0, headerEnd);
    content = request.substring(headerEnd + 4);
    
    // Parse first line
    int firstLineEnd = header.indexOf("\r\n");
    zap::Str firstLine = header.substring(0, firstLineEnd);
    
    if (!firstLine.endsWith(" EGWTTP/1.1")) return false;
    
    int methodEnd = firstLine.indexOf(' ');
    method = firstLine.substring(0, methodEnd);
    
    // Get path and trim whitespace
    path = firstLine.substring(methodEnd + 1, firstLine.length() - 10);
    path.trim();  // Trim as a separate operation
    
    // Parse headers
    offset = 0;
    if (header.indexOf("Offset: ") != -1) {
        int offsetStart = header.indexOf("Offset: ") + 8;
        int offsetEnd = header.indexOf("\r\n", offsetStart);
        offset = header.substring(offsetStart, offsetEnd).toInt();
    }
    
    return true;
}

void BLEHandler::handleRequestInternal(const zap::Str& method, const zap::Str& path, 
                                     const zap::Str& content, int offset) {
    EndpointRequest request(EndpointMapper::toEndpoint(path, method));
    request.content = content;
    request.offset = offset;
    
    EndpointResponse response = EndpointMapper::route(request);

    Serial.println((method + " " + path + " " + " Response: " + response.data).c_str());
    
    if (response.statusCode == 200) {
        sendResponse(path, method, response.data, offset);
    } else {
        sendResponse(path, method, response.data, 0);
    }
}

void SrcfulBLEServerCallbacks::onConnect(NimBLEServer* pServer) {
    Serial.println("BLE client connected");
    // Log the actual MTU size being used
    Serial.printf("Current MTU size: %d\n", NimBLEDevice::getMTU());
}

void SrcfulBLEServerCallbacks::onDisconnect(NimBLEServer* pServer) {
    Serial.println("BLE client disconnected");
    // Restart advertising when disconnected to allow new connections
    NimBLEDevice::startAdvertising();
}
