#include "wifi_endpoint_handlers.h"
#include "endpoint_handlers.h"
#include "json_light/json_light.h"
#include "firmware_version.h"
#include "crypto.h"
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
    
    // Setup AP mode
    wifiManager.setupAP(AP_SSID, AP_PASSWORD);
    
    response.statusCode = 200;
    response.data = "{\"status\":\"success\",\"message\":\"WiFi reset, credentials cleared, AP mode activated\"}";
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
#include <driver/temp_sensor.h>
// temp_sensor_config_t internal_temp_sensor_config = TSENS_CONFIG_DEFAULT();

SystemInfoHandler::SystemInfoHandler(const WifiManager& wifiManager) : wifiManager(wifiManager) {
    temp_sensor_config_t temp_sensor_config = TSENS_CONFIG_DEFAULT();
    temp_sensor_set_config(temp_sensor_config);

}

// System Info Handler Implementation
EndpointResponse SystemInfoHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";

    JsonBuilder json;
    json.beginObject();

    // Add top-level fields
    json.add("time_utc_sec", (uint32_t)time(nullptr));

    // Convert uptime to string before adding
    json.add("uptime_seconds", (esp_timer_get_time() / (uint64_t)1000000));

    float temperature;
    esp_err_t result = temp_sensor_read_celsius(&temperature);

    json.add("temperature_celsius", temperature);
    
    // Memory info as an object
    json.beginObject("memory_MB");
    
    float totalHeapMB = (float)ESP.getHeapSize() / (1024.0f * 1024.0f);
    float freeHeapMB = (float)ESP.getFreeHeap() / (1024.0f * 1024.0f);
    float usedHeapMB = totalHeapMB - freeHeapMB;
    float percentUsed = (totalHeapMB > 0) ? (usedHeapMB / totalHeapMB * 100.0f) : 0.0f;
    
    json.add("total", totalHeapMB);
    
    json.add("available", freeHeapMB);
    json.add("free", freeHeapMB);
    
    json.add("used", usedHeapMB);
    
    json.add("percent_used", percentUsed);
    json.endObject(); // End memory_MB

    // add zeros for "processes_average": {"last_1min": 0.01, "last_5min": 0.02, "last_15min": 0.0}
    json.beginObject("processes_average")
        .add("last_1min", 0)
        .add("last_5min", 0)
        .add("last_15min", 0)
        .endObject();
    
    // Begin nested "zap" object for other info
    json.beginObject("zap");
    
    // Device info
    zap::Str deviceId = crypto_getId();
    json.add("deviceId", deviceId.c_str());
    json.add("cpuFreqMHz", ESP.getCpuFreqMHz());
    
    json.add("flashSizeMB", (float)ESP.getFlashChipSize() / (1024.0f * 1024.0f));
    
    json.add("sdkVersion", ESP.getSdkVersion());
    json.add("firmwareVersion", getFirmwareVersion());
    
    zap::Str publicKey = crypto_get_public_key(PRIVATE_KEY_HEX);
    json.add("publicKey", publicKey.c_str());
    
    json.beginObject("network");
    // WiFi status
    if (wifiManager.isConnected()) {
        json.add("wifiStatus", "connected")
            .add("localIP", wifiManager.getLocalIP().c_str())
            .add("ssid", wifiManager.getConfiguredSSID().c_str())
            .add("rssi", WiFi.RSSI());
    } else {
        json.add("wifiStatus", "disconnected");
    }
    json.endObject(); // End network

    json.endObject(); // End zap object
    
    response.statusCode = 200;
    response.data = json.end();
    return response;
}