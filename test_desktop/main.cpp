#include <iostream>

#include "../src/config.cpp"    // TODO idk if thi is a good idea

#include "../src/data/data_package.h"
#include "../src/data/decoding/p1data.h"
#include "../src/data/decoding/dlms_decoder.h"
#include "ams/HdlcParser.h"
#include "frames.h"

#include "../src/data/circular_buffer.cpp"
#include "../src/debug.cpp"

#include "../src/backend/graphql.cpp"
#include "../src/backend/request_handler.cpp"

#include "../src/main_actions.cpp"

#include "../src/json_light/json_light.cpp"
#include "../src/data/decoding/ascii_decoder.cpp"
#include "../src/data/decoding/dlms_decoder.cpp"
#include "../src/data/decoding/mbus_decoder.cpp"
#include "../src/data/decoding/p1data.cpp"
#include "../src/data/frame_detector.cpp"
#include "../src/data/serial_frame_buffer.cpp"

#include "zap_str_test.cpp"
#include "debug_test.cpp"
#include "main_actions_test.cpp"

#include "data/circular_buffer_test.cpp"
#include "data/frame_detector_test.cpp"
#include "data/ascii_decoder_test.cpp"
#include "data/mbus_decoder_test.cpp"
#include "data/dlms_decoder_test.cpp"

#include "json_light/json_light_test.cpp"

#include "backend/graphql_test.cpp"
#include "backend/request_handler_test.cpp"


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

int test_decoder_frame() {
    
    P1Data p1data;
    p1data.setDeviceId("12345678901234567890");
    DLMSDecoder decoder;

    // Provide a type ID (e.g., 0 for DLMS, 1 for ASCII - adjust as needed)
    FrameData frameData(hdlc_small_frame, sizeof(hdlc_small_frame)); 

    decoder.decodeBuffer(frameData, p1data);

    // Iterate through the new obisStrings array
    for (int i = 0; i < p1data.obisStringCount; i++) {
        std::cout << "OBIS String: " << p1data.obisStrings[i] << std::endl;
    }

    return 0;
}



int main() {
    std::cout << "==== P1 DLMS Decoder Desktop Tests ====" << std::endl;
    
    try {
        // testBasicFunctionality();
        DataPackage dataPackage;

        P1Data p1data;
        p1data.setDeviceId("12345678901234567890");
        DLMSDecoder decoder;
        uint8_t buffer[] = {0x00, 0x01};

        // decoder.decodeBuffer(buffer, 2, p1data);

        test_decoder_frame();

        circular_buffer_test::run();
        frame_detector_test::run();
        ascii_decoder_test::run();
        mbus_decoder_test::run();
        dlms_decoder_test::run();

        json_light_test::run();
        graphql_test::run();
        zap_str_test::run();
        debug_test::run();
        request_handler_test::run();
        main_actions_test::run();

        std::cout << "All tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        // std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}