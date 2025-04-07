#include "ota_handler.h"
#include "firmware_version.h"
#include "json_light/json_light.h"
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
    JsonParser parser(request.content.c_str());
    char url[256] = {0};
    char version[64] = {0};
    bool hasUrl = parser.getString("url", url, sizeof(url));
    bool hasVersion = parser.getString("version", version, sizeof(version));
    
    if (!hasUrl) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"Missing firmware URL\"}";
        return response;
    }

    // Check version if provided
    if (hasVersion) {
        String newVersion = String(version);
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
    
    String firmwareUrl = String(url);
    
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
            response.data = "{\"status\":\"error\",\"message\":\"Connection lost during update\"}";
            return response;
        }
        
        size_t available = stream->available();
        if (available) {
            size_t bytesToRead = min(available, sizeof(buff));
            size_t bytesRead = stream->readBytes(buff, bytesToRead);
            
            if (bytesRead > 0) {
                if (Update.write(buff, bytesRead) != bytesRead) {
                    Serial.println("Failed to write update data");
                    http.end();
                    response.statusCode = 500;
                    response.data = "{\"status\":\"error\",\"message\":\"Failed to write update data\"}";
                    return response;
                }
                written += bytesRead;
                updateProgress(written, contentLength);
            }
        }
        delay(1);
    }
    
    if (Update.end()) {
        Serial.println("Update complete");
        http.end();
        response.statusCode = 200;
        response.data = "{\"status\":\"success\",\"message\":\"Update complete\"}";
        return response;
    } else {
        Serial.println("Update failed: " + String(Update.getError()));
        http.end();
        response.statusCode = 500;
        response.data = "{\"status\":\"error\",\"message\":\"Update failed: " + String(Update.getError()) + "\"}";
        return response;
    }
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