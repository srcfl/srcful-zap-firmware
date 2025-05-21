#include "json_light.h"
#include <cstring>
#include <cstdlib>



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

// Helper function to convert 4 hex chars to a UTF-8 sequence
// Note: This is a simplified version. A full implementation would handle surrogate pairs
// and potentially different UTF encodings based on system needs.
// Returns the number of bytes written to utf8_buffer (max 4), or 0 on error.
int hex_to_utf8(const char* hex_chars, char* utf8_buffer) {
    unsigned int codepoint;
    if (sscanf(hex_chars, "%4x", &codepoint) != 1) {
        return 0; // Invalid hex sequence
    }

    if (codepoint <= 0x7F) {
        utf8_buffer[0] = (char)codepoint;
        return 1;
    } else if (codepoint <= 0x7FF) {
        utf8_buffer[0] = (char)(0xC0 | (codepoint >> 6));
        utf8_buffer[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    } else if (codepoint <= 0xFFFF) {
        utf8_buffer[0] = (char)(0xE0 | (codepoint >> 12));
        utf8_buffer[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8_buffer[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    }
     // For codepoints > 0xFFFF (requires surrogate pairs in \u sequences, more complex)
     // or handle as needed for your specific embedded environment / character set.
     // This basic version handles up to U+FFFF.
     else {
         // Handle codepoints requiring 4 bytes in UTF-8 (outside Basic Multilingual Plane)
         // This requires handling surrogate pairs (\uD800\uDC00-\uDBFF\uDFFF) which adds complexity.
         // For simplicity, we might represent these with a replacement character or return an error.
         // Example: replace with '?'
         utf8_buffer[0] = '?';
         return 1; // Or return 0 for error
     }

     // We will return '?' for codepoints beyond the basic multilingual plane for now.
     // utf8_buffer[0] = (char)(0xF0 | (codepoint >> 18));
     // utf8_buffer[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
     // utf8_buffer[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
     // utf8_buffer[3] = (char)(0x80 | (codepoint & 0x3F));
     // return 4;

    return 0; // Should not happen with current logic, but added for safety
}


// --- Modified getStringValue for C-style string ---
bool JsonParser::getStringValue(size_t pos, char* value, size_t maxLen, size_t& endPos) const {
    pos = skipWhitespace(pos);

    if (absPos(pos) >= dataLen || data[absPos(pos)] != '"') {
        endPos = pos;
        if (maxLen > 0) value[0] = '\0'; // Ensure null termination on error
        return false;
    }

    pos++;  // Skip opening quote
    size_t i = 0;

    while (absPos(pos) < dataLen && data[absPos(pos)] != '"') {
        if (i >= maxLen - 1) { // Check buffer overflow BEFORE writing
             // Buffer full, stop processing but continue to find end quote
             while (absPos(pos) < dataLen && data[absPos(pos)] != '"') {
                 // Need to handle escaped quotes even when buffer is full
                  if (data[absPos(pos)] == '\\' && absPos(pos) + 1 < dataLen) {
                     pos++; // Skip the escaped character
                     if (data[absPos(pos)] == 'u' && absPos(pos) + 4 < dataLen) {
                         pos += 4; // Skip the 4 hex digits
                     }
                 }
                 pos++;
             }
             value[i] = '\0'; // Null terminate what we have
             if (absPos(pos) < dataLen && data[absPos(pos)] == '"') pos++; // Skip closing quote
             endPos = pos;
             // Consider returning false or a specific status indicating truncation
             return false; // Indicate failure due to insufficient buffer size
        }

        if (data[absPos(pos)] == '\\' && absPos(pos) + 1 < dataLen) {
            // Handle escape sequence
            pos++; // Move past the backslash
            switch (data[absPos(pos)]) {
                case '"':  value[i++] = '"'; break;
                case '\\': value[i++] = '\\'; break;
                case '/':  value[i++] = '/'; break;
                case 'b':  value[i++] = '\b'; break;
                case 'f':  value[i++] = '\f'; break;
                case 'n':  value[i++] = '\n'; break;
                case 'r':  value[i++] = '\r'; break;
                case 't':  value[i++] = '\t'; break;
                case 'u':
                    // Handle \uXXXX unicode escape
                    if (absPos(pos) + 4 < dataLen) {
                        char utf8_buf[4];
                        int bytes_written = hex_to_utf8(data + absPos(pos) + 1, utf8_buf);
                        if (bytes_written > 0 && i + bytes_written < maxLen) {
                             memcpy(value + i, utf8_buf, bytes_written);
                             i += bytes_written;
                             pos += 4; // Skip the 4 hex digits
                        } else {
                            // Error in hex sequence or not enough space in buffer
                            // Append a replacement char like '?' if possible
                            if (i < maxLen -1 ) value[i++] = '?';
                             pos += 4; // Skip the 4 hex digits anyway
                        }
                    } else {
                        // Invalid \u sequence (not enough chars)
                        if (i < maxLen -1 ) value[i++] = '?'; // Append replacement char
                        // Advance pos to avoid getting stuck, but might skip valid chars if JSON is malformed.
                        // Move to the end if sequence is cut short.
                         pos = (dataLen - startPos) -1;
                    }
                    break;
                default:
                    // Invalid escape sequence, just append the character after backslash
                    value[i++] = data[absPos(pos)];
                    break;
            }
        } else {
            // Regular character
            value[i++] = data[absPos(pos)];
        }
        pos++; // Move to the next character in the source JSON
    }

    value[i] = '\0'; // Null terminate the result string

    if (absPos(pos) < dataLen && data[absPos(pos)] == '"') {
        pos++;  // Skip closing quote
        endPos = pos;
        return true;
    } else {
        // String wasn't properly terminated with a quote
        endPos = pos; // endPos will be at the end or where the loop stopped
        return false; // Indicate error: unterminated string
    }
}


// --- Modified getStringValue for zap::Str ---
bool JsonParser::getStringValue(size_t pos, zap::Str& value, size_t& endPos) const {
    pos = skipWhitespace(pos);

    if (absPos(pos) >= dataLen || data[absPos(pos)] != '"') {
        endPos = pos;
        value.clear(); // Ensure value is empty on error
        return false;
    }

    pos++;  // Skip opening quote
    value.clear(); // Start with an empty string

    while (absPos(pos) < dataLen && data[absPos(pos)] != '"') {
        if (data[absPos(pos)] == '\\' && absPos(pos) + 1 < dataLen) {
            // Handle escape sequence
            pos++; // Move past the backslash
            switch (data[absPos(pos)]) {
                case '"':  value += '"'; break;
                case '\\': value += '\\'; break;
                case '/':  value += '/'; break;
                case 'b':  value += '\b'; break;
                case 'f':  value += '\f'; break;
                case 'n':  value += '\n'; break;
                case 'r':  value += '\r'; break;
                case 't':  value += '\t'; break;
                case 'u':
                     // Handle \uXXXX unicode escape
                    if (absPos(pos) + 4 < dataLen) {
                        char utf8_buf[4];
                        int bytes_written = hex_to_utf8(data + absPos(pos) + 1, utf8_buf);
                         if (bytes_written > 0) {
                             // Append the UTF-8 bytes directly
                            value.append(utf8_buf, bytes_written);
                             pos += 4; // Skip the 4 hex digits
                        } else {
                            // Error in hex sequence
                            value += '?'; // Append replacement char
                             pos += 4; // Skip the 4 hex digits anyway
                        }
                    } else {
                         // Invalid \u sequence (not enough chars)
                        value += '?'; // Append replacement char
                        // Advance pos to avoid getting stuck, but might skip valid chars if JSON is malformed.
                        // Move to the end if sequence is cut short.
                         pos = (dataLen - startPos) -1;
                    }
                    break;
                default:
                    // Invalid escape sequence, just append the character after backslash
                    value += data[absPos(pos)];
                    break;
            }
        } else {
            // Regular character
            value += data[absPos(pos)];
        }
         pos++; // Move to the next character in the source JSON
    }

    if (absPos(pos) < dataLen && data[absPos(pos)] == '"') {
        pos++;  // Skip closing quote
        endPos = pos;
        return true;
    } else {
        // String wasn't properly terminated with a quote
        endPos = pos; // endPos will be at the end or where the loop stopped
        // value will contain the partially parsed string
        return false; // Indicate error: unterminated string
    }
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
    return zap::Str("");
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

void JsonParser::asString(zap::Str& value) const {
    value.clear();
    for (size_t i = startPos; i < endPos; i++) {
        value += data[i];
    }
}