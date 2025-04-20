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

bool FrameDetector::processByte(
    const CircularBuffer& buffer, 
    uint8_t byte, 
    unsigned long currentTime, 
    FrameInfo& frameInfo
) {
    // First check for frame timeout
    if (_frameInProgress && (currentTime - buffer.getLastByteTime() > _interFrameTimeout)) {
        // Frame timed out, reset state
        _frameInProgress = false;
    }
    
    // Note: We're not adding the byte to the buffer here anymore
    // That's the responsibility of the caller
    
    // Special case if start and end delimiters are the same
    if (_startDelimiter == _endDelimiter && byte == _endDelimiter) {
        if (_frameInProgress) {
            // We have a complete frame
            return extractCompleteFrame(buffer, frameInfo);
        } else {
            // Mark the beginning of a frame
            _frameInProgress = true;
            _frameStartIndex = (buffer.getWriteIndex() - 1 + buffer.getBufferSize()) % buffer.getBufferSize();
            return false;
        }
    } else {
        // Normal case with different start and end delimiters
        if (byte == _startDelimiter && !_frameInProgress) {
            // Start of a new frame
            _frameInProgress = true;
            _frameStartIndex = (buffer.getWriteIndex() - 1 + buffer.getBufferSize()) % buffer.getBufferSize();
            return false;
        }
        
        if (_frameInProgress && byte == _endDelimiter) {
            // End of current frame
            return extractCompleteFrame(buffer, frameInfo);
        }
    }
    
    return false;
}

bool FrameDetector::update(
    const CircularBuffer& buffer,
    unsigned long currentTime, 
    FrameInfo& frameInfo
) {
    if (buffer.available() == 0) {
        return false;
    }
    
    // Check if current frame timed out
    if (_frameInProgress && (currentTime - buffer.getLastByteTime() > _interFrameTimeout)) {
        _frameInProgress = false;
        // We don't try to salvage incomplete frames
    }
    
    // Try to find and extract any complete frames in the buffer
    return extractCompleteFrame(buffer, frameInfo);
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