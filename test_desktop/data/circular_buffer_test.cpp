#include <assert.h>

#include "../src/data/circular_buffer.h"

namespace circular_buffer_test {

    int test_constructor() {
        CircularBuffer buffer(10);
        assert(buffer.getBufferSize() == 10);
        assert(buffer.getReadIndex() == 0);
        assert(buffer.getWriteIndex() == 0);
        assert(buffer.getLastByteTime() == 0);
        return 0;
    }

    int test_add_bytes() {
        CircularBuffer buffer(10);
        unsigned long currentTime = 1000;
        for (int i = 0; i < 10; i++) {
            assert(buffer.addByte(i, currentTime) == true);
            assert(buffer.getLastByteTime() == currentTime);
            assert(buffer.getByteAt(i) == i);
            currentTime += 100;
        }

        assert(buffer.getReadIndex() == 0);
        assert(buffer.getWriteIndex() == 0);
        assert(buffer.getOverflowCount() == 0);

        return 0;
    }

    int test_overflow() {
        CircularBuffer buffer(5);
        unsigned long currentTime = 1000;
        for (int i = 0; i < 10; i++) {
            assert(buffer.addByte(i, currentTime) == true);
            assert(buffer.getLastByteTime() == currentTime);
            assert(buffer.getByteAt(i % 5) == i);
            currentTime += 100;
        }

        assert(buffer.getReadIndex() == 0);
        assert(buffer.getWriteIndex() == 0);
        assert(buffer.getOverflowCount() == 5);

        return 0;
    }
    int test_clear() {
        CircularBuffer buffer(10);
        unsigned long currentTime = 1000;
        for (int i = 0; i < 5; i++) {
            assert(buffer.addByte(i, currentTime) == true);
            currentTime += 100;
        }

        buffer.clear(currentTime);
        assert(buffer.getReadIndex() == 0);
        assert(buffer.getWriteIndex() == 0);
        assert(buffer.getLastByteTime() == currentTime);
        assert(buffer.available() == 0);

        return 0;
    }
    int test_advance_read_index() {
        CircularBuffer buffer(10);
        unsigned long currentTime = 1000;
        for (int i = 0; i < 5; i++) {
            assert(buffer.addByte(i, currentTime) == true);
            currentTime += 100;
        }

        buffer.advanceReadIndex(2);
        assert(buffer.getReadIndex() == 2);
        assert(buffer.getWriteIndex() == 5);
        assert(buffer.getLastByteTime() == currentTime - 100);
        assert(buffer.available() == 3);

        return 0;
    }
    

    int run(){
        
        test_constructor();
        test_add_bytes();
        test_overflow();
        test_clear();
        test_advance_read_index();


        return 0;
    }
}