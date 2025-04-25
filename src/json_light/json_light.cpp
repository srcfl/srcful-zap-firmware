#include "json_light.h"
#include <cstring>
#include <cstdlib>

// JsonBuilder implementation

JsonBuilder& JsonBuilder::beginObject() {
    buffer += '{';
    objectStack.push_back(firstItem); // Save current state
    firstItem = true;
    inObject = true;
    return *this;
}

JsonBuilder& JsonBuilder::beginObject(const char* key) {
    if (!firstItem) buffer += ',';
    buffer += '"';
    buffer += key;
    buffer += "\":{";
    objectStack.push_back(firstItem); // Save current state
    firstItem = true;
    inObject = true;
    return *this;
}

JsonBuilder& JsonBuilder::endObject() {
    buffer += '}';
    if (!objectStack.empty()) {
        firstItem = objectStack.back(); // Restore previous state
        objectStack.pop_back();
        firstItem = false; // After closing an object, we've added an item to the parent
    }
    return *this;
}

JsonBuilder& JsonBuilder::add(const char* key, const char* value) {
    if (!firstItem) buffer += ',';
    buffer += '"';
    buffer += key;
    buffer += "\":\"";
    buffer += value;
    buffer += '"';
    firstItem = false;
    return *this;
}

JsonBuilder& JsonBuilder::add(const char* key, int value) {
    if (!firstItem) buffer += ',';
    buffer += '"';
    buffer += key;
    buffer += "\":";
    buffer += value;
    firstItem = false;
    return *this;
}

JsonBuilder& JsonBuilder::add(const char* key, uint32_t value) {
    if (!firstItem) buffer += ',';
    buffer += '"';
    buffer += key;
    buffer += "\":";
    buffer += value;
    firstItem = false;
    return *this;
}

JsonBuilder& JsonBuilder::add(const char* key, uint64_t value) {
    if (!firstItem) buffer += ',';
    buffer += '"';
    buffer += key;
    buffer += "\":";
    buffer += value;
    firstItem = false;
    return *this;
}

JsonBuilder& JsonBuilder::add(const char* key, bool value) {
    if (!firstItem) buffer += ',';
    buffer += '"';
    buffer += key;
    buffer += "\":";
    buffer += value ? "true" : "false";
    firstItem = false;
    return *this;
}

