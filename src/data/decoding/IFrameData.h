#pragma once

#include <cstddef>
#include <cstdint>

/**
 * @brief Interface for accessing frame data without copying
 * 
 * This interface allows components to access frame data directly
 * from the original buffer (which may be a circular buffer),
 * without needing to copy the data to a temporary buffer.
 */
class IFrameData {
public:

    typedef enum {
        FRAME_TYPE_UNKNOWN = -1,
        FRAME_TYPE_ASCII = 0,
        FRAME_TYPE_DLMS = 1,
    } Type;


    /**
     * @brief Virtual destructor
     */
    virtual ~IFrameData() = default;

    /**
     * @brief Get a byte from the frame at the specified index
     * 
     * @param index Index into the frame data (0 is the first byte)
     * @return uint8_t The byte value at the specified position
     */
    virtual uint8_t getFrameByte(size_t index) const = 0;
    
    /**
     * @brief Get the total size of the frame in bytes
     * 
     * @return size_t The frame size
     */
    virtual size_t getFrameSize() const = 0;

    /**
     * @brief Get the type ID of the frame
     * 
     * @return uint8_t The type ID of the frame (e.g., ASCII or DLMS)
     */
    virtual Type getFrameTypeId() const = 0;
};
