#pragma once

#include <stdlib.h>
#include <string.h>
#include <cstdio>  // Added for sprintf
#include <limits.h>
#include <stdint.h>
#include <ctype.h>  // Added for isspace

namespace zap {

/**
 * A simplified string class that uses a C-style buffer internally.
 * This is meant to replace the Arduino String class for testing purposes.
 */
class Str {
private:
    char* _buffer;      // Points to the allocated memory
    size_t _length;     // Current length of string (excluding null terminator)
    size_t _capacity;   // Current allocated capacity (including null terminator)

    // Helper method to convert uint64_t to string without using sprintf
    static void uint64ToString(uint64_t value, char* buffer, size_t bufferSize) {
        if (bufferSize == 0) return;
        
        // Handle the special case of zero
        if (value == 0) {
            buffer[0] = '0';
            buffer[1] = '\0';
            return;
        }
        
        // Convert digits in reverse order
        size_t pos = 0;
        while (value > 0 && pos < bufferSize - 1) {
            buffer[pos++] = '0' + (value % 10);
            value /= 10;
        }
        
        // Null terminate the string
        buffer[pos] = '\0';
        
        // Reverse the string
        for (size_t i = 0, j = pos - 1; i < j; i++, j--) {
            char temp = buffer[i];
            buffer[i] = buffer[j];
            buffer[j] = temp;
        }
    }

    public:
    // Helper method to ensure there's enough capacity for a string of given length
    bool reserve(size_t length) {
        if (length < _capacity) return true; // Already enough space

        // Calculate new capacity (add extra space to avoid frequent reallocations)
        size_t newCapacity = length + 16; 

        // Allocate or reallocate memory
        if (_buffer) {
            char* newBuffer = (char*)realloc(_buffer, newCapacity);
            if (!newBuffer) return false;
            _buffer = newBuffer;
        } else {
            _buffer = (char*)malloc(newCapacity);
            if (!_buffer) return false;
            _buffer[0] = '\0'; // Initialize as empty string
        }

        _capacity = newCapacity;
        return true;
    }

    // Constructors
    Str() : _buffer(nullptr), _length(0), _capacity(0) {
        reserve(1); // Reserve space for at least the null terminator
    }

    Str(const char* cstr) : _buffer(nullptr), _length(0), _capacity(0) {
        if (cstr) {
            _length = strlen(cstr);
            if (reserve(_length + 1)) {
                strcpy(_buffer, cstr);
            }
        } else {
            reserve(1); // Reserve space for at least the null terminator
        }
    }

    Str(char c) : _buffer(nullptr), _length(0), _capacity(0) {
        if (reserve(2)) {
            _buffer[0] = c;
            _buffer[1] = '\0';
            _length = 1;
        }
    }

    Str(const Str& other) : _buffer(nullptr), _length(0), _capacity(0) {
        *this = other;
    }

    // Constructor with int
    explicit Str(int value) : _buffer(nullptr), _length(0), _capacity(0) {
        char temp[12]; // Enough for int32_t
        sprintf(temp, "%d", value);
        _length = strlen(temp);
        if (reserve(_length + 1)) {
            strcpy(_buffer, temp);
        }
    }

    // Constructor with float
    explicit Str(float value) : _buffer(nullptr), _length(0), _capacity(0) {
        // Determine the required buffer size using snprintf with a NULL buffer.
        // Using %g often gives a more compact representation than %f.
        int required_len = snprintf(NULL, 0, "%g", value);

        if (required_len < 0) {
            // Handle snprintf encoding error, initialize to empty.
            reserve(1);
            return;
        }

        // Reserve enough space for the string plus the null terminator.
        if (reserve(static_cast<size_t>(required_len) + 1)) {
            // Format the float directly into the allocated buffer.
            // Pass the actual capacity to snprintf for safety.
            int written_len = snprintf(_buffer, _capacity, "%g", value);

            if (written_len >= 0 && static_cast<size_t>(written_len) < _capacity) {
                // Successfully written, update the length.
                _length = static_cast<size_t>(written_len);
            } else {
                // Handle potential snprintf error during writing. Clear the string.
                _buffer[0] = '\0';
                _length = 0;
            }
        }
        // If reserve fails, the string remains in its default empty state.
    }

    // Constructor with unsigned int
    explicit Str(unsigned int value) : _buffer(nullptr), _length(0), _capacity(0) {
        char temp[12]; // Enough for uint32_t
        sprintf(temp, "%u", value);
        _length = strlen(temp);
        if (reserve(_length + 1)) {
            strcpy(_buffer, temp);
        }
    }

