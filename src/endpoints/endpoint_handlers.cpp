#include "endpoint_handlers.h"
#include "wifi/wifi_manager.h"
#include "crypto.h"
#include "debug.h"
#include "main_actions.h"

#include "backend/graphql.h"


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
    return response;
}

// Name Info Handler Implementation
EndpointResponse NameInfoHandler::handle(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    GQL::StringResponse gqlResponse = GQL::fetchGatewayName(crypto_getId());
    zap::Str name;
    
    JsonBuilder json;
    if (gqlResponse.isSuccess()) {
        name = gqlResponse.data;
        json.beginObject()
            .add("name", name.c_str());
    
        response.statusCode = 200;
    } else {
        // Handle error case - use a default name or empty string
        name = "Unknown";
        json.beginObject()
            .add("name", name.c_str())
            .add("error", gqlResponse.error.c_str())
            .add("status", "error");
    
        response.statusCode = 500;
    }
    
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
    
    
    MainActions::triggerAction(MainActions::Type::BLE_DISCONNECT, 1000);
    response.statusCode = 200;
    response.data = "{\"status\":\"success\",\"message\":\"BLE stopping...\"}";
    
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