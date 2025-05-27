#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <vector>
#include <memory>
#include "../zap_str.h"
#include "serial_frame_buffer.h"

// Default pin and rate configuration for P1 input
#define P1_DEFAULT_RX_PIN      20     
#define P1_DEFAULT_DTR_PIN     -1     
#define P1_DEFAULT_BAUD_RATE   115200 
#define P1_DEFAULT_BUFFER_SIZE 2048

// Configuration for P1 data output and LED
#define P1_OUTPUT_DEFAULT_TX_PIN  10    
#define P1_OUTPUT_DEFAULT_DTR_PIN 1     
#define P1_DEFAULT_LED_PIN        0     

class P1Meter {
public:
    using FrameReceivedCallback = std::function<void(const IFrameData&)>; 
    
    explicit P1Meter(int rxPin = P1_DEFAULT_RX_PIN, 
            int dtrPin = P1_DEFAULT_DTR_PIN, 
            int txOutPin = P1_OUTPUT_DEFAULT_TX_PIN, // New parameter
            int dtrOutPin = P1_OUTPUT_DEFAULT_DTR_PIN, // New parameter
            int ledPin = P1_DEFAULT_LED_PIN);      // New parameter
    
    ~P1Meter();
    
    bool begin(int baudRate = P1_DEFAULT_BAUD_RATE);
    bool update();
    
    int getBufferSize() const;
    int getBufferUsed() const;
    void clearBuffer();
    void setFrameCallback(FrameReceivedCallback callback);

private:
    int _rxPin;
    int _dtrPin;
    HardwareSerial _serial; 
    
    // New members for output and LED
    int _txOutPin;
    int _dtrOutPin;     
    int _ledPin;

    SerialFrameBuffer _frameBuffer;
    
    unsigned long _lastDataTime;
    FrameReceivedCallback _frameCallback; 
    
    bool onFrameDetected(const IFrameData& frame);
};
