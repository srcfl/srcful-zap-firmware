#include "serial_frame_buffer.h"
#include <Arduino.h>

SerialFrameBuffer::SerialFrameBuffer(
    size_t bufferSize,
    uint8_t startDelimiter,
    uint8_t endDelimiter,
    unsigned long interFrameTimeout
) : _bufferSize(bufferSize),
    _writeIndex(0),
    _readIndex(0),
    _bufferUsed(0),
    _startDelimiter(startDelimiter),
    _endDelimiter(endDelimiter),
    _frameInProgress(false),
    _frameStartIndex(0),
    _lastByteTime(0),
    _interFrameTimeout(interFrameTimeout),
    _frameCount(0),
    _overflowCount(0),
    _frameCallback(nullptr) {
    
    // Allocate buffer with null check
    try {
        _buffer = new uint8_t[_bufferSize];
        if (_buffer == nullptr) {
            Serial.println("ERROR: Failed to allocate SerialFrameBuffer");
        } else {
            clear();
        }
    } catch (...) {
        Serial.println("EXCEPTION: Failed to allocate SerialFrameBuffer");
        _buffer = nullptr;
    }
}

SerialFrameBuffer::~SerialFrameBuffer() {
    if (_buffer != nullptr) {
        delete[] _buffer;
        _buffer = nullptr;
    }
}

bool SerialFrameBuffer::addByte(uint8_t byte) {
    return addByte(byte, millis());
}

bool SerialFrameBuffer::addByte(uint8_t byte, const unsigned long currentTime) {
    if (_buffer == nullptr) {
        return false;
    }
    
    // Check for frame timeout
    if (_frameInProgress && currentTime - _lastByteTime > _interFrameTimeout) {
        Serial.printf("Frame timeout occurred after %lu ms, resetting frame state\n", currentTime - _lastByteTime);
        _frameInProgress = false;
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
        
        // If we're tracking a frame and lose its start due to overflow, reset frame state
        if (_frameInProgress && _readIndex > _frameStartIndex) {
            Serial.println("WARNING: Buffer overflow caused loss of frame start, resetting frame state");
            _frameInProgress = false;
        }
    }

    // Detect end of frame we do this before detecting the start so we can use the same delimiter.
    bool frameProcessed = false;
    if (_frameInProgress && byte == _endDelimiter) {
        // Try to process complete frames
        frameProcessed = processCompleteFrames();
    }
    
    // Detect start of frame
    if (byte == _startDelimiter) {
        _frameInProgress = true;
        _frameStartIndex = (_writeIndex - 1 + _bufferSize) % _bufferSize; // Adjust to point to the start char
    }
    
    
    
    return frameProcessed;
}

bool SerialFrameBuffer::addData(const uint8_t* data, size_t length) {
    if (_buffer == nullptr || data == nullptr) {
        return false;
    }
    
    bool frameProcessed = false;
    
    // Add each byte and track if any frames were processed
    for (size_t i = 0; i < length; i++) {
        if (addByte(data[i])) {
            frameProcessed = true;
        }
    }
    
    return frameProcessed;
}

bool SerialFrameBuffer::update() {
    if (_buffer == nullptr || _bufferUsed == 0) {
        return false;
    }
    
    // Check if current frame timed out
    unsigned long currentTime = millis();
    if (_frameInProgress && currentTime - _lastByteTime > _interFrameTimeout) {
        Serial.printf("Frame timeout occurred during update after %lu ms\n", currentTime - _lastByteTime);
        _frameInProgress = false;
        
        // We could try to process what we have, but for now we'll discard incomplete frames
    }
    
    // Process any complete frames that might be in the buffer
    return processCompleteFrames();
}

void SerialFrameBuffer::clear() {
    if (_buffer == nullptr) {
        return;
    }
    
    // Reset buffer indices and state
    _writeIndex = 0;
    _readIndex = 0;
    _bufferUsed = 0;
    _frameInProgress = false;
    _frameStartIndex = 0;
    _lastByteTime = millis();
}

