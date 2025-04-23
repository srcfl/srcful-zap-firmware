#pragma once

unsigned long millis();

class SerialClass {
    public:
        void print(const char* str) {}
        void println() {}
        void println(const char* str) {}
        void printf(const char* format, ...) {}
};



extern SerialClass Serial;