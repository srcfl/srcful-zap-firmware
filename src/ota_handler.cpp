#include "ota_handler.h"
#include "firmware_version.h"
#include <ArduinoJson.h>
#include <Update.h>
#include <HTTPClient.h>
#include <algorithm>

EndpointResponse OTAHandler::handleOTAUpdate(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::POST) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    // Parse the request body
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, request.content);
    
    if (error) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"Invalid JSON\"}";
        return response;
    }
    
    if (!doc.containsKey("url")) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"Missing firmware URL\"}";
        return response;
    }

    // Check version if provided
    if (doc.containsKey("version")) {
        String newVersion = doc["version"].as<String>();
        String currentVersion = getFirmwareVersion();
        
        // Simple string comparison for now
        if (newVersion == currentVersion) {
            response.statusCode = 200;
            response.data = "{\"status\":\"success\",\"message\":\"Already running version " + currentVersion + "\"}";
            return response;
        }
        
        Serial.println("Current version: " + currentVersion);
        Serial.println("New version: " + newVersion);
    }
    
    String firmwareUrl = doc["url"].as<String>();
    
    Serial.println("Free heap before update: " + String(ESP.getFreeHeap()));
    Serial.println("Starting OTA update from: " + firmwareUrl);
    
    // Create HTTP client
    HTTPClient http;
    WiFiClientSecure client;
    
    // Configure SSL client to accept self-signed certificates
    client.setInsecure();
    
    // Begin the connection
    http.begin(client, firmwareUrl);
    
    // Get the firmware
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.println("HTTP GET failed: " + String(httpCode));
        http.end();
        response.statusCode = 500;
        response.data = "{\"status\":\"error\",\"message\":\"HTTP GET failed: " + String(httpCode) + "\"}";
        return response;
    }
    
    // Get the size of the firmware
    int contentLength = http.getSize();
    Serial.println("Firmware size: " + String(contentLength) + " bytes");
    
    // Start the update
    if (!Update.begin(contentLength)) {
        Serial.println("Failed to start update: " + String(Update.getError()));
        http.end();
        response.statusCode = 500;
        response.data = "{\"status\":\"error\",\"message\":\"Failed to start update\"}";
        return response;
    }
    
    // Get the stream
    WiFiClient *stream = http.getStreamPtr();
    
    // Write update data
    size_t written = 0;
    uint8_t buff[256] = { 0 }; // Even smaller buffer
    
    while (written < contentLength) {
        if (!stream->connected()) {
            Serial.println("Connection lost");
            http.end();
            response.statusCode = 500;
            response.data = "{\"status\":\"error\",\"message\":\"Connection lost\"}";
            return response;
        }
        
        size_t available = stream->available();
        if (available) {
            size_t bytes = stream->readBytes(buff, (available < sizeof(buff)) ? available : sizeof(buff));
            if (bytes == 0) {
                Serial.println("Failed to read data");
                http.end();
                response.statusCode = 500;
                response.data = "{\"status\":\"error\",\"message\":\"Failed to read data\"}";
                return response;
            }
            
            if (Update.write(buff, bytes) != bytes) {
                Serial.println("Failed to write update data");
                http.end();
                response.statusCode = 500;
                response.data = "{\"status\":\"error\",\"message\":\"Failed to write update data\"}";
                return response;
            }
            
            written += bytes;
            Serial.printf("Written %d bytes (%.1f%%) - Free heap: %d\n", 
                written, 
                (written * 100.0) / contentLength,
                ESP.getFreeHeap());
            
            // Add a small delay between chunks
            delay(10);
            
            // Check for errors after each chunk
            if (Update.hasError()) {
                Serial.println("Update error: " + String(Update.getError()));
                http.end();
                response.statusCode = 500;
                response.data = "{\"status\":\"error\",\"message\":\"Update error: " + String(Update.getError()) + "\"}";
                return response;
            }
        }
        delay(1);
    }
    
    // Close the HTTP connection before finalizing the update
    http.end();
    
    // Finalize the update
    if (!Update.end(true)) { // true means we want to reboot after update
        Serial.println("Update failed to finalize: " + String(Update.getError()));
        response.statusCode = 500;
        response.data = "{\"status\":\"error\",\"message\":\"Update failed to finalize\"}";
        return response;
    }
    
    Serial.println("Update complete, preparing to reboot...");
    
    // Send success response
    response.statusCode = 200;
    response.data = "{\"status\":\"success\",\"message\":\"Update successful, device will restart\"}";
    
    // Give time for the response to be sent
    delay(2000);
    
    // Reboot the device
    ESP.restart();
    
    return response;
}

bool OTAHandler::verifyFirmware(const uint8_t* data, size_t len) {
    // TODO: Implement firmware verification
    // This could include:
    // - Checking firmware signature
    // - Verifying firmware size
    // - Checking firmware version
    return true;
}

void OTAHandler::updateProgress(size_t current, size_t total) {
    // TODO: Implement progress reporting
    // This could be used to send progress updates to a connected client
    Serial.printf("Progress: %d%%\n", (current * 100) / total);
} 