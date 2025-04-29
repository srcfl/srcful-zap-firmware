#include "endpoint_handlers.h"
#include "wifi/wifi_manager.h"
#include "crypto.h"
#include "debug.h"


#include "config.h"


// Crypto Info Handler Implementation
EndpointResponse CryptoInfoHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    
    JsonBuilder json;
    
    json.beginObject()
        .add("deviceName", "software_zap");
    
    zap::Str serialNumber = crypto_getId();
    json.add("serialNumber", serialNumber.c_str());
    
    zap::Str publicKey = crypto_get_public_key(PRIVATE_KEY_HEX);
    json.add("publicKey", publicKey.c_str());
    
    response.statusCode = 200;
    response.data = json.end();
    Serial.print("Response data: ");
    Serial.println(response.data.c_str());
    return response;
}

// Name Info Handler Implementation
EndpointResponse NameInfoHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    GQL::StringResponse gqlResponse = GQL::fetchGatewayName(crypto_getId());
    zap::Str name;
    
    if (gqlResponse.isSuccess()) {
        name = gqlResponse.data;
    } else {
        // Handle error case - use a default name or empty string
        name = "Unknown Gateway";
        Serial.print("Error fetching gateway name: ");
        Serial.println(gqlResponse.error.c_str());
    }
    
    JsonBuilder json;
    json.beginObject()
        .add("name", name.c_str());
    
    response.statusCode = 200;
    response.data = json.end();
    return response;
}

// Initialize Handler Implementation
EndpointResponse InitializeHandler::handle(const zap::Str& contents) {
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
    
    zap::Str deviceId = crypto_getId();
    zap::Str idAndWallet = deviceId + ":" + zap::Str(wallet);
    
    zap::Str signature = crypto_create_signature_hex(idAndWallet.c_str(), PRIVATE_KEY_HEX);
    
    JsonBuilder json;
    json.beginObject()
        .add("idAndWallet", idAndWallet.c_str())
        .add("signature", signature.c_str());
    
    response.statusCode = 200;
    response.data = json.end();
    
    return response;
}



// OTA Update Handler Implementation
EndpointResponse OTAUpdateHandler::handle(const zap::Str& contents) {
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
EndpointResponse DebugHandler::handle(const zap::Str& contents) {
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
EndpointResponse BLEStopHandler::handle(const zap::Str& contents) {
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

// Echo Handler Implementation
EndpointResponse EchoHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    // Create JSON response with the echoed data
    JsonBuilder json;
    json.beginObject()
        .add("echo", contents.c_str());
    
    response.statusCode = 200;
    response.data = json.end();
    
    return response;
}