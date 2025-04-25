#pragma once
#include "../zap_str.h"

#include <ctype.h>
#include <vector>
#include <functional>

class JsonBuilder {
private:
    zap::Str buffer;
    bool firstItem;
    bool inObject;
    std::vector<bool> objectStack; // Stack to track firstItem state for nested objects
    
public:
    JsonBuilder() : firstItem(true), inObject(false) {}
    
    // Start a new object
    JsonBuilder& beginObject();
    
    // Start a nested object with a key
    JsonBuilder& beginObject(const char* key);
    
    // End the current object
    JsonBuilder& endObject();
    
    // Add a key-value pair
    JsonBuilder& add(const char* key, const char* value);
    JsonBuilder& add(const char* key, int value);
    JsonBuilder& add(const char* key, uint32_t value);
    JsonBuilder& add(const char* key, uint64_t value);
    JsonBuilder& add(const char* key, bool value);
    JsonBuilder& add(const char* key, uint8_t* data, size_t size);
    
    // Add an array of string values
    JsonBuilder& addArray(const char* key, const std::vector<zap::Str>& values);
    
    // Add an array of C-style strings
    JsonBuilder& addArray(const char* key, const char* const* values, size_t count);
    
    // End all objects and get the result
    zap::Str end();
    
    // Clear the buffer and reset state
    void clear();
};

class JsonParser {
private:
    const char* data;     // Original JSON buffer (not owned)
    size_t dataLen;       // Total length of the original buffer
    size_t startPos;      // Start position of our "view" into the buffer
    size_t endPos;        // End position of our "view"
    
    // Helper to get absolute position in buffer
    size_t absPos(size_t relPos) const;
    
    // Skip whitespace
    size_t skipWhitespace(size_t pos) const;
    
    // Find a key in the current object
    // Returns the position of the value after the key, or 0 if not found
    size_t findKey(const char* key, size_t pos = 0) const;
    
    // Skip any JSON value (object, array, string, number, boolean, null)
    size_t skipValue(size_t pos) const;
    
    // Get string value at current position
    bool getStringValue(size_t pos, char* value, size_t maxLen, size_t& endPos) const;
    bool getStringValue(size_t pos, zap::Str& value, size_t& endPos) const;
    
    // Get integer value at current position
    bool getIntValue(size_t pos, int& value, size_t& endPos) const;
    
    // Get boolean value at current position
    bool getBoolValue(size_t pos, bool& value, size_t& endPos) const;

    // Get object value at current position
    bool getObjectValue(size_t pos, JsonParser& result, size_t& endPos) const;
    
public:
    // Constructor for full buffer
    JsonParser(const char* jsonData) 
        : data(jsonData), 
          dataLen(strlen(jsonData)), 
          startPos(0), 
          endPos(strlen(jsonData)) {}
    
    // Constructor for a view into a buffer
    JsonParser(const char* jsonData, size_t length, size_t start, size_t end) 
        : data(jsonData), 
          dataLen(length), 
          startPos(start), 
          endPos(end) {}
    
    // Get an object by key as a new view
    bool getObject(const char* key, JsonParser& result);
    
    // Get a string value by key
    bool getString(const char* key, char* value, size_t maxLen);
    bool getString(const char* key, zap::Str& value);
    
    // Get an integer value by key
    bool getInt(const char* key, int& value);
    
    // Get a boolean value by key
    bool getBool(const char* key, bool& value);
    
    // Get a value using dot notation path
    bool getValueByPath(const char* path, std::function<bool(const JsonParser&, size_t, size_t&)> valueExtractor);
    
    // Path-based value getters
    bool getObjectByPath(const char* path, JsonParser& result);
    bool getStringByPath(const char* path, char* value, size_t maxLen);
    bool getStringByPath(const char* path, zap::Str& value);
    bool getIntByPath(const char* path, int& value);
    bool getBoolByPath(const char* path, bool& value);
    
    // Check if a field at the specified path is null
    bool isFieldNullByPath(const char* path);
    
    // Check if a key exists in the current object
    bool contains(const char* key);
    
    // Get a string value by key or return empty string if key doesn't exist
    zap::Str getStringOrEmpty(const char* key);
    
    // Get a uint64_t value by key
    bool getUInt64(const char* key, uint64_t& value);
    
    // Simplified getter that returns the value directly
    uint64_t getUInt64(const char* key);
};