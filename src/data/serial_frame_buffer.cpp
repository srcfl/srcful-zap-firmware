#include "serial_frame_buffer.h"
#include "debug.h"
#include <vector> // Include vector
#include <utility> // Include pair

SerialFrameBuffer::SerialFrameBuffer(
    size_t bufferSize,
    uint8_t startDelimiter,
    uint8_t endDelimiter,
    unsigned long interFrameTimeout
) : _circularBuffer(bufferSize),
    _frameDetector(interFrameTimeout), // Correctly initialize FrameDetector with a vector
    _currentFrameSize(0),
    _currentFrameStartIndex(0),
    _frameCallback(nullptr) {

    Debug::setMeterDataBuffer(&_circularBuffer);

    std::vector<FrameDelimiterInfo> delims = {
        FrameDelimiterInfo('/', '!', 0, true), // Start and end delimiter for ascii
        FrameDelimiterInfo(0x7e, 0x7e, 1, false) // Start and end delimiter for aidon
    };
    _frameDetector.setFrameDelimiters(delims);
    
    clear(0); // Passing 0 as placeholder time
}

SerialFrameBuffer::~SerialFrameBuffer() {
    // CircularBuffer and FrameDetector will clean up in their own destructors
}

bool SerialFrameBuffer::addByte(uint8_t byte, unsigned long currentTime) {
    // Add a single byte and treat it as a chunk of size 1
    return addData(&byte, 1, currentTime);
}

bool SerialFrameBuffer::addData(const uint8_t* data, size_t length, unsigned long currentTime) {
    if (!data || length == 0) {
        return false;
    }
    
    // Add all bytes to the circular buffer at once
    // TODO: as we are adding a chunk we can do this more efficiently than adding byte by byte
    for (size_t i = 0; i < length; i++) {
        if (!_circularBuffer.addByte(data[i], currentTime)) {
            // Buffer overflow
            return false;
        }
    }
    
    return true;
}

bool SerialFrameBuffer::processBufferForFrames(unsigned long currentTime) {
    // Process all data in the buffer to find complete frames
    FrameInfo frameInfo;
    if (_frameDetector.detect(_circularBuffer, currentTime, frameInfo) && frameInfo.complete) {
        // Update current frame information for IFrameData interface
        updateCurrentFrame(frameInfo);
        
        // Frame detected, process it if we have a callback
        if (processDetectedFrame()) {
            // Now that the frame has been successfully processed,
            // advance the buffer's read index
            _circularBuffer.advanceReadIndex(calculateReadAdvance(frameInfo));
            return true;
        }
    }
    return false;
}

void SerialFrameBuffer::clear(unsigned long currentTime) {
    // Clear the circular buffer
    _circularBuffer.clear(currentTime);
    
    // Reset the frame detector state
    _frameDetector.reset();
    
    // Reset current frame data
    _currentFrameSize = 0;
    _currentFrameStartIndex = 0;
}

bool SerialFrameBuffer::processDetectedFrame() {
    if (!_frameCallback) {
        return false;
    }
    
    // Call the callback with the frame data (this buffer implements IFrameData)
    return _frameCallback(*this);
}

size_t SerialFrameBuffer::calculateReadAdvance(const FrameInfo& frameInfo) const {
    // Calculate how many bytes to advance the read index based on frame info
    size_t bufferSize = _circularBuffer.getBufferSize();
    size_t readIndex = _circularBuffer.getReadIndex();
    size_t bufferUsed = _circularBuffer.available();
    
    // Calculate position in buffer to advance read index to
    size_t readAdvance = (frameInfo.endIndex - readIndex + 1 + bufferSize) % bufferSize;
    
    // Safety check to ensure we don't advance beyond available data
    if (readAdvance > bufferUsed) {
        readAdvance = bufferUsed;
    }
    
    return readAdvance;
}

void SerialFrameBuffer::updateCurrentFrame(const FrameInfo& frameInfo) {
    _currentFrameSize = frameInfo.size;
    _currentFrameStartIndex = frameInfo.startIndex;
}