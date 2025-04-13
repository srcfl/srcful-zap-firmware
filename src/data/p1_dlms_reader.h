#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <string>

// Constants for P1 DLMS reader
#define MAX_TELEGRAM_SIZE 1024
#define TELEGRAM_TIMEOUT_MS 10000

// Structure to hold parsed meter data
struct MeterData {
    float energyConsumed = 0.0f;   // Total energy consumed (kWh)
    float energyProduced = 0.0f;   // Total energy produced/returned (kWh)
    float powerCurrent = 0.0f;     // Current power consumption (kW)
    float powerProduction = 0.0f;  // Current power production (kW)
    float gasConsumed = 0.0f;      // Gas consumption (m³)
};

/**
 * P1DLMSReader - Class to read and parse P1 DLMS telegrams from smart meters
 */
class P1DLMSReader {
public:
    /**
     * Constructor
     * @param serial Hardware serial port to use
     */
    P1DLMSReader(HardwareSerial& serial);
    
    /**
     * Initialize the P1 DLMS reader
     * @param rxPin RX pin for UART
     * @param txPin TX pin for UART
     * @return true if initialization was successful
     */
    bool begin(int rxPin, int txPin);
    
    /**
     * Check if data is available to read
     * @return true if data is available
     */
    bool available();
    
    /**
     * Read a complete telegram from the P1 port
     * @return true if a complete telegram was read
     */
    bool readTelegram();
    
    /**
     * Parse the telegram data into a structured format
     * @param data Reference to a MeterData struct to store parsed values
     * @return true if parsing was successful
     */
    bool parseTelegram(MeterData& data);
    
    /**
     * Get the raw telegram string
     * @return String containing the raw telegram
     */
    String getRawTelegram() const;
    
    /**
     * Get the timestamp of the last successful read
     * @return Timestamp in milliseconds
     */
    uint32_t getLastReadTime() const;

private:
    HardwareSerial& _serial;
    bool _isInitialized;
    String _buffer;
    uint32_t _lastReadTime;
    
    /**
     * Process a single line from the telegram
     * @param line Line to process
     * @param data Reference to MeterData to update
     */
    void processLine(const String& line, MeterData& data);
    
    /**
     * Extract a float value with a specific unit from a line
     * @param line Line to extract from
     * @param unit Unit string to look for
     * @return Extracted float value
     */
    float extractValueWithUnit(const String& line, const String& unit);
};