JsonBuilder& JsonBuilder::add(const char* key, uint8_t* data, size_t size) {
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

JsonBuilder& JsonBuilder::addArray(const char* key, const std::vector<zap::Str>& values) {
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

JsonBuilder& JsonBuilder::addArray(const char* key, const char* const* values, size_t count) {
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

zap::Str JsonBuilder::end() {
    while (inObject && !objectStack.empty()) {
        buffer += '}';
        objectStack.pop_back();
    }
    inObject = false;
    return buffer;
}

void JsonBuilder::clear() {
    buffer.clear();
    firstItem = true;
    inObject = false;
    objectStack.clear();
}

// JsonParser implementation

size_t JsonParser::absPos(size_t relPos) const {
    return startPos + relPos;
}

size_t JsonParser::skipWhitespace(size_t pos) const {
    while (pos < (endPos - startPos) && isspace(data[startPos + pos])) {
        pos++;
    }
    return pos;
}

size_t JsonParser::findKey(const char* key, size_t pos) const {
    pos = skipWhitespace(pos);
    
    // If at the start of the view, expect an opening brace
    if (pos == 0) {
        if (absPos(pos) >= dataLen || data[absPos(pos)] != '{') {
            return 0;
        }
        pos++;
        pos = skipWhitespace(pos);
    }
    
    while (absPos(pos) < dataLen && absPos(pos) < endPos) {
        // Check for end of object
        if (data[absPos(pos)] == '}') {
            return 0;
        }
        
        // Check for comma
        if (data[absPos(pos)] == ',') {
            pos++;
            pos = skipWhitespace(pos);
        }
        
        // Expect a quoted key
        if (data[absPos(pos)] != '"') {
            return 0;
        }
        pos++;
        
        // Compare with the requested key
        size_t keyLen = strlen(key);
        if (absPos(pos) + keyLen <= dataLen && 
            strncmp(data + absPos(pos), key, keyLen) == 0 && 
            absPos(pos) + keyLen < dataLen && data[absPos(pos) + keyLen] == '"') {
            
            // Found the key
            pos += keyLen + 1;  // Skip key and closing quote
            pos = skipWhitespace(pos);
            
            // Expect a colon
            if (absPos(pos) >= dataLen || data[absPos(pos)] != ':') {
                return 0;
            }
            pos++;
            pos = skipWhitespace(pos);
            
            return pos;
        }
        
        // Not the key we want, skip to the end of this key
        while (absPos(pos) < dataLen && data[absPos(pos)] != '"') {
            pos++;
        }
        if (absPos(pos) >= dataLen) {
            return 0;
        }
        pos++;  // Skip closing quote
        
        pos = skipWhitespace(pos);
        
        // Expect a colon
        if (absPos(pos) >= dataLen || data[absPos(pos)] != ':') {
            return 0;
        }
        pos++;
        pos = skipWhitespace(pos);
        
        // Skip this value
        pos = skipValue(pos);
        pos = skipWhitespace(pos);
    }
    
    return 0;
}

size_t JsonParser::skipValue(size_t pos) const {
    pos = skipWhitespace(pos);
    
    if (absPos(pos) >= dataLen) return pos;
    
    if (data[absPos(pos)] == '{') {
        // Skip object
        int depth = 1;
        pos++;
        
        while (absPos(pos) < dataLen && depth > 0) {
            if (data[absPos(pos)] == '{') depth++;
            else if (data[absPos(pos)] == '}') depth--;
            pos++;
        }
    }
    else if (data[absPos(pos)] == '[') {
        // Skip array
        int depth = 1;
        pos++;
        
        while (absPos(pos) < dataLen && depth > 0) {
            if (data[absPos(pos)] == '[') depth++;
            else if (data[absPos(pos)] == ']') depth--;
            pos++;
        }
    }
    else if (data[absPos(pos)] == '"') {
        // Skip string
        pos++;
        while (absPos(pos) < dataLen && data[absPos(pos)] != '"') {
            // Handle escaped characters
            if (data[absPos(pos)] == '\\' && absPos(pos) + 1 < dataLen) pos++;
            pos++;
        }
        if (absPos(pos) < dataLen) pos++;  // Skip closing quote
    }
    else if (isdigit(data[absPos(pos)]) || data[absPos(pos)] == '-') {
        // Skip number
        if (data[absPos(pos)] == '-') pos++;
        
        // Skip digits before decimal point
        while (absPos(pos) < dataLen && isdigit(data[absPos(pos)])) pos++;
        
        // Skip decimal point and following digits
        if (absPos(pos) < dataLen && data[absPos(pos)] == '.') {
            pos++;
            while (absPos(pos) < dataLen && isdigit(data[absPos(pos)])) pos++;
        }
        
        // Skip exponent
        if (absPos(pos) < dataLen && (data[absPos(pos)] == 'e' || data[absPos(pos)] == 'E')) {
            pos++;
            
            // Skip optional sign
            if (absPos(pos) < dataLen && (data[absPos(pos)] == '+' || data[absPos(pos)] == '-')) pos++;
            
            // Skip exponent digits
            while (absPos(pos) < dataLen && isdigit(data[absPos(pos)])) pos++;
        }
    }
    else if (absPos(pos) + 4 <= dataLen && strncmp(data + absPos(pos), "true", 4) == 0) {
        pos += 4;  // Skip "true"
    }
    else if (absPos(pos) + 5 <= dataLen && strncmp(data + absPos(pos), "false", 5) == 0) {
        pos += 5;  // Skip "false"
    }
    else if (absPos(pos) + 4 <= dataLen && strncmp(data + absPos(pos), "null", 4) == 0) {
        pos += 4;  // Skip "null"
    }
    
    return pos;
}

bool JsonParser::getStringValue(size_t pos, char* value, size_t maxLen, size_t& endPos) const {
    pos = skipWhitespace(pos);
    
    if (absPos(pos) >= dataLen || data[absPos(pos)] != '"') {
        endPos = pos;
        return false;
    }
    
    pos++;  // Skip opening quote
    size_t i = 0;
    
    while (absPos(pos) < dataLen && data[absPos(pos)] != '"' && i < maxLen - 1) {
        // We do NOT Handle escaped characters
        value[i++] = data[absPos(pos)];
        
        pos++;
    }
    
    value[i] = '\0';
    
    if (absPos(pos) < dataLen && data[absPos(pos)] == '"') pos++;  // Skip closing quote
    
    endPos = pos;
    return true;
}

bool JsonParser::getStringValue(size_t pos, zap::Str& value, size_t& endPos) const {
    pos = skipWhitespace(pos);
    
    if (absPos(pos) >= dataLen || data[absPos(pos)] != '"') {
        endPos = pos;
        return false;
    }
    
    pos++;  // Skip opening quote
    value.clear();
    
    while (absPos(pos) < dataLen && data[absPos(pos)] != '"') {
        // We do NOT Handle escaped characters
        value += data[absPos(pos)];
        pos++;
    }
    
    if (absPos(pos) < dataLen && data[absPos(pos)] == '"') pos++;  // Skip closing quote
    
    endPos = pos;
    return true;
}



bool JsonParser::getIntValue(size_t pos, int& value, size_t& endPos) const {
    pos = skipWhitespace(pos);
    
    if (absPos(pos) >= dataLen) {
        endPos = pos;
        return false;
    }
    
    char* endPtr;
    value = strtol(data + absPos(pos), &endPtr, 10);
    
    if (endPtr == data + absPos(pos)) {
        endPos = pos;
        return false;  // No conversion
    }
    
    size_t charsRead = endPtr - (data + absPos(pos));
    pos += charsRead;
    
    endPos = pos;
    return true;
}

bool JsonParser::getBoolValue(size_t pos, bool& value, size_t& endPos) const {
    pos = skipWhitespace(pos);
    
    if (absPos(pos) >= dataLen) {
        endPos = pos;
        return false;
    }
    
    if (absPos(pos) + 4 <= dataLen && strncmp(data + absPos(pos), "true", 4) == 0) {
        value = true;
        pos += 4;
        endPos = pos;
        return true;
    } 
    else if (absPos(pos) + 5 <= dataLen && strncmp(data + absPos(pos), "false", 5) == 0) {
        value = false;
        pos += 5;
        endPos = pos;
        return true;
    }
    
    endPos = pos;
    return false;
}

bool JsonParser::getObject(const char* key, JsonParser& result) {
    size_t pos = findKey(key);
    
    if (pos == 0) {
        return false;
    }
    
    // We should now be positioned at the start of an object
    if (absPos(pos) >= dataLen || data[absPos(pos)] != '{') {
        return false;
    }
    
    // Find object boundaries
    size_t objectStart = absPos(pos);
    int depth = 1;
    pos++;  // Skip opening brace
    
    while (absPos(pos) < dataLen && depth > 0) {
        if (data[absPos(pos)] == '{') depth++;
        else if (data[absPos(pos)] == '}') depth--;
        pos++;
    }
    
    if (depth != 0) {
        return false;  // Unbalanced braces
    }
    
    size_t objectEnd = absPos(pos);
    
    // Create a new parser as a view into this section of the buffer
    result = JsonParser(data, dataLen, objectStart, objectEnd);
    
    return true;
}

bool JsonParser::getObjectValue(size_t pos, JsonParser& result, size_t& endPos) const {
    pos = skipWhitespace(pos);
    const size_t objectStart = absPos(pos);
    
    if (absPos(pos) >= dataLen || data[absPos(pos)] != '{') {
        endPos = pos;
        return false;
    }
    
    // Find object boundaries
    int depth = 1;
    pos++;  // Skip opening brace
    
    while (absPos(pos) < dataLen && depth > 0) {
        if (data[absPos(pos)] == '{') depth++;
        else if (data[absPos(pos)] == '}') depth--;
        pos++;
    }
    
    if (depth != 0) {
        return false;  // Unbalanced braces
    }
    
    size_t objectEnd = absPos(pos);
    
    // Create a new parser as a view into this section of the buffer
    result = JsonParser(data, dataLen, objectStart, objectEnd);
    
    endPos = pos;
    return true;
}

bool JsonParser::getString(const char* key, char* value, size_t maxLen) {
    size_t pos = findKey(key);
    
    if (pos == 0) {
        return false;
    }
    
    size_t endPos;
    return getStringValue(pos, value, maxLen, endPos);
}

bool JsonParser::getString(const char* key, zap::Str& value) {
    size_t pos = findKey(key);
    
    if (pos == 0) {
        return false;
    }
    
    size_t endPos;
    return getStringValue(pos, value, endPos);
}

bool JsonParser::getInt(const char* key, int& value) {
    size_t pos = findKey(key);
    
    if (pos == 0) {
        return false;
    }
    
    size_t endPos;
    return getIntValue(pos, value, endPos);
}

bool JsonParser::getBool(const char* key, bool& value) {
    size_t pos = findKey(key);
    
    if (pos == 0) {
        return false;
    }
    
    size_t endPos;
    return getBoolValue(pos, value, endPos);
}

bool JsonParser::getValueByPath(const char* path, std::function<bool(const JsonParser&, size_t, size_t&)> valueExtractor) {
    // Make a working copy of the path
    char pathCopy[256];
    strncpy(pathCopy, path, sizeof(pathCopy) - 1);
    pathCopy[sizeof(pathCopy) - 1] = '\0';
    
    // Split path by dots
    char* savePtr = nullptr;
    char* segment = strtok_r(pathCopy, ".", &savePtr);
    
    if (!segment) {
        return false;
    }
    
    JsonParser currentParser = *this;  // Start with a copy of this parser
    
    while (true) {
        char* nextSegment = strtok_r(nullptr, ".", &savePtr);
        
        if (!nextSegment) {
            // Last segment - get the value
            size_t pos = currentParser.findKey(segment);
            if (pos == 0) {
                return false;
            }
            
            size_t endPos;
            return valueExtractor(currentParser, pos, endPos);
        }
        
        // Not the last segment - navigate to sub-object
        JsonParser nextParser(nullptr, 0, 0, 0);  // Dummy initialization
        
        if (!currentParser.getObject(segment, nextParser)) {
            return false;
        }
        
        // Move to the next object
        currentParser = nextParser;
        segment = nextSegment;
    }
    
    // Should never reach here
    return false;
}

bool JsonParser::getObjectByPath(const char* path, JsonParser& result) {
    return getValueByPath(path, [this, &result](const JsonParser& parser, size_t pos, size_t& endPos) {
        return parser.getObjectValue(pos, result, endPos);
    });
}

bool JsonParser::getStringByPath(const char* path, char* value, size_t maxLen) {
    return getValueByPath(path, [this, value, maxLen](const JsonParser& parser, size_t pos, size_t& endPos) {
        return parser.getStringValue(pos, value, maxLen, endPos);
    });
}

bool JsonParser::getStringByPath(const char* path, zap::Str& value) {
    return getValueByPath(path, [this, &value](const JsonParser& parser, size_t pos, size_t& endPos) {
        return parser.getStringValue(pos, value, endPos);
    });
}

bool JsonParser::getIntByPath(const char* path, int& value) {
    return getValueByPath(path, [this, &value](const JsonParser& parser, size_t pos, size_t& endPos) {
        return parser.getIntValue(pos, value, endPos);
    });
}

bool JsonParser::getBoolByPath(const char* path, bool& value) {
    return getValueByPath(path, [this, &value](const JsonParser& parser, size_t pos, size_t& endPos) {
        return parser.getBoolValue(pos, value, endPos);
    });
}

bool JsonParser::isFieldNullByPath(const char* path) {
    // Make a working copy of the path
    char pathCopy[256];
    strncpy(pathCopy, path, sizeof(pathCopy) - 1);
    pathCopy[sizeof(pathCopy) - 1] = '\0';
    
    // Split path by dots
    char* savePtr = nullptr;
    char* segment = strtok_r(pathCopy, ".", &savePtr);
    
    if (!segment) {
        return false;
    }
    
    JsonParser currentParser = *this;  // Start with a copy of this parser
    
    while (true) {
        char* nextSegment = strtok_r(nullptr, ".", &savePtr);
        
        if (!nextSegment) {
            // Last segment - check if the value is null
            size_t pos = currentParser.findKey(segment);
            if (pos == 0) {
                return false; // Key not found
            }
            
            // Position is now after the key and colon
            pos = currentParser.skipWhitespace(pos);
            
            // Check if the value is "null"
            if (currentParser.absPos(pos) + 4 <= currentParser.dataLen && 
                strncmp(currentParser.data + currentParser.absPos(pos), "null", 4) == 0) {
                return true; // Found null value
            }
            
            return false; // Value exists but is not null
        }
        
        // Not the last segment - navigate to sub-object
        JsonParser nextParser(nullptr, 0, 0, 0);  // Dummy initialization
        
        if (!currentParser.getObject(segment, nextParser)) {
            return false;
        }
        
        // Move to the next object
        currentParser = nextParser;
        segment = nextSegment;
    }
    
    // Should never reach here
    return false;
}

bool JsonParser::contains(const char* key) {
    return findKey(key) != 0;
}

zap::Str JsonParser::getStringOrEmpty(const char* key) {
    zap::Str value;
    if (getString(key, value)) {
        return value;
    }
    return "";
}

bool JsonParser::getUInt64(const char* key, uint64_t& value) {
    size_t pos = findKey(key);
    
    if (pos == 0) {
        return false;
    }
    
    pos = skipWhitespace(pos);
    
    if (absPos(pos) >= dataLen) {
        return false;
    }
    
    char* endPtr;
    value = strtoull(data + absPos(pos), &endPtr, 10);
    
    if (endPtr == data + absPos(pos)) {
        return false;  // No conversion
    }
    
    return true;
}

uint64_t JsonParser::getUInt64(const char* key) {
    uint64_t value = 0;
    getUInt64(key, value);
    return value;
}