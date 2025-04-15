#ifndef SERIAL_FRAME_BUFFER_H
#define SERIAL_FRAME_BUFFER_H

#include <Arduino.h>
#include <functional>

/**
 * @brief A robust circular buffer for handling serial data frames
 * 
 * This class manages a circular buffer to handle serial data that may arrive in bursts,
 * with special handling for framed protocols (start/end delimiters).
 * It automatically detects complete frames and calls a user-provided callback when available.
 */
class SerialFrameBuffer {
public:
    /**
     * @brief Frame detection callback type
     * @param buffer Pointer to the buffer containing the frame data
     * @param size Size of the frame data in bytes
     * @return true if frame was successfully processed, false otherwise
     */
    using FrameCallback = std::function<bool(const uint8_t*, size_t)>;
    
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
     * @param currentTime Time in milliseconds since the last byte was received millis will be used if not provided.
     * @return true if this byte completed a frame that was then processed
     */
    bool addByte(uint8_t byte);
    bool addByte(uint8_t byte, unsigned long currentTime);
    
    /**
     * @brief Add multiple bytes to the buffer
     * 
     * @param data Pointer to the data
     * @param length Length of the data in bytes
     * @return true if at least one complete frame was detected and processed
     */
    bool addData(const uint8_t* data, size_t length);
    
    /**
     * @brief Update internal state and check for timeouts
     * 
     * @return true if a frame was processed during this call
     */
    bool update();
    
    /**
     * @brief Clear the buffer
     */
    void clear();
    
    /**
     * @brief Set the frame callback function
     * 
     * @param callback Function to call when a complete frame is detected
     */
    void setFrameCallback(FrameCallback callback) { _frameCallback = callback; }
    
    /**
     * @brief Set the frame delimiters
     * 
     * @param startDelimiter Character that marks the start of a frame
     * @param endDelimiter Character that marks the end of a frame
     */
    void setFrameDelimiters(uint8_t startDelimiter, uint8_t endDelimiter) {
        _startDelimiter = startDelimiter;
        _endDelimiter = endDelimiter;
    }
    
    /**
     * @brief Set the inter-frame timeout
     * 
     * @param timeout Maximum time (ms) between bytes in the same frame
     */
    void setInterFrameTimeout(unsigned long timeout) { _interFrameTimeout = timeout; }
    
    /**
     * @brief Get the number of bytes currently in the buffer
     * 
     * @return Number of bytes in the buffer
     */
    size_t available() const { return _bufferUsed; }
    
    /**
     * @brief Get the number of complete frames detected so far
     * 
     * @return Frame count
     */
    uint32_t getFrameCount() const { return _frameCount; }
    
    /**
     * @brief Get the number of bytes that were discarded due to buffer overflow
     * 
     * @return Bytes discarded
     */
    uint32_t getOverflowCount() const { return _overflowCount; }

private:
    // Buffer management
    uint8_t* _buffer;
    size_t _bufferSize;
    size_t _writeIndex;
    size_t _readIndex;
    size_t _bufferUsed;
    
    // Frame detection
    uint8_t _startDelimiter;
    uint8_t _endDelimiter;
    bool _frameInProgress;
    size_t _frameStartIndex;
    unsigned long _lastByteTime;
    unsigned long _interFrameTimeout;
    
    // Statistics
    uint32_t _frameCount;
    uint32_t _overflowCount;
    
    // Callback for frame processing
    FrameCallback _frameCallback;
    
    // Internal methods
    bool processCompleteFrames();
    bool findNextFrameStart();
    bool extractCompleteFrame(uint8_t** frameData, size_t* frameSize);
    void advanceReadIndex(size_t count);
};

#endif // SERIAL_FRAME_BUFFER_H