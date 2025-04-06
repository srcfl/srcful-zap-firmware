#pragma once


// Forward declarations
struct EndpointResponse;
struct EndpointRequest;

// Define HTTP methods as an enum


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
        UNKNOWN
    };
    const Type type;
    const Verb verb;
    const char* path;
    // Function pointer to the handler for this endpoint
    EndpointResponse (*handler)(const EndpointRequest&);
    
    Endpoint(Type type, Verb verb, const char* path, EndpointResponse (*handler)(const EndpointRequest&)) 
        : type(type), verb(verb), path(path), handler(handler) {}

}; 