    // Constructor with long
    explicit Str(long value) : _buffer(nullptr), _length(0), _capacity(0) {
        char temp[12]; // Enough for int32_t
        sprintf(temp, "%ld", value);
        _length = strlen(temp);
        if (reserve(_length + 1)) {
            strcpy(_buffer, temp);
        }
    }

    // Constructor with unsigned long
    explicit Str(unsigned long value) : _buffer(nullptr), _length(0), _capacity(0) {
        // On 64-bit systems, unsigned long is 64 bits, so we need to handle it accordingly.
        // On 32-bit systems, unsigned long is 32 bits, so we can use uint32_t.
        // Typically esp32 vs test_desktop
        #if ULONG_MAX != UINT64_MAX
            char temp[12]; // Enough for uint32_t
        #else
            char temp[21]; // Enough for uint64_t
        #endif
        sprintf(temp, "%lu", value);
        _length = strlen(temp);
        if (reserve(_length + 1)) {
            strcpy(_buffer, temp);
        }
    }

    #if ULONG_MAX != UINT64_MAX
    // Constructor with uint64_t
    explicit Str(uint64_t value) : _buffer(nullptr), _length(0), _capacity(0) {
        // Allocate enough space for max uint64_t (20 digits + null terminator)
        if (reserve(21)) {
            uint64ToString(value, _buffer, 21);
            _length = strlen(_buffer);
        }
    }
    #endif

    // Constructor for integer with specified base (for hex, etc.)
    Str(unsigned char value, unsigned char base) : _buffer(nullptr), _length(0), _capacity(0) {
        char temp[9]; // Enough for uint8_t in any base
        if (base == 16) {
            sprintf(temp, "%02x", value);
        } else if (base == 10) {
            sprintf(temp, "%u", value);
        } else if (base == 8) {
            sprintf(temp, "%o", value);
        } else if (base == 2) {
            // Binary conversion
            temp[0] = '\0';
            for (int i = 7; i >= 0; i--) {
                strcat(temp, ((value >> i) & 1) ? "1" : "0");
            }
        } else {
            sprintf(temp, "%u", value); // Default to decimal
        }
        
        _length = strlen(temp);
        if (reserve(_length + 1)) {
            strcpy(_buffer, temp);
        }
    }

    // Destructor
    ~Str() {
        if (_buffer) free(_buffer);
    }

    // Assignment operators
    Str& operator=(const Str& other) {
        if (this != &other) {
            if (reserve(other._length + 1)) {
                strcpy(_buffer, other._buffer);
                _length = other._length;
            }
        }
        return *this;
    }

    Str& operator=(const char* cstr) {
        if (cstr) {
            size_t newLen = strlen(cstr);
            if (reserve(newLen + 1)) {
                strcpy(_buffer, cstr);
                _length = newLen;
            }
        } else {
            if (_buffer) _buffer[0] = '\0';
            _length = 0;
        }
        return *this;
    }

    // Concatenation operators
    Str& operator+=(const Str& other) {
        if (other._buffer && reserve(_length + other._length + 1)) {
            strcat(_buffer, other._buffer);
            _length += other._length;
        }
        return *this;
    }

    Str& operator+=(const char* cstr) {
        if (cstr) {
            size_t addLen = strlen(cstr);
            if (reserve(_length + addLen + 1)) {
                strcat(_buffer, cstr);
                _length += addLen;
            }
        }
        return *this;
    }

    Str& operator+=(char c) {
        if (reserve(_length + 2)) {
            _buffer[_length] = c;
            _buffer[_length + 1] = '\0';
            _length++;
        }
        return *this;
    }

    Str& operator+=(int num) {
        return *this += Str(num);
    }

    Str& operator+=(unsigned int num) {
        return *this += Str(num);
    }

    // Concatenation for other numeric types
    Str& operator+=(long num) {
        return *this += Str(num);
    }

    Str& operator+=(unsigned long num) {
        return *this += Str(num);
    }

    #if ULONG_MAX != UINT64_MAX
    Str& operator+=(uint64_t num) {
        return *this += Str(num);
    }
    #endif

    // String concatenation
    Str operator+(const Str& other) const {
        Str result(*this);
        result += other;
        return result;
    }

    Str operator+(const char* cstr) const {
        Str result(*this);
        result += cstr;
        return result;
    }

    Str operator+(char c) const {
        Str result(*this);
        result += c;
        return result;
    }

    // Access
    const char* c_str() const {
        return _buffer ? _buffer : "";
    }

    // Utility functions
    size_t length() const { return _length; }
    bool isEmpty() const { return _length == 0; }
    void clear() {
        if (_buffer) _buffer[0] = '\0';
        _length = 0;
    }

    // Comparison operators
    bool operator==(const Str& other) const {
        if (_buffer && other._buffer) {
            return strcmp(_buffer, other._buffer) == 0;
        }
        return false;
    }

