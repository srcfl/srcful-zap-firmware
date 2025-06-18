#include "data/decoding/mbus_decoder.h"

bool MBusDecoder::detect(const CircularBuffer& buffer, unsigned long currentTime, FrameInfo& frameInfo) {
    
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

void MBusDecoder::reset() {
    _frameInProgress = false;
    _frameStartIndex = 0;
}

bool MBusDecoder::findNextFrameStart(const CircularBuffer& buffer, size_t& startPos) {
    if (buffer.available() == 0) {
        return false;
    }
    
    size_t bytesSearched = 0;
    size_t bufferUsed = buffer.available();
    
    while (bytesSearched < bufferUsed) {
        uint8_t current = buffer.getByte(bytesSearched);
        if (current == 0x68) { // Start of M-Bus frame
            startPos = (buffer.getReadIndex() + bytesSearched) % buffer.getBufferSize();
            _frameInProgress = true;
            return true;
        }
        bytesSearched++;
    }
    
    return false;
}

bool MBusDecoder::extractCompleteFrame(const CircularBuffer& buffer, FrameInfo& frameInfo) {
    if (buffer.available() < 6 || !_frameInProgress) { // Minimum M-Bus frame is 6 bytes
        return false; 
    }
    
    const size_t bufferSize = buffer.getBufferSize();
    const size_t bufferUsed = buffer.available();
    
    // Calculate how many bytes we have from frame start to current buffer end
    size_t bytesFromStart;
    if (buffer.getWriteIndex() >= _frameStartIndex) {
        bytesFromStart = buffer.getWriteIndex() - _frameStartIndex;
    } else {
        bytesFromStart = bufferSize - _frameStartIndex + buffer.getWriteIndex();
    }
    
    if (bytesFromStart < 6) {
        return false; // Not enough data yet
    }
    
    // Read the frame header to determine expected length
    // M-Bus frame format: 0x68 L1 L2 0x68 [data] FCS 0x16
    uint8_t startByte1 = getByteAtOffset(buffer, _frameStartIndex, 0);
    uint8_t len1 = getByteAtOffset(buffer, _frameStartIndex, 1);
    uint8_t len2 = getByteAtOffset(buffer, _frameStartIndex, 2);
    uint8_t startByte2 = getByteAtOffset(buffer, _frameStartIndex, 3);
    
    // Validate frame header
    if (startByte1 != 0x68 || startByte2 != 0x68) {
        // Invalid frame, reset and try to find next start
        reset();
        return false;
    }
    
    // Check if length bytes match
    if (len1 != len2) {
        // Invalid frame, reset and try to find next start
        reset();
        return false;
    }
    
    // Calculate expected frame length
    size_t expectedDataLength = len1;

    const int headersize = 3;
    const int footersize = 1;

    if(expectedDataLength == 0x00)
        expectedDataLength = bufferUsed - headersize - footersize;
    // Payload can max be 255 bytes, so I think the following case is only valid for austrian meters
    if(expectedDataLength < headersize)
        expectedDataLength += 256;

    size_t expectedTotalLength = 4 + expectedDataLength + 2; // header(4) + data + FCS(1) + end(1)
    
    // Handle special case where len1 == 0 (variable length frame)
    if (len1 == 0) {
        // For variable length frames, we need to search for the end byte
        // This is less common but used in some implementations
        return extractVariableLengthFrame(buffer, frameInfo);
    }
    
    // Check if we have enough bytes for the complete frame
    if (bytesFromStart < expectedTotalLength) {
        return false; // Frame not complete yet
    }
    
    // Validate the end sequence
    uint8_t fcs = getByteAtOffset(buffer, _frameStartIndex, 4 + expectedDataLength);
    uint8_t endByte = getByteAtOffset(buffer, _frameStartIndex, 4 + expectedDataLength + 1);
    
    if (endByte != 0x16) {
        // Invalid frame, reset and try to find next start
        reset();
        return false;
    }
    
    // Optional: Validate checksum (FCS)
    if (!validateChecksum(buffer, _frameStartIndex, expectedDataLength, fcs)) {
        // Invalid checksum, reset and try to find next start
        reset();
        return false;
    }
    
    // Frame is complete and valid
    size_t endIndex = (_frameStartIndex + expectedTotalLength - 1) % bufferSize;
    
    frameInfo.startIndex = _frameStartIndex;
    frameInfo.endIndex = endIndex;
    frameInfo.size = expectedTotalLength;
    frameInfo.complete = true;
    frameInfo.frameTypeId = IFrameData::Type::FRAME_TYPE_MBUS;
    
    _frameInProgress = false;
    
    return true;
}

bool MBusDecoder::extractVariableLengthFrame(const CircularBuffer& buffer, FrameInfo& frameInfo) {
    size_t bufferSize = buffer.getBufferSize();
    size_t searchPos = (_frameStartIndex + 4) % bufferSize; // Start after header
    size_t bytesSearched = 4; // Already processed header
    size_t maxSearch = buffer.available();
    
    while (bytesSearched < maxSearch) {
        uint8_t current = getByteAtOffset(buffer, _frameStartIndex, bytesSearched);
        
        if (current == 0x16) {
            // Found potential end byte, validate frame
            size_t frameLength = bytesSearched + 1;
            
            if (frameLength >= 6) { // Minimum valid frame
                // Validate checksum if needed
                uint8_t fcs = getByteAtOffset(buffer, _frameStartIndex, bytesSearched - 1);
                size_t dataLength = bytesSearched - 5; // Total - header(4) - FCS(1) - end(1)
                
                if (validateChecksum(buffer, _frameStartIndex, dataLength, fcs)) {
                    size_t endIndex = (_frameStartIndex + frameLength - 1) % bufferSize;
                    
                    frameInfo.startIndex = _frameStartIndex;
                    frameInfo.endIndex = endIndex;
                    frameInfo.size = frameLength;
                    frameInfo.complete = true;
                    frameInfo.frameTypeId = IFrameData::Type::FRAME_TYPE_MBUS;
                    
                    _frameInProgress = false;
                    return true;
                }
            }
        }
        
        bytesSearched++;
    }
    
    return false; // Frame not complete yet
}

uint8_t MBusDecoder::getByteAtOffset(const CircularBuffer& buffer, size_t startIndex, size_t offset) {
    size_t bufferSize = buffer.getBufferSize();
    size_t actualIndex = (startIndex + offset) % bufferSize;
    
    // Convert absolute buffer index to relative index for getByte
    size_t readIndex = buffer.getReadIndex();
    size_t relativeIndex;
    
    if (actualIndex >= readIndex) {
        relativeIndex = actualIndex - readIndex;
    } else {
        relativeIndex = bufferSize - readIndex + actualIndex;
    }
    
    return buffer.getByte(relativeIndex);
}

bool MBusDecoder::validateChecksum(const CircularBuffer& buffer, size_t startIndex, size_t dataLength, uint8_t expectedFcs) {
    uint8_t calculatedFcs = 0;
    
    // Calculate checksum over the data portion (after header, before FCS)
    for (size_t i = 0; i < dataLength; i++) {
        uint8_t dataByte = getByteAtOffset(buffer, startIndex, 4 + i); // 4 = header size
        calculatedFcs += dataByte;
    }
    
    return calculatedFcs == expectedFcs;
}