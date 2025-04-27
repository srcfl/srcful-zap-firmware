#include "frame_detector.h"
#include <vector>   // Include vector
#include <utility>  // Include pair

FrameDetector::FrameDetector(
    const std::vector<std::pair<uint8_t, uint8_t>>& delimiterPairs,
    unsigned long interFrameTimeout
) : _delimiterPairs(delimiterPairs),
    _interFrameTimeout(interFrameTimeout),
    _frameInProgress(false),
    _frameStartIndex(0),
    _activeEndDelimiter(0), // Initialize active end delimiter
    _frameCount(0) {
    // Nothing else to initialize
}

void FrameDetector::setFrameDelimiters(const std::vector<std::pair<uint8_t, uint8_t>>& delimiterPairs) {
    _delimiterPairs = delimiterPairs;
    reset(); // Reset state when delimiters change
}

void FrameDetector::reset() {
    _frameInProgress = false;
    _frameStartIndex = 0;
    _activeEndDelimiter = 0; // Reset active end delimiter
}

// Finds the next occurrence of any start delimiter
bool FrameDetector::findNextFrameStart(const CircularBuffer& buffer, size_t& startPos, uint8_t& foundStartDelimiter) {
    if (buffer.available() == 0 || _delimiterPairs.empty()) {
        return false;
    }
    
    size_t bytesSearched = 0;
    size_t bufferUsed = buffer.available();
    
    while (bytesSearched < bufferUsed) {
        uint8_t current = buffer.getByte(bytesSearched);
        // Check against all configured start delimiters
        for (const auto& pair : _delimiterPairs) {
            if (current == pair.first) {
                // Found a start delimiter
                startPos = (buffer.getReadIndex() + bytesSearched) % buffer.getBufferSize();
                foundStartDelimiter = pair.first; // Return the specific start delimiter found
                _activeEndDelimiter = pair.second; // Set the corresponding end delimiter we're looking for
                _frameInProgress = true;
                return true;
            }
        }
        bytesSearched++;
    }
    
    return false;
}

// Extracts a frame defined by the active start/end delimiters, starting search from _frameStartIndex
bool FrameDetector::extractCompleteFrame(const CircularBuffer& buffer, FrameInfo& frameInfo) {
    if (buffer.available() < 2 || !_frameInProgress) {
        return false;
    }
    
    // Start our search from the frame start index
    size_t searchPos = _frameStartIndex;
    size_t bytesSearched = 0;
    size_t bufferSize = buffer.getBufferSize();
    size_t bufferUsed = buffer.available();
    
    // Search for the active end delimiter
    // We already know the start delimiter was at _frameStartIndex
    while (bytesSearched < bufferUsed) {
        // Ensure we don't re-evaluate the start byte as the end byte immediately
        if (searchPos == _frameStartIndex && bytesSearched > 0) {
             // Wrap around or completed search without finding end delimiter yet
             // This condition might need refinement depending on exact circular buffer behavior
        } 
        
        uint8_t current = buffer.getByteAt(searchPos);
        
        if (current == _activeEndDelimiter && searchPos != _frameStartIndex) { // Make sure end is not the same as start unless frame is > 1 byte
            // Found the active end delimiter
            size_t endPos = searchPos;
            
            // Calculate frame size (including delimiters)
            size_t frameLength;
            if (endPos >= _frameStartIndex) {
                frameLength = endPos - _frameStartIndex + 1;
            } else {
                frameLength = bufferSize - _frameStartIndex + endPos + 1;
            }
            
            // Only proceed if we have a reasonable frame size
            if (frameLength >= 2) {
                // Fill the frame info structure
                frameInfo.startIndex = _frameStartIndex;
                frameInfo.endIndex = endPos;
                frameInfo.size = frameLength;
                frameInfo.complete = true;
                
                // Reset frame tracking state for the next frame
                _frameInProgress = false;
                _activeEndDelimiter = 0;
                
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
    if (_frameInProgress && _interFrameTimeout > 0 && (currentTime - buffer.getLastByteTime() > _interFrameTimeout)) {
        // Frame timed out, reset state
        reset();
    }
    
    // Check if we need to find the start of a frame
    if (!_frameInProgress) {
        size_t startPos;
        uint8_t foundStartDelimiter; // To store which start delimiter was found
        if (!findNextFrameStart(buffer, startPos, foundStartDelimiter)) {
            // No start delimiter found in the buffer
            return false;
        }
        _frameStartIndex = startPos;
        // _frameInProgress and _activeEndDelimiter are set within findNextFrameStart
    }
    
    // Now that we have a frame in progress (found a start delimiter),
    // check if we can extract a complete frame using the corresponding end delimiter.
    return extractCompleteFrame(buffer, frameInfo);
}