    bool operator==(const char* cstr) const {
        if (_buffer && cstr) {
            return strcmp(_buffer, cstr) == 0;
        }
        return false;
    }

    bool operator!=(const Str& other) const {
        return !(*this == other);
    }

    bool operator!=(const char* cstr) const {
        return !(*this == cstr);
    }

    // Comparison operators for STL sorting and containers
    bool operator<(const Str& other) const {
        if (_buffer && other._buffer) {
            return strcmp(_buffer, other._buffer) < 0;
        }
        // If this string is null/empty and the other isn't, this is "less"
        return !_buffer && other._buffer;
    }

    bool operator<(const char* cstr) const {
        if (_buffer && cstr) {
            return strcmp(_buffer, cstr) < 0;
        }
        // If this string is null/empty and the other isn't, this is "less"
        return !_buffer && cstr && *cstr;
    }
    
    // We should also provide > operators for completeness
    bool operator>(const Str& other) const {
        return other < *this;
    }

    bool operator>(const char* cstr) const {
        if (_buffer && cstr) {
            return strcmp(_buffer, cstr) > 0;
        }
        // If this string is not null/empty and the other is, this is "greater"
        return _buffer && (!cstr || !*cstr);
    }
    
    // Less-than-or-equal and greater-than-or-equal operators
    bool operator<=(const Str& other) const {
        return !(*this > other);
    }
    
    bool operator<=(const char* cstr) const {
        return !(*this > cstr);
    }
    
    bool operator>=(const Str& other) const {
        return !(*this < other);
    }
    
    bool operator>=(const char* cstr) const {
        return !(*this < cstr);
    }

    // Substring method - creates a new Str that is a substring of this Str
    Str substring(size_t beginIndex) const {
        if (!_buffer || beginIndex >= _length) {
            return Str(); // Return empty string if out of bounds
        }

        return Str(_buffer + beginIndex);
    }

    // Substring method - creates a new Str that is a substring of this Str with specified end index
    Str substring(size_t beginIndex, size_t endIndex) const {
        if (!_buffer || beginIndex >= _length) {
            return Str(); // Return empty string if out of bounds
        }

        // Adjust endIndex if it's beyond the string length
        if (endIndex > _length) {
            endIndex = _length;
        }

        // Make sure beginIndex is not greater than endIndex
        if (beginIndex >= endIndex) {
            return Str(); // Return empty string
        }

        // Calculate length of substring
        size_t subLen = endIndex - beginIndex;
        
        // Create a temporary buffer for the substring
        char* temp = (char*)malloc(subLen + 1);
        if (!temp) return Str(); // Return empty string on allocation failure
        
        // Copy the substring
        memcpy(temp, _buffer + beginIndex, subLen);
        temp[subLen] = '\0';
        
        // Create a new Str from the temp buffer
        Str result(temp);
        
        // Free the temporary buffer
        free(temp);
        
        return result;
    }

    // indexOf method - finds the first occurrence of a character in the string
    int indexOf(char c) const {
        if (!_buffer || _length == 0) {
            return -1; // Not found in empty string
        }
        
        // Search for the character in the buffer
        for (size_t i = 0; i < _length; i++) {
            if (_buffer[i] == c) {
                return static_cast<int>(i);
            }
        }
        
        return -1; // Not found
    }
    
    // indexOf method with starting position
    int indexOf(char c, size_t fromIndex) const {
        if (!_buffer || _length == 0 || fromIndex >= _length) {
            return -1; // Not found in empty string or out of bounds
        }
        
        // Search for the character in the buffer starting from fromIndex
        for (size_t i = fromIndex; i < _length; i++) {
            if (_buffer[i] == c) {
                return static_cast<int>(i);
            }
        }
        
        return -1; // Not found
    }

    // indexOf for searching a substring
    int indexOf(const char* substr) const {
        if (!_buffer || !substr || _length == 0) {
            return -1; // Not found in empty string or invalid substring
        }
        
        const char* found = strstr(_buffer, substr);
        if (found) {
            return static_cast<int>(found - _buffer);
        }
        
        return -1; // Not found
    }

    // indexOf for searching a substring with starting position
    int indexOf(const char* substr, size_t fromIndex) const {
        if (!_buffer || !substr || _length == 0 || fromIndex >= _length) {
            return -1; // Not found in empty string or out of bounds
        }
        
        const char* found = strstr(_buffer + fromIndex, substr);
        if (found) {
            return static_cast<int>(found - _buffer);
        }
        
        return -1; // Not found
    }
    
