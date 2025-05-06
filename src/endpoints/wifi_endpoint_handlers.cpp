#include "wifi_endpoint_handlers.h"
#include "endpoint_handlers.h"
#include "json_light/json_light.h"
#include <time.h> // For time()
#include "esp_timer.h" // For esp_timer_get_time()
#include "esp_system.h" // For ESP system info functions
#include "main_actions.h"
#include "../zap_log.h" // Added for logging

// Define TAG for logging
static const char* TAG = "wifi_endpoints";

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
        
        LOG_I(TAG, "Received WiFi config request: %s", contents.c_str());
        
        if (!hasSsid || !hasPsk) {
            LOG_W(TAG, "Missing ssid or psk in request");
            response.statusCode = 400;
            response.data = "{\"status\":\"error\",\"message\":\"Missing credentials\"}";
            return response;
        }
        
        zap::Str ssidStr = zap::Str(ssid);
        zap::Str password = zap::Str(psk);
        
        LOG_I(TAG, "Setting WiFi SSID: %s", ssidStr.c_str());
        LOG_I(TAG, "Setting WiFi password (length): %d", password.length());
        
        // Try to connect with new credentials
        LOG_I(TAG, "Attempting to connect to WiFi...");
        if (wifiManager.connectToWiFi(ssidStr, password)) {
            response.statusCode = 200;
            response.data = "{\"status\":\"success\",\"message\":\"WiFi credentials updated and connected\"}";
        } else {
            LOG_E(TAG, "Failed to connect to WiFi with provided credentials.");
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
    
    LOG_I(TAG, "Received WiFi reset request.");
    // Clear saved credentials from persistent storage
    wifiManager.clearCredentials();
    
    MainActions::triggerAction(MainActions::Type::WIFI_DISCONNECT, 5000);   
    
    response.statusCode = 200;
    response.data = "{\"status\":\"success\",\"message\":\"WiFi credentials cleared, disconnecting in 5 seconds\"}";
    return response;
}


// WiFi Status Handler Implementation
EndpointResponse WiFiStatusHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    LOG_D(TAG, "Received WiFi status request.");
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
    
    LOG_I(TAG, "Received WiFi scan request.");
    JsonBuilder json;
    json.beginObject();

    wifiManager.setScanWiFiNetworks(true);
    json.add("status", "sucess");
    json.add("message", "scan initiated");
    
    response.data = json.end();
    return response;
}
