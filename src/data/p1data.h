#ifndef P1DATA_H
#define P1DATA_H

#include <Arduino.h>
#include "crypto.h"
#include <time.h>

// P1Data class to store P1 meter data
class P1Data {
public:

    static const char BUFFER_SIZE = 32;

    // Meter identification
    char szDeviceId[BUFFER_SIZE];             // Device ID
    char szMeterModel[BUFFER_SIZE];           // Meter model/manufacturer
    
    // Energy readings
    float electricityDeliveredTariff1;   // Total electricity delivered tariff 1 (kWh)
    float electricityDeliveredTariff2;   // Total electricity delivered tariff 2 (kWh)
    float electricityReturnedTariff1;    // Total electricity returned tariff 1 (kWh)
    float electricityReturnedTariff2;    // Total electricity returned tariff 2 (kWh)
    
    // Power readings
    float currentPowerDelivery;          // Current power delivery (kW)
    float currentPowerReturn;            // Current power return (kW)
    
    // Gas readings (if available)
    float gasDelivered;                  // Total gas delivered (m3)
    String gasTimestamp;                 // Timestamp of last gas reading
    
    // Instantaneous voltage and current
    float voltageL1;                     // Voltage on L1 (V)
    float voltageL2;                     // Voltage on L2 (V)
    float voltageL3;                     // Voltage on L3 (V)
    float currentL1;                     // Current on L1 (A)
    float currentL2;                     // Current on L2 (A)
    float currentL3;                     // Current on L3 (A)
    
    // Timestamp
    time_t timestamp;                    // Timestamp of the reading
    
    // Constructor
    P1Data() : 
        electricityDeliveredTariff1(0),
        electricityDeliveredTariff2(0),
        electricityReturnedTariff1(0),
        electricityReturnedTariff2(0),
        currentPowerDelivery(0),
        currentPowerReturn(0),
        gasDelivered(0),
        voltageL1(0),
        voltageL2(0),
        voltageL3(0),
        currentL1(0),
        currentL2(0),
        currentL3(0),
        timestamp(0) {}
};

// Function to create the P1 meter JWT (original version)
String createP1JWT(const char* privateKey, const String& deviceId);

// Function to create the P1 meter JWT with P1Data included
String createP1JWT(const char* privateKey, const String& deviceId, const P1Data& p1data);

#endif