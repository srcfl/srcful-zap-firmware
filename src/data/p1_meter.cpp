#include "p1_meter.h"
#include <Arduino.h>
#include <memory>
#include "../zap_log.h" // Added for logging

// Define TAG for logging
static const char* TAG = "p1_meter";

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
      _frameBuffer(P1_DEFAULT_BUFFER_SIZE, P1_DEFAULT_START_CHAR, P1_DEFAULT_END_CHAR, P1_FRAME_TIMEOUT), // Removed serialPortSpeed from this call
      _lastDataTime(0),
      _frameCallback(nullptr) {
    
    LOG_I(TAG, "P1Meter constructor called");
    
    // Set up frame buffer callback
    _frameBuffer.setFrameCallback([this](const IFrameData& frame) -> bool {
        return this->onFrameDetected(frame);
    });
}

P1Meter::~P1Meter() {
    LOG_I(TAG, "P1Meter destructor called");
}

bool P1Meter::begin() {
    LOG_I(TAG, "Initializing P1 meter...");
    
    // Configure DTR pin
    if (_dtrPin >= 0) {
        pinMode(_dtrPin, OUTPUT);
        digitalWrite(_dtrPin, HIGH); // Enable P1 port
        LOG_D(TAG, "Set DTR pin %d HIGH", _dtrPin);
    }
    
    // Initialize serial
    // try {
        _serial.setRxBufferSize(2048); // Set RX buffer size
        _serial.begin(_baudRate, SERIAL_8N1, _rxPin, -1, true); // Inverted logic
        LOG_I(TAG, "Initialized UART1 with baud rate %d, RX pin %d", _baudRate, _rxPin);
    // } catch (...) {
    //     LOG_E(TAG, "EXCEPTION: Failed to initialize P1 serial");
    //     return false;
    // }
    
    // Reset state
    clearBuffer();
    // _protocolDetected = false;
    _lastDataTime = millis();
    
    LOG_I(TAG, "P1 meter initialized successfully");
    return true;
}

bool P1Meter::update() {
    bool dataProcessed = false;
    int availableBytes;
    static const int bufferSize = 1024;
    static uint8_t buffer[bufferSize];
    size_t readBytes = 0;
    
    // Read available data
    while ((availableBytes = _serial.available()) > 0) {
        LOG_V(TAG, "Available bytes: %d", availableBytes);
        const int leftInBuffer = bufferSize - readBytes;
        if (leftInBuffer <= 0) {
            break;
        }

        readBytes += _serial.readBytes(&buffer[readBytes], availableBytes < leftInBuffer ? availableBytes : leftInBuffer);
        // LOG_V(TAG, "Read bytes: %d", readBytes);
        
        // LOG_V(TAG, "%02X ", inByte); // Example for logging hex bytes if needed
        // Process byte through frame buffer
        // if (_frameBuffer.addByte(inByte)) {
        //     dataProcessed = true;
        // }
        _lastDataTime = millis();
    }

    if (readBytes) {
        
        LOG_V(TAG, "Processing read bytes: %d", readBytes);
        if (_frameBuffer.addData(buffer, readBytes, _lastDataTime)) {
            dataProcessed = true;
        }
    }

    
    // Check for timeouts and process any frames that might be complete
    if (_frameBuffer.processBufferForFrames(millis())) {
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
    //         LOG_E(TAG, "ERROR: Exception in reader processData()");
    //         return false;
    //     }
    // }
    
    return dataProcessed;
}

int P1Meter::getBufferSize() const {
    return P1_DEFAULT_BUFFER_SIZE;
}

int P1Meter::getBufferUsed() const {
    return _frameBuffer.available(); // Return number of bytes in frame buffer
}

void P1Meter::clearBuffer() {
    // Clear frame buffer
    _frameBuffer.clear(millis());
}

void P1Meter::setFrameCallback(FrameReceivedCallback callback) {
    _frameCallback = callback;
}

bool P1Meter::onFrameDetected(const IFrameData& frame) {
   
    LOG_D(TAG, "P1 frame detected (%zu bytes)", frame.getFrameSize());
    
    // Call user-provided frame callback if available
    if (_frameCallback) {
        _frameCallback(frame);
    }
    
    // Here would be a good place to integrate with protocol detection or decoding
    
    return true;
}