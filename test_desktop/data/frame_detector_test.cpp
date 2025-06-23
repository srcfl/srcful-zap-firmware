
#include "../frames.h"

#include "../src/data/decoding/IFrameData.h"
#include "../src/data/frame_detector.h"
#include "../src/data/serial_frame_buffer.h"

#include <assert.h>

namespace frame_detector_test {


    int test_detect_aidon() {
        FrameDetector frameDetector(SerialFrameBuffer::getFrameDelimiters(), 500);
        CircularBuffer buffer(1024);
        FrameInfo frameInfo;
        unsigned long currentTime = 1000;

        for (int i = 0; i < sizeof(aidon_test_buffer); i++) {
            buffer.addByte(aidon_test_buffer[i], currentTime);
            currentTime += 100;
        }

        const bool found = frameDetector.detect(buffer, currentTime, frameInfo);
        assert(found == true);
        assert(frameInfo.startIndex == 0);
        assert(frameInfo.endIndex == sizeof(aidon_test_buffer) - 1);
        assert(frameInfo.size == sizeof(aidon_test_buffer));
        assert(frameInfo.complete == true);
        assert(frameInfo.frameTypeId == IFrameData::Type::FRAME_TYPE_HDLC);

        return 0;
    }

    int test_detect_ascii() {
        FrameDetector frameDetector(SerialFrameBuffer::getFrameDelimiters(), 500);
        CircularBuffer buffer(1024);
        FrameInfo frameInfo;
        unsigned long currentTime = 1000;

        for (int i = 0; i < sizeof(ascii_frame_multi); i++) {
            buffer.addByte(ascii_frame_multi[i], currentTime);
            currentTime += 100;
        }

        const bool found = frameDetector.detect(buffer, currentTime, frameInfo);
        assert(found == true);
        assert(frameInfo.startIndex == 320);
        assert(frameInfo.endIndex == 1021);
        assert(frameInfo.complete == true);
        assert(frameInfo.frameTypeId == IFrameData::Type::FRAME_TYPE_ASCII);

        return 0;
    }

    int test_detect_ascii_incomplete() {

        FrameDetector frameDetector(SerialFrameBuffer::getFrameDelimiters(), 500);
        CircularBuffer buffer(1024);
        FrameInfo frameInfo;
        unsigned long currentTime = 1000;

        for (int i = 0; i < sizeof(ascii_incomplete_frame); i++) {
            buffer.addByte(ascii_incomplete_frame[i], currentTime);
            currentTime += 100;
        }

        const bool found = frameDetector.detect(buffer, currentTime, frameInfo);
        assert(found != true);
        assert(frameInfo.complete != true);

        return 0;
    }

    int test_detect_mbus() {

        FrameDetector frameDetector(SerialFrameBuffer::getFrameDelimiters(), 500);
        CircularBuffer buffer(1024);
        FrameInfo frameInfo;
        unsigned long currentTime = 1000;

        for (int i = 0; i < sizeof(mbus_frame); i++) {
            buffer.addByte(mbus_frame[i], currentTime);
            currentTime += 100;
        }

        const bool found = frameDetector.detect(buffer, currentTime, frameInfo);
        assert(found == true);
        assert(frameInfo.startIndex == 0);
        assert(frameInfo.frameTypeId == IFrameData::Type::FRAME_TYPE_MBUS);

        return 0;
    }


    int run() {
        test_detect_aidon();
        test_detect_ascii();
        test_detect_ascii_incomplete();
        test_detect_mbus();
        return 0;
    }
}