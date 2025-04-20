#include "endpoint_handlers.h"
#include "wifi/wifi_manager.h"
#include "crypto.h"
#include "debug.h"
#include "firmware_version.h"

// System Info Handler Implementation
EndpointResponse SystemInfoHandler::handle(const String& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    

    JsonBuilder json;
    String deviceId = crypto_getId();
    
    json.beginObject()
        .add("deviceId", deviceId.c_str())
        .add("heap", (int)ESP.getFreeHeap())
        .add("cpuFreq", ESP.getCpuFreqMHz())
        .add("flashSize", ESP.getFlashChipSize())
        .add("sdkVersion", ESP.getSdkVersion())
        .add("firmwareVersion", getFirmwareVersion());
    
    String publicKey = crypto_get_public_key(PRIVATE_KEY_HEX);
    json.add("publicKey", publicKey.c_str());
    
    // if (wifiManager.isConnected()) {
    //     json.add("wifiStatus", "connected")
    //         .add("localIP", wifiManager.getLocalIP().c_str())
    //         .add("ssid", wifiManager.getConfiguredSSID().c_str())
    //         .add("rssi", WiFi.RSSI()); // Still need direct WiFi access for RSSI
    // } else {
    //     json.add("wifiStatus", "disconnected");
    // }
    
    response.statusCode = 200;
    response.data = json.end();
    return response;
}



// Crypto Info Handler Implementation
EndpointResponse CryptoInfoHandler::handle(const String& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    
    JsonBuilder json;
    
    json.beginObject()
        .add("deviceName", "software_zap");
    
    String serialNumber = crypto_getId();
    json.add("serialNumber", serialNumber.c_str());
    
    String publicKey = crypto_get_public_key(PRIVATE_KEY_HEX);
    json.add("publicKey", publicKey.c_str());
    
    response.statusCode = 200;
    response.data = json.end();
    Serial.print("Response data: ");
    Serial.println(response.data);
    return response;
}

// Name Info Handler Implementation
EndpointResponse NameInfoHandler::handle(const String& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    
    
    String name = fetchGatewayName(crypto_getId());
    
    JsonBuilder json;
    json.beginObject()
        .add("name", name.c_str());
    
    response.statusCode = 200;
    response.data = json.end();
    return response;
}

// Initialize Handler Implementation
EndpointResponse InitializeHandler::handle(const String& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    JsonParser parser(contents.c_str());
    char wallet[128] = {0};
    bool hasWallet = parser.getString("wallet", wallet, sizeof(wallet));
    
    if (!hasWallet) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"Invalid JSON or missing wallet\"}";
        return response;
    }
    
    String deviceId = crypto_getId();
    String idAndWallet = deviceId + ":" + String(wallet);
    
    String signature = crypto_create_signature_hex(idAndWallet.c_str(), PRIVATE_KEY_HEX);
    
    JsonBuilder json;
    json.beginObject()
        .add("idAndWallet", idAndWallet.c_str())
        .add("signature", signature.c_str());
    
    response.statusCode = 200;
    response.data = json.end();
    
    return response;
}



// OTA Update Handler Implementation
EndpointResponse OTAUpdateHandler::handle(const String& contents) {
    // Implementation for OTA update endpoint
    EndpointResponse response;
    response.contentType = "application/json";
    
    JsonParser parser(contents.c_str());
    
    // Parse the request content
    char url[256] = {0};
    bool hasUrl = parser.getString("url", url, sizeof(url));
    
    if (!hasUrl) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"Invalid JSON or missing URL\"}";
        return response;
    }
    
    // Start OTA update
    // This would typically call an OTA update function
    // For now, just return success
    response.statusCode = 200;
    response.data = "{\"status\":\"success\",\"message\":\"OTA update started\"}";
    
    return response;
}

// Debug Info Handler Implementation
EndpointResponse DebugHandler::handle(const String& contents) {
    // Implementation for debug info endpoint
    EndpointResponse response;
    response.contentType = "application/json";
    
    response.statusCode = 200;
    
    JsonBuilder json;
    json.beginObject()
        .add("status", "success");
    Debug::getJsonReport(json);
    response.data = json.end();
    
    return response;
}

// BLE Stop Handler Implementation
EndpointResponse BLEStopHandler::handle(const String& contents) {
    // Implementation for BLE stop endpoint
    EndpointResponse response;
    response.contentType = "application/json";
    
    // Stop BLE
    // This would typically call a BLE stop function
    // For now, just return success
    bleHandler.stop();
    response.statusCode = 200;
    response.data = "{\"status\":\"success\",\"message\":\"BLE stopped\"}";
    
    return response;
} 