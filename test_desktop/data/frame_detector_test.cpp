
#include "../src/data/frame_detector.cpp"
#include "../frames.h"

#include <assert.h>

namespace frame_detector_test {


    int test_detect_aidon() {

        std::vector<FrameDelimiterInfo> delimiter_pairs = {
            FrameDelimiterInfo('/', '!', 0, true), // Start and end delimiter for ascii
            FrameDelimiterInfo(0x7e, 0x7e, 1, false) // Start and end delimiter for aidon
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
            FrameDelimiterInfo('/', '!', 0, true), // Start and end delimiter for ascii
            FrameDelimiterInfo(0x7e, 0x7e, 1, false) // Start and end delimiter for aidon
        };

        FrameDetector frameDetector(delimiter_pairs, 500);
        CircularBuffer buffer(1024);
        FrameInfo frameInfo;
        unsigned long currentTime = 1000;

        for (int i = 0; i < sizeof(ascii_frame); i++) {
            buffer.addByte(ascii_frame[i], currentTime);
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
            FrameDelimiterInfo('/', '!', 0, true), // Start and end delimiter for ascii
            FrameDelimiterInfo(0x7e, 0x7e, 1, false) // Start and end delimiter for aidon
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


    int run() {
        test_detect_aidon();
        test_detect_ascii();
        test_detect_ascii_incomplete();
        return 0;
    }
}