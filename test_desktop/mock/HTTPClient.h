#pragma once

#include <Arduino.h>

#include <stdint.h>
#include <cstring>


class WiFiClient {
     
    public:
        static char* read_buffer;

        int available() { 
            if (read_buffer == nullptr) {
                return 0;
            }
            return strlen(read_buffer) - pos;
        }
        int read() { return (int)read_buffer[pos++]; }

        int pos = 0;
};

class HTTPClient {
    public:
        void setTimeout(uint16_t timeout) {}
        bool begin(const char* endpoint) { return true; }
        void addHeader(const char* header, const char* value) {}
        int POST(const char* body) { return 200; }
        WiFiClient* getStreamPtr() { return new WiFiClient(); } // TODO: this will leak memory
        void end() {}
};