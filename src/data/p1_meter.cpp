#include "p1_meter.h"
#include <Arduino.h>
#include <memory>

// Timeout for protocol detection (ms)
#define PROTOCOL_DETECTION_TIMEOUT 5000

P1Meter::P1Meter(int rxPin, int dtrPin, int baudRate)
    : _rxPin(rxPin),
      _dtrPin(dtrPin),
      _baudRate(baudRate),
      _serial(1), // Use UART1
      _bufferSize(P1_DEFAULT_BUFFER_SIZE),
      _bufferIndex(0),
    //   _protocolDetected(false),
      _lastDataTime(0) {
    
    Serial.println("P1Meter constructor called");
    
    // Allocate buffer with null check
    try {
        _buffer = new uint8_t[_bufferSize];
        if (_buffer == nullptr) {
            Serial.println("ERROR: Failed to allocate P1 buffer");
            // We'll handle this in begin()
        } else {
            clearBuffer();
        }
    } catch (...) {
        Serial.println("EXCEPTION: Failed to allocate P1 buffer");
        _buffer = nullptr;
    }
}

P1Meter::~P1Meter() {
    Serial.println("P1Meter destructor called");
    // Free buffer
    if (_buffer != nullptr) {
        delete[] _buffer;
        _buffer = nullptr;
    }
}

bool P1Meter::begin() {
    Serial.println("Initializing P1 meter...");
    
    // Check if buffer allocation succeeded
    if (_buffer == nullptr) {
        Serial.println("ERROR: P1 buffer not allocated, cannot initialize");
        return false;
    }
    
    // Configure DTR pin
    if (_dtrPin >= 0) {
        pinMode(_dtrPin, OUTPUT);
        digitalWrite(_dtrPin, HIGH); // Enable P1 port
        Serial.printf("Set DTR pin %d HIGH\n", _dtrPin);
    }
    
    // Initialize serial
    try {
        _serial.begin(_baudRate, SERIAL_8N1, _rxPin, -1, true); // Inverted logic
        Serial.printf("Initialized UART1 with baud rate %d, RX pin %d\n", _baudRate, _rxPin);
    } catch (...) {
        Serial.println("EXCEPTION: Failed to initialize P1 serial");
        return false;
    }
    
    // Reset state
    clearBuffer();
    // _protocolDetected = false;
    _lastDataTime = millis();
    
    Serial.println("P1 meter initialized successfully");
    return true;
}

bool P1Meter::update() {
    if (_buffer == nullptr) {
        return false;
    }
    
    bool dataProcessed = false;
    
    // Read available data
    while (_serial.available()) {
        uint8_t inByte = _serial.read();
        addToBuffer(inByte);
        _lastDataTime = millis();
    }
    
    // Try to detect protocol if not detected yet
    // if (!_protocolDetected) {
    //     if (_bufferIndex > 30 || millis() - _lastDataTime > PROTOCOL_DETECTION_TIMEOUT) {
    //         detectProtocol();
    //     }
    // } else if (_reader) {
    //     // If protocol detected, process data with the appropriate reader
    //     try {
    //         if (_reader->hasNewData()) {
    //             dataProcessed = _reader->processData();
    //         }
    //     } catch (...) {
    //         Serial.println("ERROR: Exception in reader processData()");
    //         return false;
    //     }
    // }
    
    return dataProcessed;
}

String P1Meter::getProtocolName() const {
    // if (_protocolDetected && _reader) {
    //     return _reader->getProtocolName();
    // }
    
    return "Unknown";
}

void P1Meter::addToBuffer(uint8_t byte) {
    if (_buffer == nullptr) {
        return;
    }
    
    if (_bufferIndex < _bufferSize) {
        _buffer[_bufferIndex++] = byte;
    }
    else {
        Serial.println("Buffer overflow, clearing buffer");
        clearBuffer();
    }
}

void P1Meter::clearBuffer() {
    if (_buffer == nullptr) {
        return;
    }
    memset(_buffer, 0, _bufferSize);
    _bufferIndex = 0;
}

bool P1Meter::detectProtocol() {
    if (_buffer == nullptr || _bufferIndex == 0) {
        return false;
    }
    
    Serial.println("Attempting to detect P1 protocol...");
    
    // Use static factory method to detect protocol
    // try {
    //     _reader = P1ReaderInterface::autoDetectReader(this);
    // } catch (...) {
    //     Serial.println("EXCEPTION: Protocol detection failed");
    //     return false;
    // }
    
    // if (_reader) {
    //     _protocolDetected = true;
    //     _reader->begin();
        
    //     Serial.print("Protocol detected: ");
    //     Serial.println(_reader->getProtocolName());
    //     return true;
    // }
    
    Serial.println("Protocol detection failed, will retry");
    return false;
}