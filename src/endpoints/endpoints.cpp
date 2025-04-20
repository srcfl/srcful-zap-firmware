#include <Arduino.h>
#include "json_light/json_light.h"
#include "endpoint_types.h"
#include "endpoints.h"
#include <WiFi.h>
#include <esp_system.h>
#include "crypto.h"
#include "../backend/graphql.h"
#include "html.h"
#include "firmware_version.h"
#include "wifi/wifi_manager.h"
#include "endpoint_handlers.h"

// External variable declarations
extern WifiManager wifiManager;
#if defined(USE_BLE_SETUP)
extern BLEHandler bleHandler;
#endif

WiFiConfigHandler g_wifiConfigHandler(wifiManager);
SystemInfoHandler g_systemInfoHandler;
WiFiResetHandler g_wifiResetHandler(wifiManager);
CryptoInfoHandler g_cryptoInfoHandler;
NameInfoHandler g_nameInfoHandler;
WiFiStatusHandler g_wifiStatusHandler(wifiManager);
WiFiScanHandler g_wifiScanHandler(wifiManager);
InitializeHandler g_initializeHandler;
CryptoSignHandler g_cryptoSignHandler;
OTAUpdateHandler g_otaUpdateHandler;
DebugHandler g_debugHandler;


#if defined(USE_BLE_SETUP)
BLEStopHandler g_bleStopHandler(bleHandler);
#endif


EndpointResponse handleWiFiConfig(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::POST) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    if (request.content.length() > 0) {
        JsonParser parser(request.content.c_str());
        char ssid[64] = {0};
        char psk[64] = {0};
        bool hasSsid = parser.getString("ssid", ssid, sizeof(ssid));
        bool hasPsk = parser.getString("psk", psk, sizeof(psk));
        
        Serial.println("Received WiFi config request:");
        Serial.println(request.content.c_str());
        
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
        Serial.println("Setting WiFi password (length): " + String(password.length()));
        
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

EndpointResponse handleSystemInfo(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::GET) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    JsonBuilder json;
    zap::Str deviceId = crypto_getId();
    
    json.beginObject()
        .add("deviceId", deviceId.c_str())
        .add("heap", (int)ESP.getFreeHeap())
        .add("cpuFreq", ESP.getCpuFreqMHz())
        .add("flashSize", ESP.getFlashChipSize())
        .add("sdkVersion", ESP.getSdkVersion())
        .add("firmwareVersion", getFirmwareVersion());
    
    extern const char* PRIVATE_KEY_HEX;
    zap::Str publicKey = crypto_get_public_key(PRIVATE_KEY_HEX);
    json.add("publicKey", publicKey.c_str());
    
    if (wifiManager.isConnected()) {
        json.add("wifiStatus", "connected")
            .add("localIP", wifiManager.getLocalIP().c_str())
            .add("ssid", wifiManager.getConfiguredSSID().c_str())
            .add("rssi", WiFi.RSSI()); // Still need direct WiFi access for RSSI
    } else {
        json.add("wifiStatus", "disconnected");
    }
    
    response.statusCode = 200;
    response.data = json.end();
    return response;
}

EndpointResponse handleWiFiReset(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::POST) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }
    
    // Disconnect from WiFi
    WiFi.disconnect();
    
    // Setup AP mode - no need for this
    // wifiManager.setupAP(AP_SSID, AP_PASSWORD);
    
    response.statusCode = 200;
    response.data = "{\"status\":\"success\",\"message\":\"WiFi reset, AP mode activated\"}";
    return response;
}

EndpointResponse handleCryptoInfo(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::GET) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }
    
    JsonBuilder json;
    
    json.beginObject()
        .add("deviceName", "software_zap");
    
    zap::Str serialNumber = crypto_getId();
    json.add("serialNumber", serialNumber.c_str());
    
    extern const char* PRIVATE_KEY_HEX;
    
    zap::Str publicKey = crypto_get_public_key(PRIVATE_KEY_HEX);
    json.add("publicKey", publicKey.c_str());
    
    response.statusCode = 200;
    response.data = json.end();
    Serial.print("Response data: ");
    Serial.println(response.data.c_str());
    return response;
}

EndpointResponse handleNameInfo(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::GET) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }
    
    zap::Str name = fetchGatewayName(crypto_getId());
    
    JsonBuilder json;
    json.beginObject()
        .add("name", name.c_str());
    
    response.statusCode = 200;
    response.data = json.end();
    return response;
}

EndpointResponse handleWiFiStatus(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::GET) {
        JsonBuilder json;
        json.beginObject()
            .add("status", "error")
            .add("message", "Method not allowed");
        
        response.data = json.end();
        return response;
    }
    
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
    
    response.data = json.end();
    return response;
}

EndpointResponse handleWiFiScan(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::GET) {
        JsonBuilder json;
        json.beginObject()
            .add("status", "error")
            .add("message", "Method not allowed");
        
        response.data = json.end();
        return response;
    }
    
    JsonBuilder json;
    json.beginObject();
    
    // Create an array for SSIDs
    const char* ssids[20]; // Assuming max 20 networks
    int ssidCount = 0;
    
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

EndpointResponse handleInitialize(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::POST) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }
    
    JsonParser parser(request.content.c_str());
    char wallet[128] = {0};
    bool hasWallet = parser.getString("wallet", wallet, sizeof(wallet));
    
    if (!hasWallet) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"Invalid JSON or missing wallet\"}";
        return response;
    }
    
    zap::Str deviceId = crypto_getId();
    zap::Str idAndWallet = deviceId + ":" + zap::Str(wallet);
    
    extern const char* PRIVATE_KEY_HEX;
    zap::Str signature = crypto_create_signature_hex(idAndWallet.c_str(), PRIVATE_KEY_HEX);
    
    JsonBuilder json;
    json.beginObject()
        .add("idAndWallet", idAndWallet.c_str())
        .add("signature", signature.c_str());
    
    response.statusCode = 200;
    response.data = json.end();
    
    return response;
}

