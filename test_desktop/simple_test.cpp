#include <iostream>

#include "../src/config.cpp"    // TODO idk if thi is a good idea

#include "../src/data/data_package.h"
#include "../src/data/p1data.h"
#include "../src/data/p1_dlms_decoder.h"
#include "ams/HdlcParser.h"
#include "frames.h"

#include "../src/data/circular_buffer.cpp"
#include "../src/debug.cpp"
#include "../src/backend/graphql.cpp"

#include "zap_str_test.cpp"

#include "data/circular_buffer_test.cpp"
#include "data/frame_detector_test.cpp"

#include "json_light/json_light_test.cpp"

#include "backend/graphql_test.cpp"


class FrameData : public IFrameData {
public:
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
private:
    const uint8_t* data_;
    size_t size_;
};

int test_decoder_frame() {
    
    P1Data p1data;
    p1data.setDeviceId("12345678901234567890");
    P1DLMSDecoder decoder;

    FrameData frameData(faulty_aidon_frame_2, sizeof(faulty_aidon_frame_2));

    decoder.decodeBuffer(frameData, p1data);


    for (int i = 0; i < p1data.obisCount; i++) {
        char buffer[64];
        p1data.obisValues[i].toString(buffer, sizeof(buffer));
        std::cout << "OBIS Value: " << buffer << std::endl;
    }

    HDLCParser hdlcParser;
    DataParserContext dpc;
    dpc.length = sizeof(faulty_aidon_frame_2);
    dpc.type = 0;
    dpc.timestamp = 0;
    hdlcParser.parse(faulty_aidon_frame_2, dpc);

    return 0;
}

int main() {
    std::cout << "==== P1 DLMS Decoder Desktop Tests ====" << std::endl;
    
    try {
        // testBasicFunctionality();
        DataPackage dataPackage;

        P1Data p1data;
        p1data.setDeviceId("12345678901234567890");
        P1DLMSDecoder decoder;
        uint8_t buffer[] = {0x00, 0x01};

        // decoder.decodeBuffer(buffer, 2, p1data);

        test_decoder_frame();

        circular_buffer_test::run();
        frame_detector_test::run();
        json_light_test::run();
        graphql_test::run();
        zap_str_test::run();

        std::cout << "All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        // std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}