bool SerialFrameBuffer::processCompleteFrames() {
    if (_buffer == nullptr || _bufferUsed == 0 || !_frameCallback) {
        return false;
    }
    
    bool anyFramesProcessed = false;
    
    // Look for complete frames in buffer
    uint8_t* frameData = nullptr;
    size_t frameSize = 0;
    
    // Try to extract a complete frame
    while (extractCompleteFrame(&frameData, &frameSize)) {
        // Call the callback with the frame data
        bool processed = _frameCallback(frameData, frameSize);
        
        if (processed) {
            _frameCount++;
            anyFramesProcessed = true;
        }
    }
    
    return anyFramesProcessed;
}

bool SerialFrameBuffer::findNextFrameStart() {
    if (_buffer == nullptr || _bufferUsed == 0) {
        return false;
    }
    
    // Look for the start delimiter from the current read position
    size_t searchPos = _readIndex;
    size_t bytesSearched = 0;
    
    while (bytesSearched < _bufferUsed) {
        if (_buffer[searchPos] == _startDelimiter) {
            // Found start delimiter
            _frameStartIndex = searchPos;
            _frameInProgress = true;
            
            // Move read position to the start of the frame
            if (searchPos != _readIndex) {
                size_t skipCount = (searchPos - _readIndex + _bufferSize) % _bufferSize;
                advanceReadIndex(skipCount);
                Serial.printf("Skipped %zu bytes to find frame start\n", skipCount);
            }
            
            return true;
        }
        
        // Move to next position in circular buffer
        searchPos = (searchPos + 1) % _bufferSize;
        bytesSearched++;
    }
    
    return false;
}

bool SerialFrameBuffer::extractCompleteFrame(uint8_t** frameData, size_t* frameSize) {
    if (_buffer == nullptr || _bufferUsed < 2 || !_frameInProgress) {
        return false;
    }
    
    // Start our search at the frame start index
    size_t searchPos = _frameStartIndex;
    bool foundStart = false;
    size_t bytesSearched = 0;
    size_t startPos = 0;
    size_t endPos = 0;
    
    // Search for a complete frame (start to end delimiter)
    while (bytesSearched < _bufferUsed) {
        uint8_t current = _buffer[searchPos];
        
        if (!foundStart && current == _startDelimiter) {
            foundStart = true;
            startPos = searchPos;
        } else if (foundStart && current == _endDelimiter) {
            // Found end delimiter
            endPos = searchPos;
            
            // Calculate frame size (including delimiters)
            size_t frameLength;
            if (endPos >= startPos) {
                frameLength = endPos - startPos + 1;
            } else {
                frameLength = _bufferSize - startPos + endPos + 1;
            }
            
            // Only proceed if we have a reasonable frame size
            if (frameLength >= 2) {
                // Set output parameters
                static uint8_t tempFrame[2048]; // Static buffer for returning frame data
                
                // Copy frame data to a contiguous buffer
                size_t tempIndex = 0;
                size_t pos = startPos;
                
                for (size_t i = 0; i < frameLength && tempIndex < sizeof(tempFrame); i++) {
                    tempFrame[tempIndex++] = _buffer[pos];
                    pos = (pos + 1) % _bufferSize;
                }
                
                *frameData = tempFrame;
                *frameSize = tempIndex;
                
                // Advance read index past this frame
                // We'll start the next frame search after the end delimiter
                _readIndex = (endPos + 1) % _bufferSize;
                _bufferUsed -= frameLength;
                
                // Reset frame tracking state
                _frameInProgress = false;
                
                return true;
            }
        }
        
        // Move to next position in circular buffer
        searchPos = (searchPos + 1) % _bufferSize;
        bytesSearched++;
    }
    
    // No complete frame found
    return false;
}

void SerialFrameBuffer::advanceReadIndex(size_t count) {
    if (count > _bufferUsed) {
        count = _bufferUsed;
    }
    
    _readIndex = (_readIndex + count) % _bufferSize;
    _bufferUsed -= count;
}