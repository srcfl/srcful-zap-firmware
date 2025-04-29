#include "wifi_endpoint_handlers.h"
#include "endpoint_handlers.h"
#include "json_light/json_light.h"
#include <time.h> // For time()
#include "esp_timer.h" // For esp_timer_get_time()
#include "esp_system.h" // For ESP system info functions

// WiFi Config Handler Implementation
EndpointResponse WiFiConfigHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (contents.length() > 0) {
        JsonParser parser(contents.c_str());
        char ssid[64] = {0};
        char psk[64] = {0};
        bool hasSsid = parser.getString("ssid", ssid, sizeof(ssid));
        bool hasPsk = parser.getString("psk", psk, sizeof(psk));
        
        Serial.println("Received WiFi config request:");
        Serial.println(contents.c_str());
        
        if (!hasSsid || !hasPsk) {
            Serial.println("Missing ssid or psk in request");
            response.statusCode = 400;
            response.data = "{\"status\":\"error\",\"message\":\"Missing credentials\"}";
            return response;
        }
        
        zap::Str ssidStr = zap::Str(ssid);
        zap::Str password = zap::Str(psk);
        
        Serial.print("Setting WiFi SSID: ");
        Serial.println(ssidStr.c_str());
        Serial.println(("Setting WiFi password (length): " + zap::Str(password.length())).c_str());
        
        // Try to connect with new credentials
        Serial.println("Attempting to connect to WiFi...");
        if (wifiManager.connectToWiFi(ssidStr, password)) {
            response.statusCode = 200;
            response.data = "{\"status\":\"success\",\"message\":\"WiFi credentials updated and connected\"}";
        } else {
            response.statusCode = 500;
            response.data = "{\"status\":\"error\",\"message\":\"Failed to connect with provided credentials\"}";
        }
    }
    
    return response;
}


// WiFi Reset Handler Implementation
EndpointResponse WiFiResetHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    // Clear saved credentials from persistent storage
    wifiManager.clearCredentials();
    
    // Disconnect from WiFi
    WiFi.disconnect();
    
    response.statusCode = 200;
    response.data = "{\"status\":\"success\",\"message\":\"WiFi reset, credentials cleared\"}";
    return response;
}


// WiFi Status Handler Implementation
EndpointResponse WiFiStatusHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    JsonBuilder json;
    json.beginObject();
       
    // Add the SSIDs to the JSON
    json.addArray("ssids", wifiManager.getLastScanResults());
    
    // Add connected network info if connected
    if (wifiManager.isConnected()) {
        json.add("connected", wifiManager.getConfiguredSSID().c_str());
    } else {
        json.add("connected", nullptr);  // JSON null if not connected
    }
    
    response.statusCode = 200;
    response.data = json.end();
    return response;
}


// WiFi Scan Handler Implementation
EndpointResponse WiFiScanHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    JsonBuilder json;
    json.beginObject();
    
    // Get the cached scan results
    json.addArray("ssids", wifiManager.getLastScanResults());
    
    // Add connected network info if connected
    if (wifiManager.isConnected()) {
        json.add("connected", wifiManager.getConfiguredSSID().c_str());
    } else {
        json.add("connected", nullptr);  // JSON null if not connected
    }
    
    response.data = json.end();
    return response;
}
