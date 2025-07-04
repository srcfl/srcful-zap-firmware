#include "endpoint_mapper.h"
#include "endpoint_types.h"
// #include "../ble_handler.h"
// #include "ota/ota_handler.h"
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

const char* EndpointMapper::P1_DATA_PATH = "/api/data/p1/obis";

// Global instance of OTA handler
// TODO: The endpoint should be passed to the OTA handler

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
    Endpoint(Endpoint::DEBUG, Endpoint::Verb::GET, EndpointMapper::DEBUG_PATH, g_debugHandler), 
    Endpoint(Endpoint::BLE_STOP, Endpoint::Verb::POST, EndpointMapper::BLE_STOP_PATH, g_bleStopHandler), 
    Endpoint(Endpoint::CRYPTO_SIGN, Endpoint::Verb::POST, EndpointMapper::CRYPTO_SIGN_PATH, g_cryptoSignHandler),
    Endpoint(Endpoint::ECHO, Endpoint::Verb::POST, EndpointMapper::ECHO_PATH, g_echoHandler), 

    Endpoint(Endpoint::OTA_UPDATE, Endpoint::Verb::POST, EndpointMapper::OTA_UPDATE_PATH, g_otaUpdateHandler),
    Endpoint(Endpoint::OTA_STATUS, Endpoint::Verb::GET, EndpointMapper::OTA_STATUS_PATH, g_otaStatusHandler),

    Endpoint(Endpoint::P1_DATA, Endpoint::Verb::GET, EndpointMapper::P1_DATA_PATH, g_dataReaderGetHandler)
};

EndpointMapper::Iterator EndpointMapper::begin() const { return EndpointMapper::Iterator(endpoints); }
EndpointMapper::Iterator EndpointMapper::end() const { return EndpointMapper::Iterator(endpoints + sizeof(endpoints) / sizeof(endpoints[0])); }

const Endpoint& EndpointMapper::toEndpoint(const zap::Str& path, const zap::Str& verb) {
    // Create an instance of EndpointMapper to use the non-static begin() and end() methods
    Endpoint::Verb eVerb = stringToVerb(verb);
    static const Endpoint unknownEndpoint = Endpoint(Endpoint::UNKNOWN, Endpoint::Verb::UNKNOWN, "", g_nullHandler);
    
    for (const Endpoint& endpoint : endpointMapper) {
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
        case Endpoint::Verb::GET: return zap::Str("GET");
        case Endpoint::Verb::POST: return zap::Str("POST");
        case Endpoint::Verb::DELETE: return zap::Str("DELETE");
        default: return zap::Str("UNKNOWN");
    }
}

EndpointResponse EndpointMapper::route(const EndpointRequest& request) {
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

// Define the global instance
EndpointMapper endpointMapper;