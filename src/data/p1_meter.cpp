#include "p1_meter.h"
#include <Arduino.h>
#include <memory>
#include "../zap_log.h" // Added for logging

// Define TAG for logging
static const char* TAG = "p1_meter";

// P1 frame delimiters (most common)

#define P1_FRAME_TIMEOUT 500 // ms between bytes in same frame

// Constructor updated to include output pins
P1Meter::P1Meter(int rxPin, int dtrPin, int txOutPin, int dtrOutPin, int ledPin)
    : _rxPin(rxPin),
      _dtrPin(dtrPin),
      _txOutPin(txOutPin),
      _dtrOutPin(dtrOutPin),
      _ledPin(ledPin),
      _serial(1), // Use UART1 for input
      _frameBuffer(P1_DEFAULT_BUFFER_SIZE, P1_FRAME_TIMEOUT),
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
        digitalWrite(_dtrOutPin, HIGH); // Optionally turn off DTR for output device
    }
    if (_ledPin >= 0) {
        digitalWrite(_ledPin, HIGH); // Ensure LED is off
    }
}

static const P1Meter::Config CONFIGS[] = {
    {2400, SERIAL_8N1},     // some mbus meters 
    {2400, SERIAL_8E1},     // some mbus meters 
    {9600, SERIAL_7E1},
    {115200, SERIAL_8N1}    // most modern meters 
};
const P1Meter::Config* P1Meter::configs() const {
   return CONFIGS;   
}

size_t P1Meter::getNumConfigs() const {
    return sizeof(CONFIGS) / sizeof(Config);
}

const P1Meter::Config& P1Meter::getConfig(size_t index) const {
    if (index < getNumConfigs()) {
        return CONFIGS[index];
    }
    LOG_E(TAG, "Invalid config index %zu", index);
    return CONFIGS[0]; // Return default config if index is invalid
}

bool P1Meter::begin(const Config& config) {
    LOG_I(TAG, "Initializing P1 meter with output forwarding...");

    _serial.end();
    delay(100);
    
    // Configure DTR pin for input P1 port
    if (_dtrPin >= 0) {
        pinMode(_dtrPin, OUTPUT); delay(100);
        digitalWrite(_dtrPin, HIGH); delay(100);// Enable P1 input port
        LOG_D(TAG, "Set input DTR pin %d HIGH", _dtrPin);
    }

    // Initialize output serial (UART2)
    // RX for UART2 not used for output, TX on _txOutPin

    // Configure DTR pin for output device (as GPIO)
    if (_dtrOutPin >= 0) {
        pinMode(_dtrOutPin, OUTPUT); delay(100);
        digitalWrite(_dtrOutPin, HIGH); delay(100);// Set DTR for output device (e.g., active)
        LOG_D(TAG, "Set output DTR pin %d HIGH", _dtrOutPin);
        
    }

    // Initialize input serial (UART1)
    _serial.setRxBufferSize(2048); delay(100);
    _serial.setTxBufferSize(2048); delay(100);
    _serial.begin(config.baudRate, config.config, _rxPin, _txOutPin); delay(100);// RX on _rxPin, TX for UART1 not used for input
    _serial.setRxInvert(true); delay(100); // Invert RX signal for P1 meter compatibility
    LOG_I(TAG, "Initialized input UART1 with baud rate %d, config %d", config.baudRate, config.config);

    // Configure LED pin
    if (_ledPin >= 0) {
        pinMode(_ledPin, OUTPUT); delay(100);
        digitalWrite(_ledPin, LOW); delay(100);
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

            if (_frameBuffer.addData(buffer, bytesActuallyRead, _lastDataTime)) {
                dataProcessed = true; // Indicates frame buffer processed something (might have completed a frame)
            }

            // LOG_I(TAG, "Read %i bytes from input serial: %s", bytesActuallyRead, zap::Str(buffer[0], 16).c_str());

            // we now invert the signal to output serial
            // for (int i = 0; i < bytesActuallyRead; i++) {
            //     buffer[i] = ~buffer[i]; // Invert the byte for output
            // }
            size_t written = _serial.write(buffer, bytesActuallyRead);
            // if (written != bytesActuallyRead) {
            //     LOG_I(TAG, "Failed to write all bytes to output serial. Expected %i, wrote %i", bytesActuallyRead, written);
            // } else {
            //     LOG_I(TAG, "Wrote %i bytes to output serial", written);
            // }
            //

        } else {
            // readBytes returned 0, break to avoid busy loop if _serial.available() is buggy
            break; 
        }
    }

    // _serial.flush(); // Ensure data is sent immediately
    
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

    // {
    //     static unsigned long deadbeefTimer = millis();
    //     if (millis() - deadbeefTimer > 10000) { // Every second
    //         deadbeefTimer = millis();
    //         uint8_t deadbeef[] = {0xDE, 0xAD, 0xBE, 0xEF};

    //         // for (int i = 0; i < sizeof(deadbeef); i++) {
    //         //     deadbeef[i] = ~deadbeef[i]; // Fill buffer with deadbeef pattern
    //         // }

    //         for (int i = 0; i < 1; i++) {
    //             _serial.write(deadbeef, 4);
    //         }
    //             _serial.flush(); // Ensure data is sent immediately
    //         LOG_I(TAG, "Writing deadbeef pattern to output serial");
    //     }
    // }
    
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

