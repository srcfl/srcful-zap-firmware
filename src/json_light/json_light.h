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
    size_t pos;           // Current parsing position (relative to startPos)
    
    // Helper to get absolute position in buffer
    size_t absPos() const;
    
    // Skip whitespace
    void skipWhitespace();
    
    // Find a key in the current object
    bool findKey(const char* key);
    
    // Skip any JSON value (object, array, string, number, boolean, null)
    void skipValue();
    
    // Get string value at current position
    bool getStringValue(char* value, size_t maxLen);
    bool getStringValue(zap::Str& value);
    
    // Get integer value at current position
    bool getIntValue(int& value);
    
    // Get boolean value at current position
    bool getBoolValue(bool& value);
    
public:
    // Constructor for full buffer
    JsonParser(const char* jsonData) 
        : data(jsonData), 
          dataLen(strlen(jsonData)), 
          startPos(0), 
          endPos(strlen(jsonData)), 
          pos(0) {}
    
    // Constructor for a view into a buffer
    JsonParser(const char* jsonData, size_t length, size_t start, size_t end) 
        : data(jsonData), 
          dataLen(length), 
          startPos(start), 
          endPos(end), 
          pos(0) {}
    
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
    bool getValueByPath(const char* path, const std::function<bool()>& valueExtractor);
    
    // Path-based value getters
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
    
    // Reset parser position (within current view)
    void reset();
};