    // lastIndexOf method - finds the last occurrence of a character in the string
    int lastIndexOf(char c) const {
        if (!_buffer || _length == 0) {
            return -1; // Not found in empty string
        }
        
        // Search backwards for the character in the buffer
        for (int i = _length - 1; i >= 0; i--) {
            if (_buffer[i] == c) {
                return i;
            }
        }
        
        return -1; // Not found
    }
    
    // lastIndexOf method with starting position (search backwards from this position)
    int lastIndexOf(char c, size_t fromIndex) const {
        if (!_buffer || _length == 0) {
            return -1; // Not found in empty string
        }
        
        // Make sure fromIndex is within bounds
        if (fromIndex >= _length) {
            fromIndex = _length - 1;
        }
        
        // Search backwards from fromIndex
        for (int i = fromIndex; i >= 0; i--) {
            if (_buffer[i] == c) {
                return i;
            }
        }
        
        return -1; // Not found
    }

    // Convert string to integer
    int toInt() const {
        if (!_buffer || _length == 0) {
            return 0;
        }
        
        return atoi(_buffer);
    }
    
    // Check if string ends with the specified suffix
    bool endsWith(const char* suffix) const {
        if (!_buffer || !suffix) {
            return false;
        }
        
        size_t suffixLen = strlen(suffix);
        if (suffixLen > _length) {
            return false;
        }
        
        return strcmp(_buffer + (_length - suffixLen), suffix) == 0;
    }
    
    // Trim whitespace from both ends of the string
    void trim() {
        if (!_buffer || _length == 0) {
            return;
        }
        
        // Find first non-whitespace character from beginning
        size_t start = 0;
        while (start < _length && isspace(_buffer[start])) {
            start++;
        }
        
        // If the string is all whitespace
        if (start == _length) {
            clear();
            return;
        }
        
        // Find last non-whitespace character from end
        size_t end = _length - 1;
        while (end > start && isspace(_buffer[end])) {
            end--;
        }
        
        // Calculate new length
        size_t newLen = end - start + 1;
        
        // If we need to trim from the beginning, shift the content
        if (start > 0) {
            memmove(_buffer, _buffer + start, newLen);
        }
        
        // Terminate the string at the new end
        _buffer[newLen] = '\0';
        _length = newLen;
    }
    
    // Return a new trimmed copy without modifying this instance
    Str trimmed() const {
        Str result(*this);
        result.trim();
        return result;
    }

    // Replace all occurrences of a substring with another substring
    void replace(const char* from, const char* to) {
        if (!_buffer || !from || !to) {
            return;
        }
        
        size_t fromLen = strlen(from);
        if (fromLen == 0) {
            return; // Can't replace an empty string
        }
        
        size_t toLen = strlen(to);
        
        // Find first occurrence
        char* pos = strstr(_buffer, from);
        if (!pos) {
            return; // Not found
        }
        
        // Create a new buffer for the result
        // We'll allocate more than needed to avoid frequent reallocation
        size_t maxNewLen = _length + (_length / fromLen + 1) * (toLen > fromLen ? (toLen - fromLen) : 0) + 1;
        char* newBuf = (char*)malloc(maxNewLen);
        if (!newBuf) {
            return; // Memory allocation failed
        }
        
        char* newPos = newBuf;
        char* oldPos = _buffer;
        
        // Copy parts before the first occurrence
        size_t copyLen = pos - _buffer;
        memcpy(newPos, oldPos, copyLen);
        newPos += copyLen;
        oldPos = pos + fromLen;
        
        // Copy the replacement
        memcpy(newPos, to, toLen);
        newPos += toLen;
        
        // Find and replace remaining occurrences
        while ((pos = strstr(oldPos, from)) != nullptr) {
            // Copy part between occurrences
            copyLen = pos - oldPos;
            memcpy(newPos, oldPos, copyLen);
            newPos += copyLen;
            oldPos = pos + fromLen;
            
            // Copy the replacement
            memcpy(newPos, to, toLen);
            newPos += toLen;
        }
        
        // Copy remaining part
        strcpy(newPos, oldPos);
        
        // Calculate new length
        size_t newLen = strlen(newBuf);
        
        // Free old buffer and assign new one
        free(_buffer);
        _buffer = newBuf;
        _length = newLen;
        _capacity = maxNewLen;
    }

    Str& append(const char *bytes, size_t len) {
        if (len == 0) return *this;
        if (reserve(_length + len + 1)) {
            memcpy(_buffer + _length, bytes, len);
            _length += len;
            _buffer[_length] = '\0'; // Null-terminate
        }
        return *this;
    }
};

// Non-member operator overload for const char* + Str
inline Str operator+(const char* lhs, const Str& rhs) {
    Str result(lhs);
    result += rhs;
    return result;
}

} // namespace zap