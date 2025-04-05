#include "endpoint_mapper.h"
#include "endpoint_types.h"
#include "ble_handler.h"
#include "ota_handler.h"

// Define path constants
const char* EndpointMapper::WIFI_CONFIG_PATH = "/api/wifi";
const char* EndpointMapper::SYSTEM_INFO_PATH = "/api/system/info";
const char* EndpointMapper::WIFI_RESET_PATH = "/api/wifi";
const char* EndpointMapper::CRYPTO_INFO_PATH = "/api/crypto";
const char* EndpointMapper::NAME_INFO_PATH = "/api/name";
const char* EndpointMapper::WIFI_STATUS_PATH = "/api/wifi";
const char* EndpointMapper::WIFI_SCAN_PATH = "/api/wifi/scan";
const char* EndpointMapper::BLE_STOP_PATH = "/api/ble/stop";
const char* EndpointMapper::CRYPTO_SIGN_PATH = "/api/crypto/sign";
const char* EndpointMapper::OTA_UPDATE_PATH = "/api/ota/update";

const Endpoint endpoints[] = {
    Endpoint(Endpoint::WIFI_CONFIG, Endpoint::Verb::POST, EndpointMapper::WIFI_CONFIG_PATH, handleWiFiConfig),
    Endpoint(Endpoint::SYSTEM_INFO, Endpoint::Verb::GET, EndpointMapper::SYSTEM_INFO_PATH, handleSystemInfo),
    Endpoint(Endpoint::WIFI_RESET, Endpoint::Verb::DELETE, EndpointMapper::WIFI_RESET_PATH, handleWiFiReset),
    Endpoint(Endpoint::CRYPTO_INFO, Endpoint::Verb::GET, EndpointMapper::CRYPTO_INFO_PATH, handleCryptoInfo),
    Endpoint(Endpoint::NAME_INFO, Endpoint::Verb::GET, EndpointMapper::NAME_INFO_PATH, handleNameInfo),
    Endpoint(Endpoint::WIFI_STATUS, Endpoint::Verb::GET, EndpointMapper::WIFI_STATUS_PATH, handleWiFiStatus),
    Endpoint(Endpoint::WIFI_SCAN, Endpoint::Verb::GET, EndpointMapper::WIFI_SCAN_PATH, handleWiFiScan),
    Endpoint(Endpoint::BLE_STOP, Endpoint::Verb::POST, EndpointMapper::BLE_STOP_PATH, nullptr), // Special case handled in route
    Endpoint(Endpoint::CRYPTO_SIGN, Endpoint::Verb::POST, EndpointMapper::CRYPTO_SIGN_PATH, handleCryptoSign),
    Endpoint(Endpoint::OTA_UPDATE, Endpoint::Verb::POST, EndpointMapper::OTA_UPDATE_PATH, OTAHandler::handleOTAUpdate)
};

EndpointMapper::Iterator EndpointMapper::begin() const { return EndpointMapper::Iterator(endpoints); }
EndpointMapper::Iterator EndpointMapper::end() const { return EndpointMapper::Iterator(endpoints + sizeof(endpoints) / sizeof(endpoints[0])); }

const Endpoint unknownEndpoint = Endpoint(Endpoint::UNKNOWN, Endpoint::Verb::UNKNOWN, "", nullptr);

const Endpoint& EndpointMapper::toEndpoint(const String& path, const String& verb) {
    // Create an instance of EndpointMapper to use the non-static begin() and end() methods
    Endpoint::Verb eVerb = stringToVerb(verb);
    
    for (const Endpoint& endpoint : endpointMapper) {
    // Use endpoint here
        if (path == endpoint.path && endpoint.verb == eVerb) return endpoint;
    }
    return unknownEndpoint;
}

Endpoint::Verb EndpointMapper::stringToVerb(const String& verb) {
    if (verb == "GET") return Endpoint::Verb::GET;
    if (verb == "POST") return Endpoint::Verb::POST;
    if (verb == "DELETE") return Endpoint::Verb::DELETE;
    return Endpoint::Verb::UNKNOWN;
}

String EndpointMapper::verbToString(Endpoint::Verb verb) {
    switch (verb) {
        case Endpoint::Verb::GET: return "GET";
        case Endpoint::Verb::POST: return "POST";
        case Endpoint::Verb::DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}

EndpointResponse EndpointMapper::route(const EndpointRequest& request) {
    // Special case for BLE_STOP which doesn't have a direct handler
    if (request.endpoint.type == Endpoint::Type::BLE_STOP) {
        EndpointResponse response;
        response.statusCode = 404;
        response.contentType = "application/json";
        response.data = "{\"status\":\"error\",\"message\":\"Endpoint not found\"}";
        
        #if defined(USE_BLE_SETUP)
            extern unsigned long bleShutdownTime;
            // Schedule BLE shutdown in 10 seconds
            bleShutdownTime = millis() + 10000;
            response.statusCode = 200;
            response.data = "{\"status\":\"success\",\"message\":\"BLE shutdown scheduled\"}";
        #else
            response.statusCode = 400;
            response.data = "{\"status\":\"error\",\"message\":\"BLE not enabled\"}";
        #endif
        
        return response;
    }
    
    // If the endpoint has a handler function, call it directly
    if (request.endpoint.handler != nullptr) {
        return request.endpoint.handler(request);
    }
    
    // Default error response for unknown endpoints
    EndpointResponse response;
    response.statusCode = 404;
    response.contentType = "application/json";
    response.data = "{\"status\":\"error\",\"message\":\"Endpoint not found\"}";
    return response;
}

void EndpointMapper::printPaths() {
    Serial.println("Registered paths:");
    Serial.println(WIFI_CONFIG_PATH);
    Serial.println(SYSTEM_INFO_PATH);
    Serial.println(WIFI_RESET_PATH);
    Serial.println(CRYPTO_INFO_PATH);
    Serial.println(NAME_INFO_PATH);
    Serial.println(WIFI_STATUS_PATH);
    Serial.println(WIFI_SCAN_PATH);
    Serial.println(BLE_STOP_PATH);
    Serial.println(CRYPTO_SIGN_PATH);
}

// Define the global instance
EndpointMapper endpointMapper;


