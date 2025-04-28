#include "../src/data/decoding/ascii_decoder.h"    
    
#include <assert.h>
#include "../frames.h"

namespace ascii_decoder_test {

    class FrameData : public IFrameData {
        public:
            // Constructor now takes typeId
            FrameData(const uint8_t* data, size_t size) : data_(data), size_(size) {}
        
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
            IFrameData::Type getFrameTypeId() const override {
                return IFrameData::Type::FRAME_TYPE_UNKNOWN; // Assuming this is a DLMS frame
            }
        private:
            const uint8_t* data_;
            size_t size_;
        };

    int test_ascii_decoder() {
        P1Data p1data;
        p1data.setDeviceId("12345678901234567890");
        AsciiDecoder decoder;

        FrameData frameData(ascii_frame_single, sizeof(ascii_frame_single)); 

        assert(decoder.decodeBuffer(frameData, p1data));

        return 0;
    }
   
    int run() {
        test_ascii_decoder();
        return 0;
    }
}