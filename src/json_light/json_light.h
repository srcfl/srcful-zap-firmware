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
    JsonBuilder& beginObject() {
        buffer += '{';
        objectStack.push_back(firstItem); // Save current state
        firstItem = true;
        inObject = true;
        return *this;
    }
    
    // Start a nested object with a key
    JsonBuilder& beginObject(const char* key) {
        if (!firstItem) buffer += ',';
        buffer += '"';
        buffer += key;
        buffer += "\":{";
        objectStack.push_back(firstItem); // Save current state
        firstItem = true;
        inObject = true;
        return *this;
    }
    
    // End the current object
    JsonBuilder& endObject() {
        buffer += '}';
        if (!objectStack.empty()) {
            firstItem = objectStack.back(); // Restore previous state
            objectStack.pop_back();
            firstItem = false; // After closing an object, we've added an item to the parent
        }
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

    JsonBuilder& add(const char* key, uint64_t value) {
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

    JsonBuilder& add(const char* key, uint8_t* data, size_t size) {
        if (!firstItem) buffer += ',';
        
        // add data as hex string

        buffer += '"';
        buffer += key;
        buffer += "\":\"";
        for (size_t i = 0; i < size; i++) {
            buffer += zap::Str(data[i], 16);
        }
        buffer += '"';
        firstItem = false;
        return *this;
    }
    
    // Add an array of string values
    JsonBuilder& addArray(const char* key, const std::vector<zap::Str>& values) {
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
    
    // End all objects and get the result
    zap::Str end() {
        while (inObject && !objectStack.empty()) {
            buffer += '}';
            objectStack.pop_back();
        }
        inObject = false;
        return buffer;
    }
    
    // Clear the buffer and reset state
    void clear() {
        buffer.clear();
        firstItem = true;
        inObject = false;
        objectStack.clear();
    }
};

class JsonParser {
    private:
        const char* data;     // Original JSON buffer (not owned)
        size_t dataLen;       // Total length of the original buffer
        size_t startPos;      // Start position of our "view" into the buffer
        size_t endPos;        // End position of our "view"
        size_t pos;           // Current parsing position (relative to startPos)
        
        // Helper to get absolute position in buffer
        size_t absPos() const {
            return startPos + pos;
        }
        
        // Skip whitespace
        void skipWhitespace() {
            while (pos < (endPos - startPos) && isspace(data[startPos + pos])) {
                pos++;
            }
        }
        
        // Find a key in the current object
        bool findKey(const char* key) {
            size_t savedPos = pos;
            skipWhitespace();
            
            // If at the start of the view, expect an opening brace
            if (pos == 0) {
                if (absPos() >= dataLen || data[absPos()] != '{') {
                    pos = savedPos;
                    return false;
                }
                pos++;
                skipWhitespace();
            }
            
            while (absPos() < dataLen && absPos() < endPos) {
                // Check for end of object
                if (data[absPos()] == '}') {
                    pos = savedPos;
                    return false;
                }
                
                // Check for comma
                if (data[absPos()] == ',') {
                    pos++;
                    skipWhitespace();
                }
                
                // Expect a quoted key
                if (data[absPos()] != '"') {
                    pos = savedPos;
                    return false;
                }
                pos++;
                
                // Compare with the requested key
                size_t keyLen = strlen(key);
                if (absPos() + keyLen <= dataLen && 
                    strncmp(data + absPos(), key, keyLen) == 0 && 
                    absPos() + keyLen < dataLen && data[absPos() + keyLen] == '"') {
                    
                    // Found the key
                    pos += keyLen + 1;  // Skip key and closing quote
                    skipWhitespace();
                    
                    // Expect a colon
                    if (absPos() >= dataLen || data[absPos()] != ':') {
                        pos = savedPos;
                        return false;
                    }
                    pos++;
                    skipWhitespace();
                    
                    return true;
                }
                
                // Not the key we want, skip to the end of this key
                while (absPos() < dataLen && data[absPos()] != '"') {
                    pos++;
                }
                if (absPos() >= dataLen) {
                    pos = savedPos;
                    return false;
                }
                pos++;  // Skip closing quote
                
                skipWhitespace();
                
                // Expect a colon
                if (absPos() >= dataLen || data[absPos()] != ':') {
                    pos = savedPos;
                    return false;
                }
                pos++;
                skipWhitespace();
                
                // Skip this value
                skipValue();
                skipWhitespace();
            }
            
            pos = savedPos;
            return false;
        }
        
        // Skip any JSON value (object, array, string, number, boolean, null)
        void skipValue() {
            skipWhitespace();
            
            if (absPos() >= dataLen) return;
            
            if (data[absPos()] == '{') {
                // Skip object
                int depth = 1;
                pos++;
                
                while (absPos() < dataLen && depth > 0) {
                    if (data[absPos()] == '{') depth++;
                    else if (data[absPos()] == '}') depth--;
                    pos++;
                }
            }
            else if (data[absPos()] == '[') {
                // Skip array
                int depth = 1;
                pos++;
                
                while (absPos() < dataLen && depth > 0) {
                    if (data[absPos()] == '[') depth++;
                    else if (data[absPos()] == ']') depth--;
                    pos++;
                }
            }
            else if (data[absPos()] == '"') {
                // Skip string
                pos++;
                while (absPos() < dataLen && data[absPos()] != '"') {
                    // Handle escaped characters
                    if (data[absPos()] == '\\' && absPos() + 1 < dataLen) pos++;
                    pos++;
                }
                if (absPos() < dataLen) pos++;  // Skip closing quote
            }
            else if (isdigit(data[absPos()]) || data[absPos()] == '-') {
                // Skip number
                if (data[absPos()] == '-') pos++;
                
                // Skip digits before decimal point
                while (absPos() < dataLen && isdigit(data[absPos()])) pos++;
                
                // Skip decimal point and following digits
                if (absPos() < dataLen && data[absPos()] == '.') {
                    pos++;
                    while (absPos() < dataLen && isdigit(data[absPos()])) pos++;
                }
                
                // Skip exponent
                if (absPos() < dataLen && (data[absPos()] == 'e' || data[absPos()] == 'E')) {
                    pos++;
                    
                    // Skip optional sign
                    if (absPos() < dataLen && (data[absPos()] == '+' || data[absPos()] == '-')) pos++;
                    
                    // Skip exponent digits
                    while (absPos() < dataLen && isdigit(data[absPos()])) pos++;
                }
            }
            else if (absPos() + 4 <= dataLen && strncmp(data + absPos(), "true", 4) == 0) {
                pos += 4;  // Skip "true"
            }
            else if (absPos() + 5 <= dataLen && strncmp(data + absPos(), "false", 5) == 0) {
                pos += 5;  // Skip "false"
            }
            else if (absPos() + 4 <= dataLen && strncmp(data + absPos(), "null", 4) == 0) {
                pos += 4;  // Skip "null"
            }
        }
        
        // Get string value at current position
        bool getStringValue(char* value, size_t maxLen) {
            skipWhitespace();
            
            if (absPos() >= dataLen || data[absPos()] != '"') return false;
            
            pos++;  // Skip opening quote
            size_t i = 0;
            
            while (absPos() < dataLen && data[absPos()] != '"' && i < maxLen - 1) {
                // Handle escaped characters
                if (data[absPos()] == '\\' && absPos() + 1 < dataLen) {
                    pos++;
                    // Simple escape sequence handling
                    switch (data[absPos()]) {
                        case 'n': value[i++] = '\n'; break;
                        case 'r': value[i++] = '\r'; break;
                        case 't': value[i++] = '\t'; break;
                        default: value[i++] = data[absPos()];
                    }
                } else {
                    value[i++] = data[absPos()];
                }
                pos++;
            }
            
            value[i] = '\0';
            
            if (absPos() < dataLen && data[absPos()] == '"') pos++;  // Skip closing quote
            return true;
        }
        
        bool getStringValue(zap::Str& value) {
            skipWhitespace();
            
            if (absPos() >= dataLen || data[absPos()] != '"') return false;
            
            pos++;  // Skip opening quote
            value.clear();
            
            while (absPos() < dataLen && data[absPos()] != '"') {
                // Handle escaped characters
                if (data[absPos()] == '\\' && absPos() + 1 < dataLen) {
                    pos++;
                    // Simple escape sequence handling
                    switch (data[absPos()]) {
                        case 'n': value += '\n'; break;
                        case 'r': value += '\r'; break;
                        case 't': value += '\t'; break;
                        default: value += data[absPos()];
                    }
                } else {
                    value += data[absPos()];
                }
                pos++;
            }
            
            if (absPos() < dataLen && data[absPos()] == '"') pos++;  // Skip closing quote
            return true;
        }
        
        // Get integer value at current position
        bool getIntValue(int& value) {
            skipWhitespace();
            
            if (absPos() >= dataLen) return false;
            
            char* endPtr;
            value = strtol(data + absPos(), &endPtr, 10);
            
            if (endPtr == data + absPos()) return false;  // No conversion
            
            pos += (endPtr - (data + absPos()));
            return true;
        }
        
        // Get boolean value at current position
        bool getBoolValue(bool& value) {
            skipWhitespace();
            
            if (absPos() >= dataLen) return false;
            
            if (absPos() + 4 <= dataLen && strncmp(data + absPos(), "true", 4) == 0) {
                value = true;
                pos += 4;
                return true;
            } 
            else if (absPos() + 5 <= dataLen && strncmp(data + absPos(), "false", 5) == 0) {
                value = false;
                pos += 5;
                return true;
            }
            return false;
        }
        
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
        bool getObject(const char* key, JsonParser& result) {
            size_t savedPos = pos;
            
            if (!findKey(key)) {
                return false;
            }
            
            // We should now be positioned at the start of an object
            if (absPos() >= dataLen || data[absPos()] != '{') {
                pos = savedPos;
                return false;
            }
            
            // Find object boundaries
            size_t objectStart = absPos();
            int depth = 1;
            pos++;  // Skip opening brace
            
            while (absPos() < dataLen && depth > 0) {
                if (data[absPos()] == '{') depth++;
                else if (data[absPos()] == '}') depth--;
                pos++;
            }
            
            if (depth != 0) {
                pos = savedPos;
                return false;  // Unbalanced braces
            }
            
            size_t objectEnd = absPos();
            
            // Create a new parser as a view into this section of the buffer
            result = JsonParser(data, dataLen, objectStart, objectEnd);
            
            return true;
        }
        
        // Get a string value by key
        bool getString(const char* key, char* value, size_t maxLen) {
            size_t savedPos = pos;
            
            if (findKey(key)) {
                if (getStringValue(value, maxLen)) {
                    return true;
                }
            }
            
            pos = savedPos;
            return false;
        }
        
        bool getString(const char* key, zap::Str& value) {
            size_t savedPos = pos;
            
            if (findKey(key)) {
                if (getStringValue(value)) {
                    return true;
                }
            }
            
            pos = savedPos;
            return false;
        }
        
        // Get an integer value by key
        bool getInt(const char* key, int& value) {
            size_t savedPos = pos;
            
            if (findKey(key)) {
                if (getIntValue(value)) {
                    return true;
                }
            }
            
            pos = savedPos;
            return false;
        }
        
        // Get a boolean value by key
        bool getBool(const char* key, bool& value) {
            size_t savedPos = pos;
            
            if (findKey(key)) {
                if (getBoolValue(value)) {
                    return true;
                }
            }
            
            pos = savedPos;
            return false;
        }
        
        // Get a value using dot notation path
        bool getValueByPath(const char* path, const std::function<bool()>& valueExtractor) {
            // Make a working copy of the path
            char pathCopy[256];
            strncpy(pathCopy, path, sizeof(pathCopy) - 1);
            pathCopy[sizeof(pathCopy) - 1] = '\0';
            
            // Save initial position
            size_t savedPos = pos;
            pos = 0;
            
            // Split path by dots
            char* savePtr = nullptr;
            char* segment = strtok_r(pathCopy, ".", &savePtr);
            
            if (!segment) {
                pos = savedPos;
                return false;
            }
            
            JsonParser currentParser = *this;  // Start with a copy of this parser
            
            while (true) {
                char* nextSegment = strtok_r(nullptr, ".", &savePtr);
                
                if (!nextSegment) {
                    // Last segment - get the value
                    if (!currentParser.findKey(segment)) {
                        pos = savedPos;
                        return false;
                    }
                    
                    // Use the current parser's context to extract the value
                    // Need to temporarily swap 'this' context with currentParser's
                    size_t thisPos = pos;
                    size_t thisStartPos = startPos;
                    size_t thisEndPos = endPos;
                    
                    pos = currentParser.pos;
                    startPos = currentParser.startPos;
                    endPos = currentParser.endPos;
                    
                    bool result = valueExtractor();
                    
                    // Restore original context
                    pos = thisPos;
                    startPos = thisStartPos;
                    endPos = thisEndPos;
                    
                    return result;
                }
                
                // Not the last segment - navigate to sub-object
                JsonParser nextParser(nullptr, 0, 0, 0);  // Dummy initialization
                
                if (!currentParser.getObject(segment, nextParser)) {
                    pos = savedPos;
                    return false;
                }
                
                // Move to the next object
                currentParser = nextParser;
                segment = nextSegment;
            }
            
            // Should never reach here
            pos = savedPos;
            return false;
        }
        
        // Path-based value getters
        bool getStringByPath(const char* path, char* value, size_t maxLen) {
            return getValueByPath(path, [this, value, maxLen]() {
                return getStringValue(value, maxLen);
            });
        }
        
        bool getStringByPath(const char* path, zap::Str& value) {
            return getValueByPath(path, [this, &value]() {
                return getStringValue(value);
            });
        }
        
        bool getIntByPath(const char* path, int& value) {
            return getValueByPath(path, [this, &value]() {
                return getIntValue(value);
            });
        }
        
        bool getBoolByPath(const char* path, bool& value) {
            return getValueByPath(path, [this, &value]() {
                return getBoolValue(value);
            });
        }
        
        // Reset parser position (within current view)
        void reset() {
            pos = 0;
        }
    };