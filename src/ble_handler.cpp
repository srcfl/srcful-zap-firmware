#include "endpoint_types.h"
#include "ble_handler.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <esp_system.h>
#include <esp_bt.h>              // For esp_bt_controller functions
#include <esp_bt_main.h>         // For esp_bluedroid functions
#include <esp_gap_ble_api.h>     // For BLE GAP functionality
#include <WebServer.h>
#include <ArduinoJson.h>
#include "crypto.h"
#include "endpoint_mapper.h"


// Define Queue properties
#define REQUEST_QUEUE_LENGTH 5
#define REQUEST_QUEUE_ITEM_SIZE sizeof(char*) // Size of the pointer to the data
#define REQUEST_QUEUE_RECEIVE_TIMEOUT_MS 10 // Timeout for waiting on the queue

BLEHandler::BLEHandler(WebServer* server) : webServer(server) {
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
    // Initialize BLE with reduced features and increase stack size for better handling
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    bt_cfg.controller_task_stack_size = 4096;
    
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    esp_bluedroid_init();
    esp_bluedroid_enable();
    
    BLEDevice::init("Sourceful Zippy Zap");
    btStart();
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);  // Release classic BT memory
    
    // Create server
    pServer = BLEDevice::createServer();
    
    // Add server connection callbacks to detect disconnections
    pServerCallbacks = new SrcfulBLEServerCallbacks();
    pServer->setCallbacks(pServerCallbacks);
    
    // Create service with extended attribute table size for iOS
    pService = pServer->createService(BLEUUID(SRCFUL_SERVICE_UUID), 40);
    
    // Create characteristics with notification support
    // Use WRITE_NR (no response) for request characteristic as iOS prefers this
    pRequestChar = pService->createCharacteristic(
        SRCFUL_REQUEST_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE | 
        BLECharacteristic::PROPERTY_WRITE_NR |  // Add no-response write for iOS
        BLECharacteristic::PROPERTY_NOTIFY
    );
    
    // For response char, ensure both NOTIFY and INDICATE are available for iOS
    pResponseChar = pService->createCharacteristic(
        SRCFUL_RESPONSE_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | 
        BLECharacteristic::PROPERTY_NOTIFY | 
        BLECharacteristic::PROPERTY_INDICATE
    );
    
    // Create BLE Descriptors for notifications - crucial for iOS
    BLE2902* pRequestDescriptor = new BLE2902();
    pRequestDescriptor->setNotifications(true);
    pRequestChar->addDescriptor(pRequestDescriptor);
    
    BLE2902* pResponseDescriptor = new BLE2902();
    pResponseDescriptor->setNotifications(true);
    pResponseDescriptor->setIndications(true);  // Add indication support
    pResponseChar->addDescriptor(pResponseDescriptor);
    
    // Set callbacks
    pRequestCallback = new BLERequestCallback(this);
    pResponseCallback = new BLEResponseCallback(this);
    pRequestChar->setCallbacks(pRequestCallback);
    pResponseChar->setCallbacks(pResponseCallback);
    
    // Start service
    pService->start();

    // Improved advertising configuration for iOS compatibility
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SRCFUL_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    
    // These settings significantly improve iOS/macOS compatibility
    pAdvertising->setMinPreferred(0x06);  // Settings that help with iPhone connections
    pAdvertising->setMaxPreferred(0x12);
    
    // Set specific advertising intervals for better iOS/macOS discovery
    // Apple devices prefer faster advertising intervals
    pAdvertising->setMinInterval(0x20);   // 20ms
    pAdvertising->setMaxInterval(0x30);   // 30ms
    
    // Set appearance and device name - helps with iOS discovery
    esp_ble_gap_set_device_name("Sourceful Zippy Zap");
    
    // Start advertising
    BLEDevice::startAdvertising();
    isAdvertising = true;
    Serial.println("BLE service started and advertising with iOS-optimized settings");
}

void BLEHandler::stop() {
    if (pServer != nullptr) {
        pServer->getAdvertising()->stop();
        isAdvertising = false;
        BLEDevice::deinit(true);  // true = release memory
        Serial.println("BLE stopped and resources released");
    }
    // Optional: Delete queue if BLEHandler instance is being destroyed or permanently stopped
    // if (_requestQueue != nullptr) {
    //     vQueueDelete(_requestQueue);
    //     _requestQueue = nullptr;
    //     Serial.println("BLE request queue deleted.");
    // }
}

