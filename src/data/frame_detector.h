#ifndef FRAME_DETECTOR_H
#define FRAME_DETECTOR_H

#include "circular_buffer.h"

/**
 * @brief Frame information structure
 * 
 * Contains details about a detected frame in the circular buffer
 */
struct FrameInfo {
    size_t startIndex;        // Start position of the frame in the buffer
    size_t endIndex;          // End position of the frame in the buffer 
    size_t size;              // Total size of the frame including delimiters
    bool complete;            // Whether the frame is complete
};

/**
 * @brief A class to detect framed data in a circular buffer
 * 
 * This class detects frames delimited by start and end markers in a circular buffer.
 * When a complete frame is found, it returns information about the frame location.
 */
class FrameDetector {
public:
    /**
     * @brief Construct a new Frame Detector
     * 
     * @param startDelimiter Character that marks the start of a frame
     * @param endDelimiter Character that marks the end of a frame
     * @param interFrameTimeout Maximum time (ms) between bytes in the same frame
     */
    FrameDetector(
        uint8_t startDelimiter = '/',  // Default for many P1 protocols
        uint8_t endDelimiter = '!',    // Default for many P1 protocols
        unsigned long interFrameTimeout = 500
    );
    

    
    /**
     * @brief Process a chunk of data and check if it completes a frame
     * 
     * @param buffer The circular buffer containing the data
     * @param currentTime Current time in milliseconds
     * @param frameInfo Output parameter to store detected frame information
     * @return true if a complete frame was detected
     */
    bool detect(
        const CircularBuffer& buffer,
        unsigned long currentTime,
        FrameInfo& frameInfo
    );
    
   
    /**
     * @brief Reset the detector state
     */
    void reset();
    
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
     * @brief Get the number of complete frames detected
     * 
     * @return Frame count
     */
    uint32_t getFrameCount() const { return _frameCount; }

private:
    // Frame detection
    uint8_t _startDelimiter;
    uint8_t _endDelimiter;
    bool _frameInProgress;
    size_t _frameStartIndex;
    unsigned long _interFrameTimeout;
    
    // Statistics
    uint32_t _frameCount;
    
    // Internal methods
    bool findNextFrameStart(const CircularBuffer& buffer, size_t& startPos);
    bool extractCompleteFrame(const CircularBuffer& buffer, FrameInfo& frameInfo);
};

#endif // FRAME_DETECTOR_H