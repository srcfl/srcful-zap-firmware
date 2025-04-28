#ifndef SERIAL_FRAME_BUFFER_H
#define SERIAL_FRAME_BUFFER_H

#include <functional>
#include "circular_buffer.h"
#include "frame_detector.h"
#include "decoding/IFrameData.h"
#include <vector> // Include vector

/**
 * @brief A robust buffer for handling serial data frames
 * 
 * This class manages serial data that may arrive in bursts,
 * with special handling for framed protocols (start/end delimiters).
 * It automatically detects complete frames and calls a user-provided callback when available.
 */
class SerialFrameBuffer : public IFrameData {
public:
    /**
     * @brief Frame detection callback type
     * @param frameData Reference to the IFrameData interface for accessing frame data
     * @return true if frame was successfully processed, false otherwise
     */
    using FrameCallback = std::function<bool(IFrameData& frameData)>;
    
    /**
     * @brief Construct a new SerialFrameBuffer
     * 
     * @param bufferSize Size of the buffer in bytes
     * @param startDelimiter Character that marks the start of a frame
     * @param endDelimiter Character that marks the end of a frame
     * @param interFrameTimeout Maximum time (ms) between bytes in the same frame
     */
    SerialFrameBuffer(
        size_t bufferSize = 1024,
        uint8_t startDelimiter = '/',  // Default for many P1 protocols
        uint8_t endDelimiter = '!',    // Default for many P1 protocols
        unsigned long interFrameTimeout = 500
    );
    
    /**
     * @brief Destroy the SerialFrameBuffer
     */
    ~SerialFrameBuffer();
    
    /**
     * @brief Add data to the buffer
     * 
     * @param byte Byte to add to the buffer
     * @param currentTime Time in milliseconds since the last byte was received
     * @return true if this byte completed a frame that was then processed
     */
    bool addByte(uint8_t byte, unsigned long currentTime);
    
    /**
     * @brief Add multiple bytes to the buffer
     * 
     * @param data Pointer to the data
     * @param length Length of the data in bytes
     * @param currentTime Current time in milliseconds
     * @return true if at least one complete frame was detected and processed
     */
    bool addData(const uint8_t* data, size_t length, unsigned long currentTime);
    
    /**
     * @brief Update internal state and check for timeouts
     * 
     * @param currentTime Current time in milliseconds
     * @return true if a frame was processed during this call
     */
    bool processBufferForFrames(unsigned long currentTime);
    
    /**
     * @brief Clear the buffer
     * 
     * @param currentTime Current time in milliseconds
     */
    void clear(unsigned long currentTime);
    
    /**
     * @brief Set the frame callback function
     * 
     * @param callback Function to call when a complete frame is detected
     */
    void setFrameCallback(FrameCallback callback) { _frameCallback = callback; }
    
  
    /**
     * @brief Set the inter-frame timeout
     * 
     * @param timeout Maximum time (ms) between bytes in the same frame
     */
    void setInterFrameTimeout(unsigned long timeout) {
        _frameDetector.setInterFrameTimeout(timeout);
    }
    
    /**
     * @brief Get the number of bytes currently in the buffer
     * 
     * @return Number of bytes in the buffer
     */
    size_t available() const { return _circularBuffer.available(); }
    
    /**
     * @brief Get the number of complete frames detected so far
     * 
     * @return Frame count
     */
    uint32_t getFrameCount() const { return _frameDetector.getFrameCount(); }
    
    /**
     * @brief Get the number of bytes that were discarded due to buffer overflow
     * 
     * @return Bytes discarded
     */
    uint32_t getOverflowCount() const { return _circularBuffer.getOverflowCount(); }

    // IFrameData interface implementation
    virtual size_t getFrameSize() const override { 
        return _currentFrameSize; 
    } 
    
    virtual uint8_t getFrameByte(size_t index) const override {
        if (index < _currentFrameSize) {
            return _circularBuffer.getByteAt((_currentFrameStartIndex + index) % _circularBuffer.getBufferSize());
        }
        return 0;
    }

    // Implementation for the new virtual function
    virtual IFrameData::Type getFrameTypeId() const override {
        return _currentFrameTypeId;
    }

private:
    CircularBuffer _circularBuffer;
    FrameDetector _frameDetector;
    
    // Frame information for IFrameData interface
    size_t _currentFrameSize;
    size_t _currentFrameStartIndex;
    IFrameData::Type _currentFrameTypeId; // Added member to store type ID
    
    // Callback for frame processing
    FrameCallback _frameCallback;
    
    // Internal methods
    bool processDetectedFrame();
    
    size_t calculateReadAdvance(const FrameInfo& frameInfo) const;
    void updateCurrentFrame(const FrameInfo& frameInfo);
};

#endif // SERIAL_FRAME_BUFFER_H