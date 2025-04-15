#include "p1_meter.h"
#include <Arduino.h>
#include <memory>

// Timeout for protocol detection (ms)
#define PROTOCOL_DETECTION_TIMEOUT 5000

// P1 frame delimiters (most common)
#define P1_DEFAULT_START_CHAR 0x7E
#define P1_DEFAULT_END_CHAR 0x7E
#define P1_FRAME_TIMEOUT 500 // ms between bytes in same frame

P1Meter::P1Meter(int rxPin, int dtrPin, int baudRate)
    : _rxPin(rxPin),
      _dtrPin(dtrPin),
      _baudRate(baudRate),
      _serial(1), // Use UART1
      _frameBuffer(P1_DEFAULT_BUFFER_SIZE, P1_DEFAULT_START_CHAR, P1_DEFAULT_END_CHAR, P1_FRAME_TIMEOUT),
      _lastDataTime(0),
      _frameCallback(nullptr) {
    
    Serial.println("P1Meter constructor called");
    
    // Set up frame buffer callback
    _frameBuffer.setFrameCallback([this](const IFrameData& frame) -> bool {
        return this->onFrameDetected(frame);
    });
}

P1Meter::~P1Meter() {
    Serial.println("P1Meter destructor called");
}

bool P1Meter::begin() {
    Serial.println("Initializing P1 meter...");
    
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
    bool dataProcessed = false;
    bool dataRecieved = false;
    
    // Read available data
    while (_serial.available()) {
        uint8_t inByte = _serial.read();
        Serial.print(inByte, HEX); Serial.print(" ");
        dataRecieved = true;
        // Process byte through frame buffer
        if (_frameBuffer.addByte(inByte)) {
            dataProcessed = true;
        }
        _lastDataTime = millis();
    }
    if (dataRecieved) {
        Serial.println();
    }

    
    // Check for timeouts and process any frames that might be complete
    if (_frameBuffer.update()) {
        dataProcessed = true;
    }
    
    // Try to detect protocol if needed
    // if (!_protocolDetected) {
    //     if (getBufferUsed() > 30 || millis() - _lastDataTime > PROTOCOL_DETECTION_TIMEOUT) {
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

int P1Meter::getBufferSize() const {
    return P1_DEFAULT_BUFFER_SIZE;
}

int P1Meter::getBufferUsed() const {
    return _frameBuffer.available(); // Return number of bytes in frame buffer
}

void P1Meter::clearBuffer() {
    // Clear frame buffer
    _frameBuffer.clear();
}

void P1Meter::setFrameCallback(FrameReceivedCallback callback) {
    _frameCallback = callback;
}

bool P1Meter::onFrameDetected(const IFrameData& frame) {
   
    Serial.printf("P1 frame detected (%zu bytes)\n", frame.getFrameSize());
    
    // Call user-provided frame callback if available
    if (_frameCallback) {
        _frameCallback(frame);
    }
    
    // Here would be a good place to integrate with protocol detection or decoding
    
    return true;
}

bool P1Meter::detectProtocol() {
    // Serial.println("Attempting to detect P1 protocol...");
    
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
    //     
    //     Serial.print("Protocol detected: ");
    //     Serial.println(_reader->getProtocolName());
    //     return true;
    // }
    
    Serial.println("Protocol detection not implemented yet");
    return false;
}