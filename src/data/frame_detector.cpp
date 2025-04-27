#include "frame_detector.h"
#include <vector>
#include <cstdint>

FrameDetector::FrameDetector(
    const std::vector<FrameDelimiterInfo>& delimiterConfigs,
    unsigned long interFrameTimeout
) : _delimiterConfigs(delimiterConfigs),
    _interFrameTimeout(interFrameTimeout),
    _frameInProgress(false),
    _frameStartIndex(0),
    _activeDelimiterInfo(nullptr),
    _frameCount(0) {
}

void FrameDetector::setFrameDelimiters(const std::vector<FrameDelimiterInfo>& delimiterConfigs) {
    _delimiterConfigs = delimiterConfigs;
    reset();
}

void FrameDetector::reset() {
    _frameInProgress = false;
    _frameStartIndex = 0;
    _activeDelimiterInfo = nullptr;
}

bool FrameDetector::findNextFrameStart(const CircularBuffer& buffer, size_t& startPos) {
    if (buffer.available() == 0 || _delimiterConfigs.empty()) {
        return false;
    }
    
    size_t bytesSearched = 0;
    size_t bufferUsed = buffer.available();
    
    while (bytesSearched < bufferUsed) {
        uint8_t current = buffer.getByte(bytesSearched);
        for (const auto& config : _delimiterConfigs) {
            if (current == config.startDelimiter) {
                startPos = (buffer.getReadIndex() + bytesSearched) % buffer.getBufferSize();
                _activeDelimiterInfo = &config;
                _frameInProgress = true;
                return true;
            }
        }
        bytesSearched++;
    }
    
    return false;
}

bool FrameDetector::extractCompleteFrame(const CircularBuffer& buffer, FrameInfo& frameInfo) {
    if (buffer.available() < 2 || !_frameInProgress || !_activeDelimiterInfo) {
        return false; 
    }
    
    size_t searchPos = _frameStartIndex;
    size_t bytesSearched = 0;
    size_t bufferSize = buffer.getBufferSize();
    size_t bufferUsed = buffer.available();
    uint8_t endDelimiter = _activeDelimiterInfo->endDelimiter;
    bool isLineBased = _activeDelimiterInfo->isLineBased;

    while (bytesSearched < bufferUsed) {
        if (searchPos == _frameStartIndex && bytesSearched > 0) {
            // Optional: Add logic here if start/end can be the same and frame size > 1
        }

        uint8_t current = buffer.getByteAt(searchPos);
        
        if (current == endDelimiter && searchPos != _frameStartIndex) { 
            size_t endPos = searchPos;
            size_t frameLength;

            if (isLineBased) {
                while (bytesSearched + 1 < bufferUsed) { 
                    uint8_t nextByte = buffer.getByteAt(searchPos);
                    if (nextByte == '\r' || nextByte == '\n') {
                        endPos = searchPos - 1; // Exclude the return character typical \r\n
                        break;
                    } else {
                        searchPos = (searchPos + 1) % bufferSize;
                        bytesSearched++; 
                    }
                }
                // no newline found so frame is not complete (yet)
                if (!(bytesSearched + 1 < bufferUsed)) {
                     frameInfo.complete = false;
                     return false;
                }
            }

            if (endPos >= _frameStartIndex) {
                frameLength = endPos - _frameStartIndex + 1;
            } else {
                frameLength = bufferSize - _frameStartIndex + endPos + 1;
            }
            
            size_t minSize = isLineBased ? 3 : 2;
            if (frameLength >= minSize) {
                frameInfo.startIndex = _frameStartIndex;
                frameInfo.endIndex = endPos;
                frameInfo.size = frameLength;
                frameInfo.complete = true;
                frameInfo.frameTypeId = _activeDelimiterInfo->id;
                
                _frameInProgress = false;
                _activeDelimiterInfo = nullptr;
                
                _frameCount++;
                
                return true;
            }
        }
        
        searchPos = (searchPos + 1) % bufferSize;
        bytesSearched++;
    }
    
    frameInfo.complete = false;
    return false;
}

bool FrameDetector::detect(
    const CircularBuffer& buffer,
    unsigned long currentTime,
    FrameInfo& frameInfo
) {
    if (_frameInProgress && _interFrameTimeout > 0 && (currentTime - buffer.getLastByteTime() > _interFrameTimeout)) {
        reset();
    }
    
    if (!_frameInProgress) {
        size_t startPos;
        if (!findNextFrameStart(buffer, startPos)) {
            return false;
        }
        _frameStartIndex = startPos;
    }
    
    return extractCompleteFrame(buffer, frameInfo);
}