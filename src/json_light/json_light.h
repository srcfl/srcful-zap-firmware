#pragma once

#include <Arduino.h>
#include <vector>
class JsonBuilder {
private:
    String buffer;
    bool firstItem;
    bool inObject;
    
public:
    JsonBuilder() : firstItem(true), inObject(false) {}
    
    // Start a new object
    JsonBuilder& beginObject() {
        buffer += '{';
        firstItem = true;
        inObject = true;
        return *this;
    }
    
    // Add a key-value pair
    JsonBuilder& add(const char* key, const char* value) {
        if (!firstItem) buffer += ',';
        buffer += '"';
        buffer += key;
        buffer += "\":\"";
        buffer += value;
        buffer += '"';
        firstItem = false;
        return *this;
    }
    
    JsonBuilder& add(const char* key, int value) {
        if (!firstItem) buffer += ',';
        buffer += '"';
        buffer += key;
        buffer += "\":";
        buffer += value;
        firstItem = false;
        return *this;
    }

    JsonBuilder& add(const char* key, uint32_t value) {
        if (!firstItem) buffer += ',';
        buffer += '"';
        buffer += key;
        buffer += "\":";
        buffer += value;
        firstItem = false;
        return *this;
    }


    JsonBuilder& add(const char* key, bool value) {
        if (!firstItem) buffer += ',';
        buffer += '"';
        buffer += key;
        buffer += "\":";
        buffer += value ? "true" : "false";
        firstItem = false;
        return *this;
    }
    
    // Add an array of string values
    JsonBuilder& addArray(const char* key, const std::vector<String>& values) {
        if (!firstItem) buffer += ',';
        buffer += '"';
        buffer += key;
        buffer += "\":[\"";
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) buffer += "\",\"";
            buffer += values[i];
        }
        buffer += "\"]";
        firstItem = false;
        return *this;
    }
    
    // Add an array of C-style strings
    JsonBuilder& addArray(const char* key, const char* const* values, size_t count) {
        if (!firstItem) buffer += ',';
        buffer += '"';
        buffer += key;
        buffer += "\":[\"";
        for (size_t i = 0; i < count; i++) {
            if (i > 0) buffer += "\",\"";
            buffer += values[i];
        }
        buffer += "\"]";
        firstItem = false;
        return *this;
    }
    
    // End the object and get the result
    String end() {
        if (inObject) {
            buffer += '}';
            inObject = false;
        }
        return buffer;
    }
    
    // Clear the buffer
    void clear() {
        buffer = "";
        firstItem = true;
        inObject = false;
    }
};

class JsonParser {
private:
    const char* data;
    size_t pos;
    size_t len;
    
    void skipWhitespace() {
        while (pos < len && isspace(data[pos])) pos++;
    }
    
    // Find the next key in the JSON
    bool findKey(const char* key) {
        // Reset to start of JSON
        pos = 0;
        
        // Skip opening brace
        skipWhitespace();
        if (pos >= len || data[pos] != '{') return false;
        pos++;
        
        // Search for the key
        while (pos < len) {
            skipWhitespace();
            
            // Check for end of object
            if (data[pos] == '}') return false;
            
            // Check for comma (not first item)
            if (pos > 1 && data[pos] == ',') pos++;
            
            // Skip whitespace after comma
            skipWhitespace();
            
            // Check for opening quote
            if (data[pos] != '"') return false;
            pos++;
            
            // Check if this is our key
            size_t keyLen = strlen(key);
            if (pos + keyLen > len) return false;
            
            if (strncmp(data + pos, key, keyLen) == 0) {
                pos += keyLen;
                
                // Check for closing quote
                if (data[pos] != '"') return false;
                pos++;
                
                // Skip whitespace and colon
                skipWhitespace();
                if (data[pos] != ':') return false;
                pos++;
                
                // Skip whitespace after colon
                skipWhitespace();
                
                return true;
            }
            
            // Skip this key and its value
            while (pos < len && data[pos] != '"') pos++;
            if (pos < len) pos++; // Skip closing quote
            
            // Skip colon
            skipWhitespace();
            if (data[pos] != ':') return false;
            pos++;
            
            // Skip value
            skipWhitespace();
            if (data[pos] == '"') {
                // String value
                pos++;
                while (pos < len && data[pos] != '"') pos++;
                if (pos < len) pos++; // Skip closing quote
            } else if (data[pos] == '{') {
                // Object value - skip until matching closing brace
                int braceCount = 1;
                pos++;
                while (pos < len && braceCount > 0) {
                    if (data[pos] == '{') braceCount++;
                    else if (data[pos] == '}') braceCount--;
                    pos++;
                }
            } else if (data[pos] == '[') {
                // Array value - skip until matching closing bracket
                int bracketCount = 1;
                pos++;
                while (pos < len && bracketCount > 0) {
                    if (data[pos] == '[') bracketCount++;
                    else if (data[pos] == ']') bracketCount--;
                    pos++;
                }
            } else {
                // Simple value (number, boolean, null)
                while (pos < len && data[pos] != ',' && data[pos] != '}') pos++;
            }
        }
        
        return false;
    }
    
    // Get a string value
    bool getStringValue(char* value, size_t maxLen) {
        if (data[pos] != '"') return false;
        pos++;
        
        size_t i = 0;
        while (pos < len && data[pos] != '"' && i < maxLen - 1) {
            value[i++] = data[pos++];
        }
        value[i] = '\0';
        
        if (pos < len && data[pos] == '"') pos++;
        
        return true;
    }
    
    // Get an integer value
    bool getIntValue(int& value) {
        char* endPtr;
        value = strtol(data + pos, &endPtr, 10);
        pos += (endPtr - (data + pos));
        return true;
    }
    
    // Get a boolean value
    bool getBoolValue(bool& value) {
        if (pos + 4 <= len && strncmp(data + pos, "true", 4) == 0) {
            value = true;
            pos += 4;
            return true;
        } else if (pos + 5 <= len && strncmp(data + pos, "false", 5) == 0) {
            value = false;
            pos += 5;
            return true;
        }
        return false;
    }
    
public:
    JsonParser(const char* json) : data(json), pos(0), len(strlen(json)) {}
    
    // Find a string value by key
    bool getString(const char* key, char* value, size_t maxLen) {
        if (!findKey(key)) return false;
        return getStringValue(value, maxLen);
    }
    
    // Find an integer value by key
    bool getInt(const char* key, int& value) {
        if (!findKey(key)) return false;
        return getIntValue(value);
    }
    
    // Find a boolean value by key
    bool getBool(const char* key, bool& value) {
        if (!findKey(key)) return false;
        return getBoolValue(value);
    }
    
    // Reset parser to start
    void reset() {
        pos = 0;
    }
}; 