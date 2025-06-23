#include "../src/data/decoding/mbus_decoder.h"    
    
#include <assert.h>
#include "../frames.h"
#include <iostream>

namespace mbus_decoder_test {

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

    bool isin(const char* szObisString, const P1Data& p1data) {
        for (uint8_t i = 0; i < p1data.obisStringCount; i++) {
            std::cout << "Checking OBIS string: " << p1data.obisStrings[i] << std::endl;
            if (strcmp(szObisString, p1data.obisStrings[i]) == 0) {
                return true; // Found the OBIS string
            }
        }
        return false; // Not found
    }

    int test_mbus_decoder() {
        P1Data p1data;
        p1data.setDeviceId("12345678901234567890");
        MBusDecoder decoder;

        FrameData frameData(mbus_energy_frame, sizeof(mbus_energy_frame)); 

        assert(decoder.decodeBuffer(frameData, p1data));

        // assert(p1data.obisStringCount == 5);
        assert(isin("1-0:1.8.0*255(12.565000*kWh)", p1data));

        return 0;
    }
   
    int run() {
        test_mbus_decoder();
        return 0;
    }
}