#include "frame_detector.h"

FrameDetector::FrameDetector(
    uint8_t startDelimiter,
    uint8_t endDelimiter,
    unsigned long interFrameTimeout
) : _startDelimiter(startDelimiter),
    _endDelimiter(endDelimiter),
    _frameInProgress(false),
    _frameStartIndex(0),
    _interFrameTimeout(interFrameTimeout),
    _frameCount(0) {
    // Nothing else to initialize
}



void FrameDetector::reset() {
    _frameInProgress = false;
    _frameStartIndex = 0;
}

bool FrameDetector::findNextFrameStart(const CircularBuffer& buffer, size_t& startPos) {
    if (buffer.available() == 0) {
        return false;
    }
    
    // Look for the start delimiter from the current read position
    size_t bytesSearched = 0;
    size_t bufferUsed = buffer.available();
    
    while (bytesSearched < bufferUsed) {
        uint8_t current = buffer.getByte(bytesSearched);
        if (current == _startDelimiter) {
            // Found start delimiter
            startPos = (buffer.getReadIndex() + bytesSearched) % buffer.getBufferSize();
            _frameInProgress = true;
            
            // No longer advancing the buffer's read index here
            // We just report the position of the start marker
            return true;
        }
        bytesSearched++;
    }
    
    return false;
}

bool FrameDetector::extractCompleteFrame(const CircularBuffer& buffer, FrameInfo& frameInfo) {
    if (buffer.available() < 2 || !_frameInProgress) {
        return false;
    }
    
    // Start our search from the frame start
    size_t searchPos = _frameStartIndex;
    bool foundStart = false;
    size_t bytesSearched = 0;
    size_t startPos = 0;
    size_t endPos = 0;
    size_t bufferSize = buffer.getBufferSize();
    size_t bufferUsed = buffer.available();
    
    // Search for a complete frame (start to end delimiter)
    while (bytesSearched < bufferUsed) {
        uint8_t current = buffer.getByteAt(searchPos);
        
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
                frameLength = bufferSize - startPos + endPos + 1;
            }
            
            // Only proceed if we have a reasonable frame size
            if (frameLength >= 2) {
                // Fill the frame info structure
                frameInfo.startIndex = startPos;
                frameInfo.endIndex = endPos;
                frameInfo.size = frameLength;
                frameInfo.complete = true;
                
                // Reset frame tracking state
                _frameInProgress = false;
                
                // Increment frame counter
                _frameCount++;
                
                return true;
            }
        }
        
        // Move to next position in circular buffer
        searchPos = (searchPos + 1) % bufferSize;
        bytesSearched++;
    }
    
    // No complete frame found
    frameInfo.complete = false;
    return false;
}

bool FrameDetector::detect(
    const CircularBuffer& buffer,
    unsigned long currentTime,
    FrameInfo& frameInfo
) {
    // First check for frame timeout if a frame was in progress
    if (_frameInProgress && (currentTime - buffer.getLastByteTime() > _interFrameTimeout)) {
        // Frame timed out, reset state
        _frameInProgress = false;
    }
    
    // Check if we need to find the start of a frame
    if (!_frameInProgress) {
        size_t startPos;
        if (!findNextFrameStart(buffer, startPos)) {
            // No start delimiter found in the buffer
            return false;
        }
        _frameStartIndex = startPos;
        _frameInProgress = true;
    }
    
    // Now that we have a frame in progress (or found a new one),
    // check if we can extract a complete frame
    return extractCompleteFrame(buffer, frameInfo);
}