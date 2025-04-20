
#include "../src/data/frame_detector.cpp"
#include "../frames.h"

#include <assert.h>

namespace frame_detector_test {


    int test_update() {
        FrameDetector frameDetector(aidon_test_buffer[0], aidon_test_buffer[0]);
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


    int run() {
        test_update();
        return 0;
    }
}