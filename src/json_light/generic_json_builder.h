#pragma once

#include "zap_str.h" // Include your existing string class
#include <vector>
#include <cstdio> // For sprintf used in escaping control characters
#include <cstdarg> 

namespace zap {

// Forward declaration of buffer strategy classes
class JsonBuilderDynamicBuffer;
class JsonBuilderFixedBuffer;

/**
 * A generic JSON builder that uses a buffer strategy pattern
 */
template <typename BufferStrategy>
class GenericJsonBuilder {
private:
    BufferStrategy _buffer;
    bool _firstItem;
    bool _inObject;
    std::vector<bool> _objectStack; // Stack to track firstItem state for nested objects

    // --- Helper function to append an escaped string ---
    void appendEscaped(const char* str) {
        if (!str) return;
        while (*str) {
            char c = *str++;
            switch (c) {
                case '"':  _buffer.append("\\\""); break;
                case '\\': _buffer.append("\\\\"); break;
                case '\b': _buffer.append("\\b"); break;
                case '\f': _buffer.append("\\f"); break;
                case '\n': _buffer.append("\\n"); break;
                case '\r': _buffer.append("\\r"); break;
                case '\t': _buffer.append("\\t"); break;
                default:
                    // Check for control characters (U+0000 to U+001F)
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char hex_buf[7]; // "\\uXXXX\0"
                        sprintf(hex_buf, "\\u%04x", static_cast<unsigned char>(c));
                        _buffer.append(hex_buf);
                    } else {
                        // Regular character
                        _buffer.append(c);
                    }
                    break;
            }
        }
    }
     // Overload for zap::Str
    void appendEscaped(const zap::Str& str) {
        appendEscaped(str.c_str()); // Delegate to C-string version
    }
    // --- End Helper ---


public:
    // Constructor - forwards arguments to the buffer strategy
    template <typename... Args>
    GenericJsonBuilder(Args&&... args)
        : _buffer(std::forward<Args>(args)...),
          _firstItem(true),
          _inObject(false) {}

    // Start a new object
    GenericJsonBuilder& beginObject() {
        if (!_firstItem && _inObject) _buffer.append(','); // Add comma if not first item *in current object*
        _buffer.append('{');
        _objectStack.push_back(_firstItem); // Save current state
        _firstItem = true;
        _inObject = true;
        return *this;
    }

    // Start a nested object with a key
    GenericJsonBuilder& beginObject(const char* key) {
        if (!_firstItem) _buffer.append(',');
         // Keys are typically not escaped unless they contain special chars,
         // assuming simple keys here. Could add appendEscaped(key) if needed.
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":{");
        _objectStack.push_back(_firstItem); // Save current state
        _firstItem = true;
        _inObject = true;
        return *this;
    }

    // End the current object
    GenericJsonBuilder& endObject() {
        _buffer.append('}');
        if (!_objectStack.empty()) {
            _firstItem = _objectStack.back(); // Restore previous state
            _objectStack.pop_back();
            // Determine _inObject based on whether the stack is now empty
            _inObject = !_objectStack.empty();
            _firstItem = false; // After closing an object, we've added an item to the parent context
        } else {
             _inObject = false; // No longer in any object
             _firstItem = false; // Have definitely added the root object
        }

        return *this;
    }

    // --- Modified add methods for strings ---
    GenericJsonBuilder& add(const char* key, const char* value) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key); // Assuming key doesn't need escaping
        _buffer.append("\":\"");
        // --- Use helper function ---
        appendEscaped(value);
        // -------------------------
        _buffer.append('"');
        _firstItem = false;
        return *this;
    }

    GenericJsonBuilder& add(const char* key, const zap::Str& value) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key); // Assuming key doesn't need escaping
        _buffer.append("\":\"");
        // --- Use helper function ---
        appendEscaped(value); // Use overload for zap::Str
        // -------------------------
        _buffer.append('"');
        _firstItem = false;
        return *this;
    }
    // --- End Modified add methods ---


    GenericJsonBuilder& add(const char* key, int value) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":");
        _buffer.append(value);
        _firstItem = false;
        return *this;
    }

    GenericJsonBuilder& add(const char* key, uint32_t value) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":");
        _buffer.append(value);
        _firstItem = false;
        return *this;
    }

    GenericJsonBuilder& add(const char* key, uint64_t value) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":");
        _buffer.append(value);
        _firstItem = false;
        return *this;
    }

    GenericJsonBuilder& add(const char* key, bool value) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":");
        _buffer.append(value ? "true" : "false");
        _firstItem = false;
        return *this;
    }

    // Adds raw binary data as a hex string (already safe)
    GenericJsonBuilder& add(const char* key, uint8_t* data, size_t size) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":\"");
        for (size_t i = 0; i < size; i++) {
            char hex[3];
            sprintf(hex, "%02x", data[i]);
            _buffer.append(hex);
        }
        _buffer.append('"');
        _firstItem = false;
        return *this;
    }

    GenericJsonBuilder& add(const char* key, float value) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":");
        _buffer.append(value); // Note: Precision issues might occur. Consider snprintf for control.
        _firstItem = false;
        return *this;
    }

    // --- Modified addArray methods ---
    GenericJsonBuilder& addArray(const char* key, const std::vector<zap::Str>& values) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":["); // Start array
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) _buffer.append(','); // Comma between elements
            _buffer.append('"'); // Start string element
            // --- Use helper function ---
            appendEscaped(values[i]);
            // -------------------------
            _buffer.append('"'); // End string element
        }
        _buffer.append("]"); // End array
        _firstItem = false;
        return *this;
    }

    GenericJsonBuilder& addArray(const char* key, const char* const* values, size_t count) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":["); // Start array
        for (size_t i = 0; i < count; i++) {
             if (i > 0) _buffer.append(','); // Comma between elements
            _buffer.append('"'); // Start string element
            // --- Use helper function ---
            appendEscaped(values[i]);
            // -------------------------
            _buffer.append('"'); // End string element
        }
        _buffer.append("]"); // End array
        _firstItem = false;
        return *this;
    }
     // --- End Modified addArray methods ---


    // End all objects and get the result
     auto end() -> decltype(_buffer.get()) {
        while (_inObject && !_objectStack.empty()) {
            _buffer.append('}');
            _objectStack.pop_back();
        }
        _inObject = false;
        return _buffer.get();
     }

    // Clear the buffer and reset state
    void clear() {
        _buffer.clear();
        _firstItem = true;
        _inObject = false;
        _objectStack.clear();
    }

    // Delegate to buffer for status
    bool hasOverflow() const { return _buffer.hasOverflow(); }
    size_t length() const { return _buffer.length(); }
};

