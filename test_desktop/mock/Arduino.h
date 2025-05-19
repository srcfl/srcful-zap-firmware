#pragma once

extern const long millis_default_return_value;
extern unsigned long millis_return_value;

unsigned long millis();

class SerialClass {
    public:
        void print(const char* str) {}
        void println() {}
        void println(const char* str) {}
        void printf(const char* format, ...) {}
};



extern SerialClass Serial;