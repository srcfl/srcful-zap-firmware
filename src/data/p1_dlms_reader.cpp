#include "p1_dlms_reader.h"
#include <Arduino.h>
#include <vector>
#include <string>
#include <algorithm>

// Define constants for P1 DLMS protocol
#define P1_STX '/'     // Start of telegram character
#define P1_ETX '!'     // End of telegram character
#define P1_CR '\r'     // Carriage return
#define P1_LF '\n'     // Line feed
#define P1_BAUDRATE 115200

P1DLMSReader::P1DLMSReader(HardwareSerial& serial) : 
    _serial(serial), 
    _isInitialized(false), 
    _buffer(""),
    _lastReadTime(0) {
}

bool P1DLMSReader::begin(int rxPin, int txPin) {
    _serial.begin(P1_BAUDRATE, SERIAL_8N1, rxPin, txPin);
    _isInitialized = true;
    _buffer.reserve(MAX_TELEGRAM_SIZE);
    return _isInitialized;
}

bool P1DLMSReader::available() {
    return _serial.available() > 0;
}

bool P1DLMSReader::readTelegram() {
    if (!_isInitialized) {
        return false;
    }

    // Check if we have data available
    if (!available()) {
        return false;
    }

    // Read data from serial port
    uint32_t startTime = millis();
    bool startFound = false;
    bool endFound = false;

    _buffer = "";

    // Wait for the start character
    while (millis() - startTime < TELEGRAM_TIMEOUT_MS && !startFound) {
        if (_serial.available()) {
            char c = _serial.read();
            if (c == P1_STX) {
                startFound = true;
                _buffer += c;
            }
        }
    }

    if (!startFound) {
        return false;
    }

    // Read the telegram contents
    startTime = millis();
    while (millis() - startTime < TELEGRAM_TIMEOUT_MS && !endFound) {
        if (_serial.available()) {
            char c = _serial.read();
            _buffer += c;
            
            // Check if we've found the end of the telegram
            if (c == P1_ETX) {
                // Read the checksum (usually follows after the ETX character)
                startTime = millis();
                while (_serial.available() == 0 && millis() - startTime < 500) {
                    // Wait for checksum data
                    delay(10);
                }
                
                // Read remaining checksum characters
                while (_serial.available() && _buffer.length() < MAX_TELEGRAM_SIZE) {
                    char c = _serial.read();
                    _buffer += c;
                    
                    if (c == P1_CR || c == P1_LF) {
                        endFound = true;
                        break;
                    }
                }
            }
        }
    }

    if (!endFound) {
        _buffer = "";
        return false;
    }

    _lastReadTime = millis();
    return true;
}

bool P1DLMSReader::parseTelegram(MeterData& data) {
    if (_buffer.length() == 0) {
        return false;
    }

    data = {}; // Reset the data structure
    
    // Parse the telegram line by line
    int pos = 0;
    int prev = 0;
    String line;
    
    while ((pos = _buffer.indexOf("\r\n", prev)) != -1) {
        line = _buffer.substring(prev, pos);
        prev = pos + 2;
        
        // Process the line
        processLine(line, data);
    }
    
    // Parse the last line if any
    if (prev < _buffer.length()) {
        line = _buffer.substring(prev);
        processLine(line, data);
    }
    
    return true;
}

void P1DLMSReader::processLine(const String& line, MeterData& data) {
    // Skip empty lines
    if (line.length() == 0) {
        return;
    }
    
    // Check for OBIS codes in the line
    // Examples of OBIS codes:
    // 1-0:1.8.0 - Total energy consumed
    // 1-0:2.8.0 - Total energy produced
    // 1-0:1.7.0 - Current power consumption
    // 1-0:2.7.0 - Current power production
    
    // Check for total energy consumed
    if (line.indexOf("1-0:1.8.0") != -1) {
        data.energyConsumed = extractValueWithUnit(line, "kWh");
    }
    // Check for total energy produced
    else if (line.indexOf("1-0:2.8.0") != -1) {
        data.energyProduced = extractValueWithUnit(line, "kWh");
    }
    // Check for current power consumption
    else if (line.indexOf("1-0:1.7.0") != -1) {
        data.powerCurrent = extractValueWithUnit(line, "kW");
    }
    // Check for current power production
    else if (line.indexOf("1-0:2.7.0") != -1) {
        data.powerProduction = extractValueWithUnit(line, "kW");
    }
    // Check for gas consumption (this varies by meter type)
    else if (line.indexOf("0-1:24.2.1") != -1) {
        data.gasConsumed = extractValueWithUnit(line, "m3");
    }
}

float P1DLMSReader::extractValueWithUnit(const String& line, const String& unit) {
    int startPos = line.indexOf("(");
    int endPos = line.indexOf(unit);
    
    if (startPos != -1 && endPos != -1 && startPos < endPos) {
        String valueStr = line.substring(startPos + 1, endPos);
        valueStr.trim();
        return valueStr.toFloat();
    }
    
    return 0.0f;
}

String P1DLMSReader::getRawTelegram() const {
    return _buffer;
}

uint32_t P1DLMSReader::getLastReadTime() const {
    return _lastReadTime;
}
