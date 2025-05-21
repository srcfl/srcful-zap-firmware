#include "data_sender.h"
#include <WiFiClientSecure.h>
#include "../crypto.h"
#include "../config.h"
#include "../data/p1data_funcs.h"
#include "../data/data_package.h"
#include "../json_light/json_light.h"
#include <esp_log.h>

#include "zap_log.h"

static const char* TAG = "data_sender";

zap::Str createP1JWT(const char* privateKey, const zap::Str& deviceId, const char* szPayload) {
    // Create the header
   
    JsonBuilder header;
    header.beginObject()
        .add("alg", "ES256")
        .add("typ", "JWT")
        .add("device", deviceId.c_str())
        .add("opr", "production")
        .add("model", "p1zap")
        .add("dtype", "p1_telnet_json")
        .add("sn", METER_SN);

    zap::Str headerStr = header.end();
 
    
    
    // Use the crypto_create_jwt function from the crypto module
    zap::Str jwt = crypto_create_jwt(headerStr.c_str(), szPayload, privateKey);
    
    return jwt;
}

DataSenderTask::DataSenderTask() : bleActive(true) {    // ble will need to be actively disabled for the sending to start
    
    // Create the queue for data packages (store up to 3 packages)
    p1DataQueue = xQueueCreate(3, sizeof(DataPackage));
    if (p1DataQueue == nullptr) {
        LOG_E(TAG, "Data sender task: Failed to create queue");
    }
}

DataSenderTask::~DataSenderTask() {
}


QueueHandle_t DataSenderTask::getQueueHandle() {
    return p1DataQueue;
}

void DataSenderTask::loop() {
    if (uxQueueMessagesWaiting(p1DataQueue) > 0) {
        // Serial.println("Data sender task: Found data in queue to send");
        
        // Get the oldest package from the queue (FIFO behavior)
        DataPackage package;
        if (xQueueReceive(p1DataQueue, &package, 0) == pdTRUE) {
            // Serial.println("Data sender task: Retrieved package from queue");
            
            // Convert char array back to zap::Str for sending
            zap::Str dataStr(package.data);
            
            // Send the data from the package
            sendJWT(dataStr);
        }
    } else {
        // Serial.println("Data sender task: No data in queue to send");
    }
}

void DataSenderTask::sendJWT(const zap::Str& payload) {
    if (payload.length() == 0) {
        LOG_W(TAG, "Data sender task: Empty JWT, not sending");
        return;
    }

    zap::Str jwt = createP1JWT(PRIVATE_KEY_HEX, crypto_getId(), payload.c_str());
    
    // Serial.println("Data sender task: Sending JWT...");
    // Serial.print("Data sender task jwt:");
    // Serial.println(jwt.c_str());
    
    // Serial.print("Data sender task: Sending JWT to: ");
    // Serial.println(DATA_URL);
    
    // Close any previous connections and start a new one
    http.end();
    
    // Start the request
    if (http.begin(DATA_URL)) {
        // Add headers
        http.addHeader("Content-Type", "text/plain");
        
        // Send POST request with JWT as body
        int httpResponseCode = http.POST(jwt.c_str());
        
        if (httpResponseCode > 0) {
            LOG_I(TAG, "HTTP Response code: %d", httpResponseCode);
            zap::Str response(http.getString().c_str());
            LOG_D(TAG, "Response: %s", response.c_str());
        } else {
            LOG_W(TAG, "HTTP Error code: %d", httpResponseCode);
        }
        
        // Note: Don't call http.end() here to reuse the connection
        // The connection will be closed when needed or in the destructor
    } else {
        LOG_E(TAG, "Failed to connect to server");
    }
}