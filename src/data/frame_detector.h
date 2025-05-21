#pragma once

#include "circular_buffer.h"
#include <vector>
#include <cstdint> // For uint8_t

#include "data/decoding/IFrameData.h"

/**
 * @brief Configuration for a specific frame delimiter type
 */
struct FrameDelimiterInfo {

    uint8_t startDelimiter; // Byte marking the start of the frame
    uint8_t endDelimiter;   // Byte marking the end of the frame
    IFrameData::Type frameType;                 // Identifier for this frame type
    bool isLineBased;       // If true, the actual frame ends with a newline after the endDelimiter

    FrameDelimiterInfo(uint8_t start, uint8_t end, IFrameData::Type type, bool lineBased)
        : startDelimiter(start), endDelimiter(end), frameType(type), isLineBased(lineBased) {}
};

/**
 * @brief Frame information structure
 * 
 * Contains details about a detected frame in the circular buffer
 */
struct FrameInfo {
    size_t startIndex;        // Start position of the frame in the buffer
    size_t endIndex;          // End position of the frame in the buffer 
    size_t size;              // Total size of the frame including delimiters (and newline if line-based)
    bool complete;            // Whether the frame is complete
    IFrameData::Type frameTypeId;          // ID of the detected frame type (from FrameDelimiterInfo)
};

/**
 * @brief A class to detect framed data in a circular buffer
 * 
 * This class detects frames delimited by start and end markers in a circular buffer.
 * It supports multiple delimiter configurations defined by FrameDelimiterInfo.
 * When a complete frame is found, it returns information about the frame location and type.
 */
class FrameDetector {
public:
    /**
     * @brief Construct a new Frame Detector
     * 
     * @param delimiterConfigs A vector of FrameDelimiterInfo structs defining the supported frame types.
     * @param interFrameTimeout Maximum time (ms) between bytes in the same frame
     */
    explicit FrameDetector(
        const std::vector<FrameDelimiterInfo>& delimiterConfigs,
        unsigned long interFrameTimeout = 500
    );
    explicit FrameDetector(unsigned long interFrameTimeout = 500)
        : FrameDetector({}, interFrameTimeout) {} // Default constructor with empty delimiters
    
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
     * @brief Set the frame delimiter configurations
     * 
     * @param delimiterConfigs A vector of FrameDelimiterInfo structs.
     */
    void setFrameDelimiters(const std::vector<FrameDelimiterInfo>& delimiterConfigs);
    
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
    std::vector<FrameDelimiterInfo> _delimiterConfigs;
    unsigned long _interFrameTimeout;

    // Frame detection state
    bool _frameInProgress;
    size_t _frameStartIndex;
    const FrameDelimiterInfo* _activeDelimiterInfo; // Pointer to the active config

    // Statistics
    uint32_t _frameCount;
    
    // Internal methods
    // Finds the next occurrence of any start delimiter
    bool findNextFrameStart(const CircularBuffer& buffer, size_t& startPos); 
    // Extracts a frame defined by the active delimiter config
    bool extractCompleteFrame(const CircularBuffer& buffer, FrameInfo& frameInfo); 
};
