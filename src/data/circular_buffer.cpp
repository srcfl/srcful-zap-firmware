#include "circular_buffer.h"

CircularBuffer::CircularBuffer(size_t bufferSize)
    : _bufferSize(bufferSize),
      _writeIndex(0),
      _readIndex(0),
      _bufferUsed(0),
      _lastByteTime(0),
      _overflowCount(0) {
    
    // Allocate the buffer
    _buffer = new uint8_t[_bufferSize];
    // Note: We don't handle allocation failure here - it will result in 
    // a nullptr which is checked in all relevant methods
}

CircularBuffer::~CircularBuffer() {
    if (_buffer != nullptr) {
        delete[] _buffer;
        _buffer = nullptr;
    }
}

bool CircularBuffer::addByte(uint8_t byte, unsigned long currentTime) {
    if (_buffer == nullptr) {
        return false;
    }
    
    // Update timestamp of last received byte
    _lastByteTime = currentTime;
    
    // Add byte to circular buffer
    _buffer[_writeIndex] = byte;
    _writeIndex = (_writeIndex + 1) % _bufferSize;
    
    // Update buffer usage
    if (_bufferUsed < _bufferSize) {
        _bufferUsed++;
    } else {
        // Buffer full, move read index to keep space
        _readIndex = (_readIndex + 1) % _bufferSize;
        _overflowCount++;
    }
    
    return true;
}

void CircularBuffer::clear(unsigned long currentTime) {
    if (_buffer == nullptr) {
        return;
    }
    
    // Reset buffer indices and state
    _writeIndex = 0;
    _readIndex = 0;
    _bufferUsed = 0;
    _lastByteTime = currentTime;
}

uint8_t CircularBuffer::getByte(size_t index) const {
    if (_buffer == nullptr || index >= _bufferUsed) {
        return 0;
    }
    
    size_t pos = (_readIndex + index) % _bufferSize;
    return _buffer[pos];
}

uint8_t CircularBuffer::getByteAt(size_t position) const {
    if (_buffer == nullptr || position >= _bufferSize) {
        return 0;
    }
    
    return _buffer[position];
}

void CircularBuffer::advanceReadIndex(size_t count) {
    if (count > _bufferUsed) {
        count = _bufferUsed;
    }
    
    _readIndex = (_readIndex + count) % _bufferSize;
    _bufferUsed -= count;
}