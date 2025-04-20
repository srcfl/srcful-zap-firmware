#include <Arduino.h>
#include "endpoint_types.h"
#include "endpoints.h"
#include <WiFi.h>
#include <esp_system.h>
#include "../crypto.h"
#include "../json_light/json_light.h"



// Crypto Sign Handler Implementation
EndpointResponse CryptoSignHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    // Parse the incoming JSON request
    JsonParser parser(contents.c_str());
    char message[512] = {0};
    char timestamp[64] = {0};
    bool hasMessage = parser.getString("message", message, sizeof(message));
    bool hasTimestamp = parser.getString("timestamp", timestamp, sizeof(timestamp));
    
    // Get message to sign if provided, otherwise use an empty string
    zap::Str messageStr = hasMessage ? zap::Str(message) : "";
    
    // Check for pipe characters which are not allowed
    if (hasMessage && messageStr.indexOf('|') != -1) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"Message cannot contain | characters\"}";
        return response;
    }
    
    // Generate nonce (random string)
    zap::Str nonce = zap::Str(random(100000, 999999));
    // String nonce = "313589";    // hard coded to avoid nonce fragmentation in ble case
    
    // Get timestamp - use provided timestamp or generate one
    zap::Str timestampStr;
    if (hasTimestamp) {
        timestampStr = zap::Str(timestamp);
        
        // Check for pipe characters which are not allowed
        if (timestampStr.indexOf('|') != -1) {
            response.statusCode = 400;
            response.data = "{\"status\":\"error\",\"message\":\"Timestamp cannot contain | characters\"}";
            return response;
        }
    } else {
        // Generate timestamp in UTC format (Y-m-dTH:M:SZ)
        time_t now;
        time(&now);
        char timestamp[24];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
        timestampStr = zap::Str(timestamp);
    }
    
    // Get device serial number
    zap::Str serialNumber = crypto_getId();
    
    // Create the combined message: message|nonce|timestamp|serial
    // If message is empty, don't include the initial pipe character
    zap::Str combinedMessage;
    if (messageStr.length() > 0) {
        combinedMessage = messageStr + "|" + nonce + "|" + timestampStr + "|" + serialNumber;
    } else {
        combinedMessage = nonce + "|" + timestampStr + "|" + serialNumber;
    }
    
    // Sign the combined message
    extern const char* PRIVATE_KEY_HEX;
    zap::Str signature = crypto_create_signature_hex(combinedMessage.c_str(), PRIVATE_KEY_HEX);
    
    // Create the response as a json string, we put the signature first so this is sent first to void fragmented signatures in ble case
    zap::Str responseString = "{\"sign\":\"" + signature + "\",\"message\":\"" + combinedMessage + "\"}";
    response.data = responseString;
    response.statusCode = 200;
    return response;
}