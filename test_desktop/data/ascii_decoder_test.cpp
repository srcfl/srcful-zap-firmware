#include "../src/data/decoding/ascii_decoder.h"    
    
#include <assert.h>
#include "../frames.h"

namespace ascii_decoder_test {

    class FrameData : public IFrameData {
        public:
            // Constructor now takes typeId
            FrameData(const uint8_t* data, size_t size, uint8_t typeId) : data_(data), size_(size), typeId_(typeId) {}
        
            uint8_t getFrameByte(size_t index) const override {
                if (index < size_) {
                    return data_[index];
                }
                return 0;
            }
        
            size_t getFrameSize() const override {
                return size_;
            }
        
            // Implementation for the new virtual function
            uint8_t getFrameTypeId() const override {
                return typeId_;
            }
        private:
            const uint8_t* data_;
            size_t size_;
            uint8_t typeId_; // Added member to store type ID
    };

    int test_ascii_decoder() {
        P1Data p1data;
        p1data.setDeviceId("12345678901234567890");
        AsciiDecoder decoder;

        FrameData frameData(ascii_frame_single, sizeof(ascii_frame_single), 1); 

        assert(decoder.decodeBuffer(frameData, p1data));

        return 0;
    }
   
    int run() {
        test_ascii_decoder();
        return 0;
    }
}