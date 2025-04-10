#pragma once

#include <Arduino.h>

// Forward declarations
struct EndpointResponse;
struct EndpointRequest;

class EndpointFunction {
    public:
        virtual EndpointResponse handle(const String& contents) = 0;
};

// Define all possible endpoints as an enum
class Endpoint {
    public:

    enum class Verb {
        GET = 0,
        POST = 1,
        DELETE = 2,
        UNKNOWN = 3
    };

    enum Type {
        WIFI_CONFIG,
        SYSTEM_INFO,
        WIFI_RESET,
        CRYPTO_INFO,
        NAME_INFO,
        WIFI_STATUS,
        WIFI_SCAN,
        BLE_STOP,
        CRYPTO_SIGN,
        OTA_UPDATE,
        OTA_STATUS,
        UNKNOWN
    };
    const Type type;
    const Verb verb;
    const char* path;
    // Function pointer to the handler for this endpoint
    EndpointFunction &handler;
    
    Endpoint(Type type, Verb verb, const char* path, EndpointFunction &handler) 
        : type(type), verb(verb), path(path), handler(handler) {}
};

// Response structure that can be used by both BLE and HTTP handlers
struct EndpointResponse {
    int statusCode;      // HTTP-style status code (200, 400, etc.)
    String contentType;  // Content type of the response
    String data;        // Response data
};

// Request structure that normalizes input from both BLE and HTTP
struct EndpointRequest {
    EndpointRequest(const Endpoint& endpoint) : endpoint(endpoint) {}
    const Endpoint& endpoint;
    String content;
    int offset;
};