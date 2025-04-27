#ifndef FRAME_DETECTOR_H
#define FRAME_DETECTOR_H

#include "circular_buffer.h"
#include <vector>   // Include vector
#include <utility>  // Include pair

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
 * It supports multiple delimiter pairs.
 * When a complete frame is found, it returns information about the frame location.
 */
class FrameDetector {
public:
    /**
     * @brief Construct a new Frame Detector
     * 
     * @param delimiterPairs A vector of pairs, where each pair contains a start and end delimiter.
     * @param interFrameTimeout Maximum time (ms) between bytes in the same frame
     */
    FrameDetector(
        const std::vector<std::pair<uint8_t, uint8_t>>& delimiterPairs,
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
     * @brief Set the frame delimiter pairs
     * 
     * @param delimiterPairs A vector of pairs, where each pair contains a start and end delimiter.
     */
    void setFrameDelimiters(const std::vector<std::pair<uint8_t, uint8_t>>& delimiterPairs);
    
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
    // Frame detection configuration
    std::vector<std::pair<uint8_t, uint8_t>> _delimiterPairs;
    unsigned long _interFrameTimeout;

    // Frame detection state
    bool _frameInProgress;
    size_t _frameStartIndex;
    uint8_t _activeEndDelimiter; // The end delimiter we are currently looking for

    // Statistics
    uint32_t _frameCount;
    
    // Internal methods
    // Finds the next occurrence of any start delimiter
    bool findNextFrameStart(const CircularBuffer& buffer, size_t& startPos, uint8_t& foundStartDelimiter); 
    // Extracts a frame defined by the active start/end delimiters, starting search from _frameStartIndex
    bool extractCompleteFrame(const CircularBuffer& buffer, FrameInfo& frameInfo); 
};

#endif // FRAME_DETECTOR_H