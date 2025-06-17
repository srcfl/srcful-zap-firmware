
#include "../frames.h"

#include "../src/data/decoding/IFrameData.h"
#include "../src/data/frame_detector.h"
#include "../src/data/mbus_frame_detector.h"

#include <assert.h>

namespace frame_detector_test {


    int test_detect_aidon() {

        std::vector<FrameDelimiterInfo> delimiter_pairs = {
            FrameDelimiterInfo('/', '!', IFrameData::Type::FRAME_TYPE_ASCII, true), // Start and end delimiter for ascii
            FrameDelimiterInfo(0x7e, 0x7e, IFrameData::Type::FRAME_TYPE_DLMS, false) // Start and end delimiter for aidon
        };

        FrameDetector frameDetector(delimiter_pairs, 500);
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

        return 0;
    }

    int test_detect_ascii() {
        std::vector<FrameDelimiterInfo> delimiter_pairs = {
            FrameDelimiterInfo('/', '!', IFrameData::Type::FRAME_TYPE_ASCII, true), // Start and end delimiter for ascii
            FrameDelimiterInfo(0x7e, 0x7e, IFrameData::Type::FRAME_TYPE_DLMS, false) // Start and end delimiter for aidon
        };

        FrameDetector frameDetector(delimiter_pairs, 500);
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

        return 0;
    }

    int test_detect_ascii_incomplete() {
        std::vector<FrameDelimiterInfo> delimiter_pairs = {
            FrameDelimiterInfo('/', '!', IFrameData::Type::FRAME_TYPE_ASCII, true), // Start and end delimiter for ascii
            FrameDelimiterInfo(0x7e, 0x7e, IFrameData::Type::FRAME_TYPE_DLMS, false) // Start and end delimiter for aidon
        };

        FrameDetector frameDetector(delimiter_pairs, 500);
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

        MbusFrameDetector frameDetector;
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
        assert(frameInfo.endIndex == sizeof(mbus_frame) - 1);
        assert(frameInfo.size == sizeof(mbus_frame));
        assert(frameInfo.complete == true);

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