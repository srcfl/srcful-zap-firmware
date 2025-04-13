#ifndef P1_METER_H
#define P1_METER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <vector>
#include <memory>

// Default pin and rate configuration
#define P1_DEFAULT_RX_PIN      20     // Default RX pin for P1 port
#define P1_DEFAULT_DTR_PIN     -1      // Default Data Terminal Ready pin
#define P1_DEFAULT_BAUD_RATE   115200 // Default baud rate for P1 port
#define P1_DEFAULT_BUFFER_SIZE 1024   // Default buffer size for reading P1 data

class P1Meter {
public:
    // Constructor with default values
    P1Meter(int rxPin = P1_DEFAULT_RX_PIN, 
            int dtrPin = P1_DEFAULT_DTR_PIN, 
            int baudRate = P1_DEFAULT_BAUD_RATE);
    
    ~P1Meter();
    
    // Initialize the meter
    bool begin();
    
    // Process available data (non-blocking)
    bool update();
    
    // Get the detected protocol name
    String getProtocolName() const;
    
    // Get the current buffer and buffer size (used for protocol detection)
    uint8_t* getBuffer() { return _buffer; }
    int getBufferSize() const { return _bufferSize; }
    int getBufferIndex() const { return _bufferIndex; }
    
    // Add data to buffer (used by reader for protocol detection)
    void addToBuffer(uint8_t byte);
    
    // Clear the buffer
    void clearBuffer();
    
    // Get the serial interface
    HardwareSerial* getSerial() { return &_serial; }

private:
    // Hardware configuration
    int _rxPin;
    int _dtrPin;
    int _baudRate;
    HardwareSerial _serial;
    
    // Data buffer
    uint8_t* _buffer;
    int _bufferSize;
    int _bufferIndex;
    
    // Protocol detection
    // bool _protocolDetected;
    // std::unique_ptr<P1ReaderInterface> _reader;
    
    unsigned long _lastDataTime;
    
    // Attempt to auto-detect protocol if needed
    bool detectProtocol();
};

#endif // P1_METER_H