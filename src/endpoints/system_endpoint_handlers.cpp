#include "firmware_version.h"
#include "system_endpoint_handlers.h"
#include "crypto.h"
#include "wifi/wifi_manager.h"
#include "json_light/json_light.h"
#include "config.h"

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

EndpointResponse SystemRebootHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";

    // Reboot the system
    esp_restart();

    return response;
}