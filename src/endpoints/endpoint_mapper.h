#pragma once

#include <Arduino.h>
#include "endpoint_types.h"
#include "../endpoints.h"
#include <array>

class EndpointMapper {
public:
    // Define path constants
    static const char* WIFI_CONFIG_PATH;
    static const char* SYSTEM_INFO_PATH;
    static const char* WIFI_RESET_PATH;
    static const char* CRYPTO_INFO_PATH;
    static const char* NAME_INFO_PATH;
    static const char* WIFI_STATUS_PATH;
    static const char* WIFI_SCAN_PATH;
    static const char* BLE_STOP_PATH;
    static const char* CRYPTO_SIGN_PATH;
    static const char* OTA_UPDATE_PATH;
    static const char* INITIALIZE_FORM_PATH;
    static const char* INITIALIZE_PATH;
    
    // Iterator support
    class Iterator {
    public:
        Iterator(const Endpoint* ptr) : ptr_(ptr) {}
        
        // Dereference operator
        const Endpoint& operator*() const { return *ptr_; }
        
        // Arrow operator
        const Endpoint* operator->() const { return ptr_; }
        
        // Prefix increment
        Iterator& operator++() { ++ptr_; return *this; }
        
        // Postfix increment
        Iterator operator++(int) { Iterator tmp = *this; ++ptr_; return tmp; }
        
        // Equality comparison
        bool operator==(const Iterator& other) const { return ptr_ == other.ptr_; }
        
        // Inequality comparison
        bool operator!=(const Iterator& other) const { return ptr_ != other.ptr_; }
        
    private:
        const Endpoint* ptr_;
    };
    
    // Begin and end methods for iteration - these need to be non-static for range-based for loops
    Iterator begin() const;
    Iterator end() const;
    
    // Mapping functions
    static const Endpoint& toEndpoint(const String& path, const String& verb);
    static Endpoint::Verb stringToVerb(const String& method);
    static String verbToString(Endpoint::Verb method);
    static EndpointResponse route(const EndpointRequest& request);
    static void printPaths();
};

// Global instance of EndpointMapper
extern EndpointMapper endpointMapper; 