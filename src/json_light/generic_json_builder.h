#pragma once

#include "zap_str.h" // Include your existing string class
#include <vector>

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

public:
    // Constructor - forwards arguments to the buffer strategy
    template <typename... Args>
    GenericJsonBuilder(Args&&... args) 
        : _buffer(std::forward<Args>(args)...), 
          _firstItem(true), 
          _inObject(false) {}
    
    // Start a new object
    GenericJsonBuilder& beginObject() {
        _buffer.append('{');
        _objectStack.push_back(_firstItem); // Save current state
        _firstItem = true;
        _inObject = true;
        return *this;
    }
    
    // Start a nested object with a key
    GenericJsonBuilder& beginObject(const char* key) {
        if (!_firstItem) _buffer.append(',');
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
            _firstItem = false; // After closing an object, we've added an item to the parent
        }
        return *this;
    }
    
    // Add a key-value pair
    GenericJsonBuilder& add(const char* key, const char* value) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":\"");
        _buffer.append(value);
        _buffer.append('"');
        _firstItem = false;
        return *this;
    }
    
    GenericJsonBuilder& add(const char* key, const zap::Str& value) {
        return add(key, value.c_str());
    }
    
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
    
    GenericJsonBuilder& add(const char* key, uint8_t* data, size_t size) {
        if (!_firstItem) _buffer.append(',');
        
        // add data as hex string
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
        _buffer.append(value);
        _firstItem = false;
        return *this;
    }
    
    GenericJsonBuilder& addArray(const char* key, const std::vector<zap::Str>& values) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":[\"");
        for (size_t i = 0; i < values.size(); i++) {
            if (i > 0) _buffer.append("\",\"");
            _buffer.append(values[i].c_str());
        }
        _buffer.append("\"]");
        _firstItem = false;
        return *this;
    }
    
    GenericJsonBuilder& addArray(const char* key, const char* const* values, size_t count) {
        if (!_firstItem) _buffer.append(',');
        _buffer.append('"');
        _buffer.append(key);
        _buffer.append("\":[\"");
        for (size_t i = 0; i < count; i++) {
            if (i > 0) _buffer.append("\",\"");
            _buffer.append(values[i]);
        }
        _buffer.append("\"]");
        _firstItem = false;
        return *this;
    }
    
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
    void append(float value) { _str += zap::Str(value); }
    
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
            _overflow = true;
        }
    }
    
    void append(const char* s) {
        if (_overflow) return;
        
        size_t len = strlen(s);
        if (_length + len >= _capacity) {
            _overflow = true;
            return;
        }
        
        strcpy(_buffer + _length, s);
        _length += len;
    }
    
    void append(char c) {
        if (_overflow) return;
        
        if (_length + 1 >= _capacity) {
            _overflow = true;
            return;
        }
        
        _buffer[_length++] = c;
        _buffer[_length] = '\0';
    }
    
    void append(int value) {
        char temp[12];
        sprintf(temp, "%d", value);
        append(temp);
    }
    
    void append(uint32_t value) {
        char temp[12];
        sprintf(temp, "%u", value);
        append(temp);
    }
    
    void append(uint64_t value) {
        char temp[21];
        sprintf(temp, "%llu", (unsigned long long)value);
        append(temp);
    }
    
    void append(float value) {
        char temp[32];
        sprintf(temp, "%g", value);
        append(temp);
    }
    
    const char* get() const { return _buffer; }
    
    void clear() {
        if (_buffer && _capacity > 0) {
            _buffer[0] = '\0';
            _length = 0;
            _overflow = false;
        }
    }
    
    bool hasOverflow() const { return _overflow; }
    size_t length() const { return _length; }
    size_t capacity() const { return _capacity; }
};



} // namespace zap