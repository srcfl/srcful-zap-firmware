#include "endpoint_mapper.h"
#include "endpoint_types.h"
// #include "../ble_handler.h"
#include "../ota_handler.h"
#include "endpoint_handlers.h"

// Define path constants
const char* EndpointMapper::WIFI_CONFIG_PATH = "/api/wifi";
const char* EndpointMapper::SYSTEM_INFO_PATH = "/api/system";
const char* EndpointMapper::SYSTEM_REBOOT_PATH = "/api/system/reboot";
const char* EndpointMapper::WIFI_RESET_PATH = "/api/wifi";
const char* EndpointMapper::CRYPTO_INFO_PATH = "/api/crypto";
const char* EndpointMapper::NAME_INFO_PATH = "/api/name";
const char* EndpointMapper::WIFI_STATUS_PATH = "/api/wifi";
const char* EndpointMapper::WIFI_SCAN_PATH = "/api/wifi/scan";
const char* EndpointMapper::BLE_STOP_PATH = "/api/ble/stop";
const char* EndpointMapper::CRYPTO_SIGN_PATH = "/api/crypto/sign";
const char* EndpointMapper::OTA_UPDATE_PATH = "/api/ota/update";
const char* EndpointMapper::OTA_STATUS_PATH = "/api/ota/status";
const char* EndpointMapper::DEBUG_PATH = "/api/debug";
const char* EndpointMapper::ECHO_PATH = "/api/echo";

// Global instance of OTA handler
// TODO: The endpoint should be passed to the OTA handler
OTAHandler g_otaHandler;

NameInfoHandler g_nullHandler;

const Endpoint endpoints[] = {
    Endpoint(Endpoint::WIFI_CONFIG, Endpoint::Verb::POST, EndpointMapper::WIFI_CONFIG_PATH, g_wifiConfigHandler),
    Endpoint(Endpoint::SYSTEM_INFO, Endpoint::Verb::GET, EndpointMapper::SYSTEM_INFO_PATH, g_systemInfoHandler),
    Endpoint(Endpoint::SYSTEM_INFO, Endpoint::Verb::POST, EndpointMapper::SYSTEM_REBOOT_PATH, g_systemRebootHandler),
    Endpoint(Endpoint::WIFI_RESET, Endpoint::Verb::DELETE, EndpointMapper::WIFI_RESET_PATH, g_wifiResetHandler),
    Endpoint(Endpoint::CRYPTO_INFO, Endpoint::Verb::GET, EndpointMapper::CRYPTO_INFO_PATH, g_cryptoInfoHandler),
    Endpoint(Endpoint::NAME_INFO, Endpoint::Verb::GET, EndpointMapper::NAME_INFO_PATH, g_nameInfoHandler),
    Endpoint(Endpoint::WIFI_STATUS, Endpoint::Verb::GET, EndpointMapper::WIFI_STATUS_PATH, g_wifiStatusHandler),
    Endpoint(Endpoint::WIFI_SCAN, Endpoint::Verb::GET, EndpointMapper::WIFI_SCAN_PATH, g_wifiScanHandler),
    Endpoint(Endpoint::DEBUG, Endpoint::Verb::GET, EndpointMapper::DEBUG_PATH, g_debugHandler), // Special case handled in route
#if defined(USE_BLE_SETUP)
    Endpoint(Endpoint::BLE_STOP, Endpoint::Verb::POST, EndpointMapper::BLE_STOP_PATH, g_bleStopHandler), // Special case handled in route
#endif
    Endpoint(Endpoint::CRYPTO_SIGN, Endpoint::Verb::POST, EndpointMapper::CRYPTO_SIGN_PATH, g_cryptoSignHandler),
    Endpoint(Endpoint::ECHO, Endpoint::Verb::POST, EndpointMapper::ECHO_PATH, g_echoHandler), // New Echo endpoint
    Endpoint(Endpoint::OTA_UPDATE, Endpoint::Verb::POST, EndpointMapper::OTA_UPDATE_PATH, g_nullHandler),   // Will be handled by route()
    Endpoint(Endpoint::OTA_STATUS, Endpoint::Verb::GET, EndpointMapper::OTA_STATUS_PATH, g_nullHandler)     // Will be handled by route()
};

EndpointMapper::Iterator EndpointMapper::begin() const { return EndpointMapper::Iterator(endpoints); }
EndpointMapper::Iterator EndpointMapper::end() const { return EndpointMapper::Iterator(endpoints + sizeof(endpoints) / sizeof(endpoints[0])); }

const Endpoint& EndpointMapper::toEndpoint(const zap::Str& path, const zap::Str& verb) {
    // Create an instance of EndpointMapper to use the non-static begin() and end() methods
    Endpoint::Verb eVerb = stringToVerb(verb);
    static const Endpoint unknownEndpoint = Endpoint(Endpoint::UNKNOWN, Endpoint::Verb::UNKNOWN, "", g_nullHandler);
    
    for (const Endpoint& endpoint : endpointMapper) {
    // Use endpoint here
        if (path == endpoint.path && endpoint.verb == eVerb) return endpoint;
    }
    return unknownEndpoint;
}

Endpoint::Verb EndpointMapper::stringToVerb(const zap::Str& verb) {
    if (verb == "GET") return Endpoint::Verb::GET;
    if (verb == "POST") return Endpoint::Verb::POST;
    if (verb == "DELETE") return Endpoint::Verb::DELETE;
    return Endpoint::Verb::UNKNOWN;
}

zap::Str EndpointMapper::verbToString(Endpoint::Verb verb) {
    switch (verb) {
        case Endpoint::Verb::GET: return "GET";
        case Endpoint::Verb::POST: return "POST";
        case Endpoint::Verb::DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}

EndpointResponse EndpointMapper::route(const EndpointRequest& request) {
    // Handle special cases for endpoints without a dedicated handler class
    if (request.endpoint.type == Endpoint::OTA_UPDATE) {
        return g_otaHandler.handleOTAUpdate(request);
    } else if (request.endpoint.type == Endpoint::OTA_STATUS) {
        return g_otaHandler.handleOTAStatus(request);
    }
    
    // If the endpoint has a handler function, call it directly
    if (&request.endpoint.handler != &g_nullHandler) {
        return request.endpoint.handler.handle(request.content);
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
    Serial.println(OTA_UPDATE_PATH);
    Serial.println(OTA_STATUS_PATH);
    Serial.println(DEBUG_PATH);
    Serial.println(ECHO_PATH);
}

// Define the global instance
EndpointMapper endpointMapper;