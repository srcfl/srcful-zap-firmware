#include "endpoint_mapper.h"
#include "endpoint_types.h"
#include "ble_handler.h"
#include "ota_handler.h"

// Define path constants
const char* EndpointMapper::WIFI_CONFIG_PATH = "/api/wifi";
const char* EndpointMapper::SYSTEM_INFO_PATH = "/api/system/info";
const char* EndpointMapper::WIFI_RESET_PATH = "/api/wifi/reset";
const char* EndpointMapper::CRYPTO_INFO_PATH = "/api/crypto";
const char* EndpointMapper::NAME_INFO_PATH = "/api/name";
const char* EndpointMapper::WIFI_STATUS_PATH = "/api/wifi";
const char* EndpointMapper::WIFI_SCAN_PATH = "/api/wifi/scan";
const char* EndpointMapper::BLE_STOP_PATH = "/api/ble/stop";
const char* EndpointMapper::CRYPTO_SIGN_PATH = "/api/crypto/sign";

Endpoint EndpointMapper::pathToEndpoint(const String& path) {
    if (path == WIFI_CONFIG_PATH) return Endpoint::WIFI_CONFIG;
    if (path == SYSTEM_INFO_PATH) return Endpoint::SYSTEM_INFO;
    if (path == WIFI_RESET_PATH) return Endpoint::WIFI_RESET;
    if (path == CRYPTO_INFO_PATH) return Endpoint::CRYPTO_INFO;
    if (path == NAME_INFO_PATH) return Endpoint::NAME_INFO;
    if (path == WIFI_STATUS_PATH) return Endpoint::WIFI_STATUS;
    if (path == WIFI_SCAN_PATH) return Endpoint::WIFI_SCAN;
    if (path == BLE_STOP_PATH) return Endpoint::BLE_STOP;
    if (path == CRYPTO_SIGN_PATH) return Endpoint::CRYPTO_SIGN;
    return Endpoint::UNKNOWN;
}

String EndpointMapper::endpointToPath(Endpoint endpoint) {
    switch (endpoint) {
        case Endpoint::WIFI_CONFIG: return WIFI_CONFIG_PATH;
        case Endpoint::SYSTEM_INFO: return SYSTEM_INFO_PATH;
        case Endpoint::WIFI_RESET: return WIFI_RESET_PATH;
        case Endpoint::CRYPTO_INFO: return CRYPTO_INFO_PATH;
        case Endpoint::NAME_INFO: return NAME_INFO_PATH;
        case Endpoint::WIFI_STATUS: return WIFI_STATUS_PATH;
        case Endpoint::WIFI_SCAN: return WIFI_SCAN_PATH;
        case Endpoint::BLE_STOP: return BLE_STOP_PATH;
        case Endpoint::CRYPTO_SIGN: return CRYPTO_SIGN_PATH;
        default: return "";
    }
}

HttpMethod EndpointMapper::stringToMethod(const String& method) {
    if (method == "GET") return HttpMethod::GET;
    if (method == "POST") return HttpMethod::POST;
    return HttpMethod::UNKNOWN;
}

String EndpointMapper::methodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        default: return "UNKNOWN";
    }
}

EndpointResponse EndpointMapper::route(const EndpointRequest& request) {
    EndpointResponse response;
    response.statusCode = 404;
    response.contentType = "application/json";
    response.data = "{\"status\":\"error\",\"message\":\"Endpoint not found\"}";

    // Route based on endpoint and method
    switch (request.endpoint) {
        case Endpoint::WIFI_CONFIG:
            if (request.method == HttpMethod::POST) {
                return handleWiFiConfig(request);
            } else if (request.method == HttpMethod::GET) {
                return handleWiFiStatus(request);
            }
            break;
            
        case Endpoint::SYSTEM_INFO:
            if (request.method == HttpMethod::GET) {
                return handleSystemInfo(request);
            }
            break;
            
        case Endpoint::WIFI_RESET:
            if (request.method == HttpMethod::POST) {
                return handleWiFiReset(request);
            }
            break;
            
        case Endpoint::CRYPTO_INFO:
            if (request.method == HttpMethod::GET) {
                return handleCryptoInfo(request);
            }
            break;
            
        case Endpoint::NAME_INFO:
            if (request.method == HttpMethod::GET) {
                return handleNameInfo(request);
            }
            break;
            
        case Endpoint::WIFI_STATUS:
            if (request.method == HttpMethod::GET) {
                return handleWiFiStatus(request);
            }
            break;
            
        case Endpoint::WIFI_SCAN:
            if (request.method == HttpMethod::GET) {
                return handleWiFiScan(request);
            }
            break;
            
        case Endpoint::BLE_STOP:
            if (request.method == HttpMethod::POST) {
                #if defined(USE_BLE_SETUP)
                    extern unsigned long bleShutdownTime;
                    // Schedule BLE shutdown in 10 seconds
                    bleShutdownTime = millis() + 10000;
                    response.statusCode = 200;
                    response.data = "{\"status\":\"success\",\"message\":\"BLE shutdown scheduled\"}";
                    return response;
                #else
                    response.statusCode = 400;
                    response.data = "{\"status\":\"error\",\"message\":\"BLE not enabled\"}";
                    return response;
                #endif
            }
            break;
            
        case Endpoint::CRYPTO_SIGN:
            if (request.method == HttpMethod::POST) {
                return handleCryptoSign(request);
            }
            break;
            
        case Endpoint::OTA_UPDATE:
            if (request.method == HttpMethod::POST) {
                return OTAHandler::handleOTAUpdate(request);
            }
            break;
            
        default:
            break;
    }

    response.statusCode = 405;
    response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
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