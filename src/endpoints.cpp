#include <Arduino.h>
#include "endpoint_types.h"
#include "endpoints.h"
#include <WiFi.h>
#include <esp_system.h>
#include "crypto.h"
#include "graphql.h"
#include "html.h"
#include "firmware_version.h"

// External function declarations
extern bool connectToWiFi(const String& ssid, const String& password, bool updateGlobals = true);

EndpointResponse handleWiFiConfig(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.method != HttpMethod::POST) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    if (request.content.length() > 0) {
        StaticJsonDocument<128> doc;
        DeserializationError error = deserializeJson(doc, request.content);
        
        Serial.println("Received WiFi config request:");
        Serial.println(request.content);
        
        if (error) {
            Serial.println("JSON parsing failed");
            response.statusCode = 400;
            response.data = "{\"status\":\"error\",\"message\":\"Invalid JSON\"}";
            return response;
        }
        
        if (!doc.containsKey("ssid") || !doc.containsKey("psk")) {
            Serial.println("Missing ssid or psk in request");
            response.statusCode = 400;
            response.data = "{\"status\":\"error\",\"message\":\"Missing credentials\"}";
            return response;
        }
        
        String ssid = doc["ssid"].as<const char*>();
        String password = doc["psk"].as<const char*>();
        
        Serial.print("Setting WiFi SSID: ");
        Serial.println(ssid);
        Serial.println("Setting WiFi password (length): " + String(password.length()));
        
        // Try to connect with new credentials
        Serial.println("Attempting to connect to WiFi...");
        if (connectToWiFi(ssid, password)) {
            response.statusCode = 200;
            response.data = "{\"status\":\"success\",\"message\":\"WiFi credentials updated and connected\"}";
        } else {
            response.statusCode = 500;
            response.data = "{\"status\":\"error\",\"message\":\"Failed to connect with provided credentials\"}";
        }
    }
    
    return response;
}

EndpointResponse handleSystemInfo(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.method != HttpMethod::GET) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    StaticJsonDocument<384> doc;
    extern String getId();
    String deviceId = getId();
    
    doc["deviceId"] = deviceId;
    doc["heap"] = ESP.getFreeHeap();
    doc["cpuFreq"] = ESP.getCpuFreqMHz();
    doc["flashSize"] = ESP.getFlashChipSize();
    doc["sdkVersion"] = ESP.getSdkVersion();
    doc["firmwareVersion"] = getFirmwareVersion();
    
    extern const char* PRIVATE_KEY_HEX;
    String publicKey = crypto_get_public_key(PRIVATE_KEY_HEX);
    doc["publicKey"] = publicKey;
    
    if (WiFi.status() == WL_CONNECTED) {
        doc["wifiStatus"] = "connected";
        doc["localIP"] = WiFi.localIP().toString();
        doc["ssid"] = WiFi.SSID();
        doc["rssi"] = WiFi.RSSI();
    } else {
        doc["wifiStatus"] = "disconnected";
    }
    
    response.statusCode = 200;
    serializeJson(doc, response.data);
    return response;
}

EndpointResponse handleWiFiReset(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.method != HttpMethod::POST) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    WiFi.disconnect();
    extern String configuredSSID;
    extern String configuredPassword;
    extern bool isProvisioned;
    extern void setupAP();
    
    configuredSSID = "";
    configuredPassword = "";
    isProvisioned = false;
    
    setupAP();
    
    response.statusCode = 200;
    response.data = "{\"status\":\"success\",\"message\":\"WiFi reset successful\"}";
    return response;
}

EndpointResponse handleCryptoInfo(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.method != HttpMethod::GET) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    StaticJsonDocument<512> doc;
    
    doc["deviceName"] = "software_zap";
    
    extern String getId();
    doc["serialNumber"] = getId();
    
    extern const char* PRIVATE_KEY_HEX;
    String publicKey = crypto_get_public_key(PRIVATE_KEY_HEX);
    doc["publicKey"] = publicKey;
    
    response.statusCode = 200;
    serializeJson(doc, response.data);
    return response;
}

EndpointResponse handleNameInfo(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.method != HttpMethod::GET) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    extern String getId();
    String name = fetchGatewayName(getId());
    
    StaticJsonDocument<256> doc;
    doc["name"] = name;
    
    response.statusCode = 200;
    serializeJson(doc, response.data);
    return response;
}

EndpointResponse handleWiFiStatus(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.method != HttpMethod::GET) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    StaticJsonDocument<1024> doc;  // Increased size to accommodate SSID list
    JsonArray ssids = doc.createNestedArray("ssids");
    
    // Get the cached scan results
    extern std::vector<String> lastScanResults;
    for (const String& ssid : lastScanResults) {
        ssids.add(ssid);
    }
    
    // Add connected network info if connected
    if (WiFi.status() == WL_CONNECTED) {
        doc["connected"] = WiFi.SSID();
    } else {
        doc["connected"] = nullptr;  // JSON null if not connected
    }
    
    response.statusCode = 200;
    serializeJson(doc, response.data);
    return response;
}

EndpointResponse handleWiFiScan(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.method != HttpMethod::GET) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    // Start async WiFi scan
    WiFi.scanNetworks(true);  // true = async scan
    
    StaticJsonDocument<128> doc;
    doc["status"] = "scan initiated";
    
    response.statusCode = 200;
    serializeJson(doc, response.data);
    return response;
}

EndpointResponse handleInitializeForm(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "text/html";
    response.statusCode = 200;
    response.data = FPSTR(INITIALIZE_FORM_HTML);
    return response;
}

EndpointResponse handleInitialize(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.method != HttpMethod::POST) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    if (request.content.length() == 0) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"No body provided\"}";
        return response;
    }

    StaticJsonDocument<512> requestDoc;
    DeserializationError error = deserializeJson(requestDoc, request.content);
    
    if (error || !requestDoc.containsKey("wallet")) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"Invalid JSON or missing wallet\"}";
        return response;
    }

    extern String getId();
    String deviceId = getId();
    String idAndWallet = deviceId + ":" + requestDoc["wallet"].as<String>();
    
    extern const char* PRIVATE_KEY_HEX;
    String signature = crypto_create_signature_hex(idAndWallet.c_str(), PRIVATE_KEY_HEX);
    
    StaticJsonDocument<512> doc;
    doc["idAndWallet"] = idAndWallet;
    doc["signature"] = signature;
    
    response.statusCode = 200;
    serializeJson(doc, response.data);
    
    return response;
}

EndpointResponse handleCryptoSign(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.method != HttpMethod::POST) {
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
    extern String getId();
    String serialNumber = getId();
    
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
    
    // Create the response
    StaticJsonDocument<1024> responseDoc;
    responseDoc["message"] = combinedMessage;
    responseDoc["sign"] = signature;
    
    response.statusCode = 200;
    serializeJson(responseDoc, response.data);
    return response;
} 