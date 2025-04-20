#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <cstddef>
#include <cstdint>

/**
 * @brief A simple circular buffer implementation for byte data
 * 
 * This class manages a circular buffer that can be used as the underlying 
 * storage for various data processing classes.
 */
class CircularBuffer {
public:
    /**
     * @brief Construct a new Circular Buffer
     * 
     * @param bufferSize Size of the buffer in bytes
     */
    CircularBuffer(size_t bufferSize = 1024);
    
    /**
     * @brief Destroy the Circular Buffer
     */
    ~CircularBuffer();
    
    /**
     * @brief Add a byte to the buffer
     * 
     * @param byte Byte to add to the buffer
     * @param currentTime Current time in milliseconds
     * @return true if successful, false if buffer couldn't be allocated
     */
    bool addByte(uint8_t byte, unsigned long currentTime);
    
    /**
     * @brief Clear the buffer and reset all indices
     * 
     * @param currentTime Current time in milliseconds
     */
    void clear(unsigned long currentTime);
    
    /**
     * @brief Get a byte at a specific index in the buffer
     * 
     * @param index Index relative to the current read position
     * @return The byte at the specified position or 0 if out of bounds
     */
    uint8_t getByte(size_t index) const;
    
    /**
     * @brief Get a byte at an absolute position in the buffer
     * 
     * @param position Absolute position in the circular buffer
     * @return The byte at the specified position or 0 if out of bounds
     */
    uint8_t getByteAt(size_t position) const;
    
    /**
     * @brief Advance the read index by a certain number of bytes
     * 
     * @param count Number of bytes to advance
     */
    void advanceReadIndex(size_t count);
    
    /**
     * @brief Get the number of bytes currently in the buffer
     * 
     * @return Number of bytes available to read
     */
    size_t available() const { return _bufferUsed; }
    
    /**
     * @brief Get the time when the last byte was added
     * 
     * @return Time in milliseconds when the last byte was added
     */
    unsigned long getLastByteTime() const { return _lastByteTime; }
    
    /**
     * @brief Get the current write index
     * 
     * @return The write index
     */
    size_t getWriteIndex() const { return _writeIndex; }
    
    /**
     * @brief Get the current read index
     * 
     * @return The read index
     */
    size_t getReadIndex() const { return _readIndex; }
    
    /**
     * @brief Get the buffer size
     * 
     * @return The buffer size in bytes
     */
    size_t getBufferSize() const { return _bufferSize; }
    
    /**
     * @brief Get the number of buffer overflows
     * 
     * @return The overflow count
     */
    uint32_t getOverflowCount() const { return _overflowCount; }

private:
    uint8_t* _buffer;
    size_t _bufferSize;
    size_t _writeIndex;
    size_t _readIndex;
    size_t _bufferUsed;
    unsigned long _lastByteTime;
    uint32_t _overflowCount;
};

#endif // CIRCULAR_BUFFER_H