void BLEHandler::checkAdvertising() {
    if (!isAdvertising) {
        Serial.println("BLE advertising stopped - restarting");
        BLEDevice::startAdvertising();
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
String BLEHandler::constructResponse(const String& location, const String& method, 
                                   const String& data, int offset) {
    String header;
    
    // Add each part separately to avoid concatenation issues
    header += FPSTR(RESPONSE_OK);
    header += FPSTR(LOCATION);
    header += location;
    header += "\r\n";
    
    header += FPSTR(METHOD);
    header += method;
    header += "\r\n";
    
    header += FPSTR(CONTENT_TYPE);
    
    header += FPSTR(CONTENT_LENGTH);
    header += String(data.length());
    header += "\r\n";
    
    if (offset > 0) {
        header += FPSTR(OFFSET);
        header += String(offset);
        header += "\r\n";
    }
    
    header += "\r\n";
    String response = header + data.substring(offset);
    
    if (response.length() > MAX_BLE_PACKET_SIZE) {
        response = response.substring(0, MAX_BLE_PACKET_SIZE);
    }
    
    return response;
}

bool BLEHandler::sendResponse(const String& location, const String& method, 
                            const String& data, int offset) {
    String response = constructResponse(location, method, data, offset);
    pResponseChar->setValue((uint8_t*)response.c_str(), response.length());
    pResponseChar->notify();  // Add notification
    return true;
}

// handleRequest processes a single request string
void BLEHandler::handleRequest(const String& request) {
    String method, path, content;
    int offset = 0;
    // Reserve some space to potentially reduce reallocations during parsing
    method.reserve(10);
    path.reserve(64);
    // Content reservation depends heavily on expected payload size

    if (!parseRequest(request, method, path, content, offset)) {
        Serial.println("Failed to parse request.");
        // Send error response - use FPSTR to avoid String allocation for the error message itself
        sendResponse(path, method, FPSTR(ERROR_INVALID_REQUEST), 0);
        return;
    }

    Serial.printf("Parsed Request: Method='%s', Path='%s', Offset=%d, Content Length=%d\n",
                  method.c_str(), path.c_str(), offset, content.length());
    // Serial.println("Content: " + content); // Optional: Print content

    handleRequestInternal(method, path, content, offset);
}

void BLEHandler::handleRequestInternal(const String& method, const String& path, 
                                     const String& content, int offset) {
    // Create endpoint request
    EndpointRequest request;
    request.method = EndpointMapper::stringToMethod(method);
    request.endpoint = EndpointMapper::pathToEndpoint(path);
    request.content = content;
    request.offset = offset;

    // Route request through endpoint mapper
    EndpointResponse response = EndpointMapper::route(request);

    // Send response using BLE protocol format
    sendResponse(path, method, response.data, offset);
}

bool BLEHandler::parseRequest(const String& request, String& method, String& path, 
                            String& content, int& offset) {
    int headerEnd = request.indexOf("\r\n\r\n");
    if (headerEnd == -1) return false;
    
    String header = request.substring(0, headerEnd);
    content = request.substring(headerEnd + 4);
    
    // Parse first line
    int firstLineEnd = header.indexOf("\r\n");
    String firstLine = header.substring(0, firstLineEnd);
    
    if (!firstLine.endsWith(" EGWTTP/1.1")) return false;
    
    int methodEnd = firstLine.indexOf(" ");
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

void BLEHandler::handlePendingRequest() {
    if (_requestQueue == nullptr) return;

    // Receive the pointer to the data
    char* buffer = nullptr;
    if (xQueueReceive(_requestQueue, &buffer, pdMS_TO_TICKS(REQUEST_QUEUE_RECEIVE_TIMEOUT_MS)) == pdPASS) {
        if (buffer != nullptr) {
            Serial.printf("Dequeued request (%d bytes)\n", strlen(buffer));

            // Create a String object from the buffer
            String receivedRequest(buffer);

            // Process the request
            handleRequest(receivedRequest);

            // Free the buffer that was allocated in enqueueRequest
            free(buffer);
        }
    }
}

void BLEHandler::enqueueRequest(const String& requestStr) {
    if (_requestQueue == nullptr) {
        Serial.println("Error: Request queue is null in enqueueRequest.");
        return;
    }

    // Create a buffer to store the raw data
    char* buffer = (char*)malloc(requestStr.length() + 1);
    if (buffer == nullptr) {
        Serial.println("Error: Failed to allocate buffer for BLE request");
        return;
    }

    // Copy the string data to the buffer
    strcpy(buffer, requestStr.c_str());

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult = xQueueSendFromISR(_requestQueue, &buffer, &xHigherPriorityTaskWoken);

    if (xResult != pdPASS) {
        Serial.println("Error: Failed to enqueue BLE request (Queue full?). Request lost.");
        free(buffer);  // Free the buffer if enqueue failed
    }
    // Note: We don't free the buffer here if enqueue succeeded because the receiving task will free it
    
    // Yield if necessary
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void BLERequestCallback::onWrite(BLECharacteristic* pCharacteristic) {
    // Ensure handler is valid
    if (handler == nullptr) {
         Serial.println("Error: BLEHandler is null in onWrite callback!");
         return;
     }

    std::string value = pCharacteristic->getValue();
    if (!value.empty()) {
        // Limit logged output size if necessary
        Serial.printf("Received BLE write request (%d bytes)\n", value.length());

        // Create an Arduino String object from the received data.
        String requestStr = String(value.c_str());

        // Use the public handler method to enqueue the request
        handler->enqueueRequest(requestStr);

         // IMPORTANT: 'requestStr' goes out of scope here. The handler's 
         // enqueueRequest method handles the copy needed for the queue.
    }
}

void BLEResponseCallback::onRead(BLECharacteristic* pCharacteristic) {
    Serial.println("BLE read request received");
    // Typically, reads fetch the current value set by setValue,
    // which should contain the last response sent.
    // No specific action usually needed here unless implementing custom read logic.
    // String currentValue = pCharacteristic->getValue().c_str();
    // Serial.println("Current Response Char Value: " + currentValue);
} 