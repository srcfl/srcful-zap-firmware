#include <Arduino.h>
#include "../endpoint_types.h"
#include "../endpoints.h"
#include <WiFi.h>
#include <esp_system.h>
#include "../crypto.h"
#include <ArduinoJson.h>


EndpointResponse handleCryptoSign(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::POST) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    // Parse the incoming JSON request
    StaticJsonDocument<512> requestDoc;
    DeserializationError error = deserializeJson(requestDoc, request.content);
    
    if (error) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"Invalid JSON\"}";
        return response;
    }
    
    // Get message to sign if provided, otherwise use an empty string
    String message = "";
    if (requestDoc.containsKey("message")) {
        message = requestDoc["message"].as<String>();
        
        // Check for pipe characters which are not allowed
        if (message.indexOf('|') != -1) {
            response.statusCode = 400;
            response.data = "{\"status\":\"error\",\"message\":\"Message cannot contain | characters\"}";
            return response;
        }
    }
    
    // Generate nonce (random string)
    String nonce = String(random(100000, 999999));
    // String nonce = "313589";    // hard coded to avoid nonce fragmentation in ble case
    
    
    // Get timestamp - use provided timestamp or generate one
    String timestampStr;
    if (requestDoc.containsKey("timestamp")) {
        timestampStr = requestDoc["timestamp"].as<String>();
        
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
        timestampStr = String(timestamp);
    }
    
    // Get device serial number
    String serialNumber = crypto_getId();
    
    // Create the combined message: message|nonce|timestamp|serial
    // If message is empty, don't include the initial pipe character
    String combinedMessage;
    if (message.length() > 0) {
        combinedMessage = message + "|" + nonce + "|" + timestampStr + "|" + serialNumber;
    } else {
        combinedMessage = nonce + "|" + timestampStr + "|" + serialNumber;
    }
    
    // Sign the combined message
    extern const char* PRIVATE_KEY_HEX;
    String signature = crypto_create_signature_hex(combinedMessage.c_str(), PRIVATE_KEY_HEX);
    
    // Create the response as a json string, we put the signature first so this is sent first to void fragmented signatures in ble case
    String responseString = "{\"sign\":\"" + signature + "\",\"message\":\"" + combinedMessage + "\"}";
    response.data = responseString;
    response.statusCode = 200;
    return response;
} 