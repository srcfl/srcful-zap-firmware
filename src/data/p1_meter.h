#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <vector>
#include <memory>
#include "../zap_str.h"
#include "serial_frame_buffer.h"

// Default pin and rate configuration
#define P1_DEFAULT_RX_PIN      20     // Default RX pin for P1 port
#define P1_DEFAULT_DTR_PIN     -1      // Default Data Terminal Ready pin
// #define P1_DEFAULT_RX_PIN      10     // Default RX pin for P1 port
// #define P1_DEFAULT_DTR_PIN     6      // Default Data Terminal Ready pin

#define P1_DEFAULT_BAUD_RATE   115200 // Default baud rate for P1 port
#define P1_DEFAULT_BUFFER_SIZE 1024   // Default buffer size for reading P1 data

class P1Meter {
public:
    // Frame received callback
    using FrameReceivedCallback = std::function<void(const IFrameData&)>;
    
    // Constructor with default values
    P1Meter(int rxPin = P1_DEFAULT_RX_PIN, 
            int dtrPin = P1_DEFAULT_DTR_PIN, 
            int baudRate = P1_DEFAULT_BAUD_RATE);
    
    ~P1Meter();
    
    // Initialize the meter
    bool begin();
    
    // Process available data (non-blocking)
    bool update();
    
    // Get buffer size and usage
    int getBufferSize() const;
    int getBufferUsed() const;
    
    // Clear the buffer
    void clearBuffer();
    
    // Set frame callback
    void setFrameCallback(FrameReceivedCallback callback);
    
    // Get the serial interface
    HardwareSerial* getSerial() { return &_serial; }

private:
    // Hardware configuration
    int _rxPin;
    int _dtrPin;
    int _baudRate;
    HardwareSerial _serial;
    
    // Frame buffer for robust serial data handling
    SerialFrameBuffer _frameBuffer;
    
    // Protocol detection
    // bool _protocolDetected;
    // std::unique_ptr<P1ReaderInterface> _reader;
    
    unsigned long _lastDataTime;
    FrameReceivedCallback _frameCallback;
    
    // Internal frame callback
    bool onFrameDetected(const IFrameData& frame);
};