// ...(JsonBuilderDynamicBuffer and JsonBuilderFixedBuffer remain the same)...

/**
 * Dynamic buffer strategy that uses zap::Str
 */
class JsonBuilderDynamicBuffer {
private:
    zap::Str _str;

public:
    JsonBuilderDynamicBuffer() = default;

    void append(const char* s) { _str += s; }
    void append(char c) { _str += c; }
    void append(int value) { _str += value; }
    void append(uint32_t value) { _str += value; }
    void append(uint64_t value) { _str += value; }
    void append(float value) { _str += zap::Str(value); } // Assuming zap::Str has float constructor

    const zap::Str& get() const { return _str; }
    void clear() { _str.clear(); }
    bool hasOverflow() const { return false; } // Dynamic buffer never overflows
    size_t length() const { return _str.length(); }
};

/**
 * Fixed buffer strategy that works with a pre-allocated buffer
 */
class JsonBuilderFixedBuffer {
private:
    char* _buffer;
    size_t _length;
    size_t _capacity;
    bool _overflow;

public:
    JsonBuilderFixedBuffer(char* buffer, size_t capacity)
        : _buffer(buffer), _length(0), _capacity(capacity), _overflow(false) {
        if (_buffer && _capacity > 0) {
            _buffer[0] = '\0';
        } else {
            // Handle null buffer or zero capacity case
            _buffer = nullptr; // Ensure buffer pointer is safe
            _capacity = 0;
            _overflow = true;
        }
    }

    void append(const char* s) {
        if (_overflow || !s) return; // Check for null input string

        size_t len = strlen(s);
        // Need space for string AND null terminator
        if (_length + len >= _capacity) { // Use >= because _length is 0-based index, capacity is size
            _overflow = true;
             // Optionally, fill remaining buffer space if desired, but usually just stop
            if (_buffer && _length < _capacity) {
                 _buffer[_length] = '\0'; // Ensure null termination even on overflow
            }
            return;
        }

        // Use memcpy for potentially better performance than strcpy
        memcpy(_buffer + _length, s, len);
        _length += len;
        _buffer[_length] = '\0'; // Ensure null termination
    }

    void append(char c) {
        if (_overflow) return;

        // Need space for char AND null terminator
        if (_length + 1 >= _capacity) { // Use >=
            _overflow = true;
            if (_buffer && _length < _capacity) {
                 _buffer[_length] = '\0';
            }
            return;
        }

        _buffer[_length++] = c;
        _buffer[_length] = '\0';
    }

    // Helper for appending formatted numbers to fixed buffer
    void appendFormatted(const char* format, ...) {
         if (_overflow) return;

         va_list args;
         va_start(args, format);

         // Calculate remaining space (leaving 1 for null terminator)
         size_t remaining_capacity = (_capacity > _length) ? (_capacity - _length) : 0;

         // Use vsnprintf for safe printing into the buffer
         int written = vsnprintf(_buffer + _length, remaining_capacity, format, args);

         va_end(args);

         if (written < 0 || static_cast<size_t>(written) >= remaining_capacity) {
             // Encoding error or buffer full
             _overflow = true;
             if (_length < _capacity) {
                 _buffer[_length] = '\0'; // Ensure null termination at the current point
             }
         } else {
             // Success
             _length += written;
             // vsnprintf should have added the null terminator
         }
    }


    void append(int value) {
        appendFormatted("%d", value);
    }

    void append(uint32_t value) {
        appendFormatted("%u", value);
    }

    void append(uint64_t value) {
         // Check which format specifier is correct for uint64_t on your platform
         // C99 standard uses %llu
        appendFormatted("%llu", (unsigned long long)value);
    }

    void append(float value) {
         // Use %g for general format, but consider precision needs.
         // %f or %e might be better depending on requirements.
         // Adjust precision as needed, e.g., "%.7g"
         appendFormatted("%g", value);
    }

    const char* get() const { return _buffer; }

    void clear() {
        if (_buffer && _capacity > 0) {
            _buffer[0] = '\0';
            _length = 0;
            _overflow = false;
        } else {
            // Ensure state is consistent even if buffer was invalid initially
            _length = 0;
            _overflow = true; // Cannot clear an invalid/overflowed buffer state fully
        }
    }

    bool hasOverflow() const { return _overflow; }
    size_t length() const { return _length; }
    size_t capacity() const { return _capacity; }
};


} // namespace zap