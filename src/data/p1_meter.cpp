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

// Constructor updated to include output pins
P1Meter::P1Meter(int rxPin, int dtrPin, int baudRate, int txOutPin, int dtrOutPin, int ledPin)
    : _rxPin(rxPin),
      _dtrPin(dtrPin),
      _baudRate(baudRate),
      _txOutPin(txOutPin),
      _dtrOutPin(dtrOutPin),
      _ledPin(ledPin),
      _serial(1), // Use UART1 for input
      _outputSerial(2), // Use UART2 for output
      _frameBuffer(P1_DEFAULT_BUFFER_SIZE, P1_DEFAULT_START_CHAR, P1_DEFAULT_END_CHAR, P1_FRAME_TIMEOUT),
      _lastDataTime(0),
      _frameCallback(nullptr) {
    
    LOG_I(TAG, "P1Meter constructor called with output forwarding support");
    
    // Set up frame buffer callback
    _frameBuffer.setFrameCallback([this](const IFrameData& frame) -> bool {
        return this->onFrameDetected(frame);
    });
}

P1Meter::~P1Meter() {
    LOG_I(TAG, "P1Meter destructor called");
    if (_dtrOutPin >= 0) {
        digitalWrite(_dtrOutPin, LOW); // Optionally turn off DTR for output device
    }
    if (_ledPin >= 0) {
        digitalWrite(_ledPin, HIGH); // Ensure LED is off
    }
}

bool P1Meter::begin() {
    LOG_I(TAG, "Initializing P1 meter with output forwarding...");
    
    // Configure DTR pin for input P1 port
    if (_dtrPin >= 0) {
        pinMode(_dtrPin, OUTPUT);
        digitalWrite(_dtrPin, HIGH); // Enable P1 input port
        LOG_D(TAG, "Set input DTR pin %d HIGH", _dtrPin);
    }
    
    // Initialize input serial (UART1)
    _serial.setRxBufferSize(2048);
    _serial.begin(_baudRate, SERIAL_8N1, _rxPin, -1, true); // RX on _rxPin, TX for UART1 not used for input
    LOG_I(TAG, "Initialized input UART1 with baud rate %d, RX pin %d", _baudRate, _rxPin);

    // Initialize output serial (UART2)
    // RX for UART2 not used for output, TX on _txOutPin
    _outputSerial.begin(_baudRate, SERIAL_8N1, -1, _txOutPin); 
    LOG_I(TAG, "Initialized output UART2 with baud rate %d, TX pin %d", _baudRate, _txOutPin);

    // Configure DTR pin for output device (as GPIO)
    if (_dtrOutPin >= 0) {
        pinMode(_dtrOutPin, OUTPUT);
        digitalWrite(_dtrOutPin, HIGH); // Set DTR for output device (e.g., active)
        LOG_D(TAG, "Set output DTR pin %d HIGH", _dtrOutPin);
    }

    // Configure LED pin
    if (_ledPin >= 0) {
        pinMode(_ledPin, OUTPUT);
        digitalWrite(_ledPin, HIGH); // Assuming HIGH is LED OFF initially
        LOG_D(TAG, "Initialized LED pin %d", _ledPin);
    }
    
    // Reset state
    clearBuffer();
    _lastDataTime = millis();
    
    LOG_I(TAG, "P1 meter with output forwarding initialized successfully");
    return true;
}

bool P1Meter::update() {
    bool dataProcessed = false;
    int availableBytes;
    static const int localBufferSize = 256; // Smaller buffer for chunking reads/writes
    static uint8_t buffer[localBufferSize];
    size_t totalBytesReadThisUpdate = 0;
    static unsigned long ledOnTime = 0; // LED state for toggling
    
    // Read available data in chunks
    while ((availableBytes = _serial.available()) > 0) {
        digitalWrite(_ledPin, HIGH); // Turn LED ON
        size_t bytesToRead = (availableBytes > localBufferSize) ? localBufferSize : availableBytes;
        size_t bytesActuallyRead = _serial.readBytes(buffer, bytesToRead);
        ledOnTime = millis(); // Update LED ON time

        if (bytesActuallyRead > 0) {
            _lastDataTime = millis();
            totalBytesReadThisUpdate += bytesActuallyRead;

            // 1. Forward data to _outputSerial
            // if (_ledPin >= 0) {
            // }
            _outputSerial.write(buffer, bytesActuallyRead);
            // if (_ledPin >= 0) {
            // }
            // LOG_I(TAG, "Forwarded %i bytes to output serial", bytesActuallyRead);

            // 2. Add data to frameBuffer for P1 frame processing
            if (_frameBuffer.addData(buffer, bytesActuallyRead, _lastDataTime)) {
                dataProcessed = true; // Indicates frame buffer processed something (might have completed a frame)
            }
        } else {
            // readBytes returned 0, break to avoid busy loop if _serial.available() is buggy
            break; 
        }
    }
    
    // Check for timeouts and process any frames that might be complete due to timeout
    if (_frameBuffer.processBufferForFrames(millis())) {
       
        dataProcessed = true;
    }

    if (!dataProcessed) {
        // Check if LED should be turned off
        if (_ledPin >= 0 && (millis() - ledOnTime > 500)) { // Turn off after 1 second of inactivity
            digitalWrite(_ledPin, LOW); // Turn LED OFF
        }
    }
    
    return dataProcessed; // Returns true if data was added to frame buffer or a frame was processed
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
    
    return true;
}