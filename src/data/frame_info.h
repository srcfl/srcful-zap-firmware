#pragma once
#include <cstddef>
#include "data/decoding/IFrameData.h"


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