#include "p1data_funcs.h"
#include "json_light/json_light.h"
#include "crypto.h"

String createP1JWT(const char* privateKey, const String& deviceId, const P1Data& p1data) {
    // Create the header
    JsonBuilder header;
    header.beginObject()
        .add("alg", "ES256")
        .add("typ", "JWT")
        .add("device", deviceId.c_str())
        .add("opr", "production")
        .add("model", "p1homewizard")
        .add("dtype", "p1_telnet_json")
        .add("sn", "LGF5E360");

    String headerStr = header.end();

    // Create the payload
    JsonBuilder payload;
    String timestampStr;
    
    timestampStr = String(p1data.timestamp);
    timestampStr += "000"; // Append '000' to the timestamp as in milliseconds
    
    // Start the payload object
    payload.beginObject();
    
    // Create a nested object for the timestamp
    JsonBuilder dataObj;
    dataObj.beginObject();
    

    dataObj.add("serial_number", p1data.szDeviceId);
    
    // Values to use for the P1 data
    float totalPower = 0.0;
    float powerL1 = 0.0, powerL2 = 0.0, powerL3 = 0.0;
    float reactivePowerL1 = 0.0, reactivePowerL2 = 0.0, reactivePowerL3 = 0.0;
    float totalReactivePower = 0.0;
    float voltageL1 = 0.0, voltageL2 = 0.0, voltageL3 = 0.0;
    float currentL1 = 0.0, currentL2 = 0.0, currentL3 = 0.0;
    float energyDelivered = 0.0;
    float reactiveEnergy1 = 0.0, reactiveEnergy2 = 0.0;
    
    // if (useSimulatedData || p1data == NULL) {
    //     // Use the existing simulation code
    //     float timeInSeconds = millis() / 1000.0;
        
    //     // Power simulation: Base load + daily cycle + random variation
    //     float basePower = 0.25; // 0.25 kW base load
        
    //     // Phase 1: ~1 hour cycle
    //     float phase1Cycle = sin(2 * PI * timeInSeconds / 3600) * 0.1; // ±0.1 kW variation
    //     // Phase 2: ~45 minutes cycle with offset
    //     float phase2Cycle = sin(2 * PI * timeInSeconds / 2700 + PI/3) * 0.15; // ±0.15 kW variation
    //     // Phase 3: ~1.5 hour cycle with offset
    //     float phase3Cycle = sin(2 * PI * timeInSeconds / 5400 + 2*PI/3) * 0.05; // ±0.05 kW variation
        
    //     // Calculate individual phase powers
    //     powerL1 = basePower/3 + phase1Cycle;
    //     powerL2 = basePower/3 + phase2Cycle;
    //     powerL3 = basePower/3 + phase3Cycle;
        
    //     // Ensure no negative power
    //     if(powerL1 < 0) powerL1 = 0;
    //     if(powerL2 < 0) powerL2 = 0;
    //     if(powerL3 < 0) powerL3 = 0;
        
    //     // Total power for other calculations
    //     totalPower = powerL1 + powerL2 + powerL3;

    //     // Reactive power simulation
    //     reactivePowerL1 = powerL1 * 0.1; // 10% of active power
    //     reactivePowerL2 = powerL2 * 0.15; // 15% of active power
    //     reactivePowerL3 = powerL3 * 0.05; // 5% of active power
    //     totalReactivePower = reactivePowerL1 + reactivePowerL2 + reactivePowerL3;

    //     // Voltage simulation: 230V + sine wave variation
    //     float baseVoltage = 233.0;
    //     float voltageVariation = sin(2 * PI * timeInSeconds / 10) * 2.0; // ±2V variation every 10 seconds
    //     voltageL1 = baseVoltage + voltageVariation;
    //     voltageL2 = baseVoltage + voltageVariation * cos(2 * PI / 3);
    //     voltageL3 = baseVoltage + voltageVariation * cos(4 * PI / 3);

    //     // Current simulation based on individual phase powers and voltages
    //     currentL1 = (powerL1 * 1000) / voltageL1;
    //     currentL2 = (powerL2 * 1000) / voltageL2;
    //     currentL3 = (powerL3 * 1000) / voltageL3;

    //     // Calculate cumulative energy
    //     static float lastEnergy = 10968.132;
    //     lastEnergy += totalPower * (10.0 / 3600.0); // Add energy for 10-second interval in kWh
    //     energyDelivered = lastEnergy;
        
    //     // Calculate cumulative reactive energy
    //     static float lastReactiveEnergy1 = 5.151;
    //     static float lastReactiveEnergy2 = 2109.781;
    //     lastReactiveEnergy1 += reactivePowerL1 * (10.0 / 3600.0);
    //     lastReactiveEnergy2 += (reactivePowerL2 + reactivePowerL3) * (10.0 / 3600.0);
    //     reactiveEnergy1 = lastReactiveEnergy1;
    //     reactiveEnergy2 = lastReactiveEnergy2;
    // }
    // else {
        // Use the provided P1Data values
        // totalPower = p1data.currentPowerDelivery;
        // // Split the total power between phases (simplified approach)
        // powerL1 = p1data.voltageL1 * p1data.currentL1 / 1000.0; // Convert to kW
        // powerL2 = p1data.voltageL2 * p1data.currentL2 / 1000.0; // Convert to kW
        // powerL3 = p1data.voltageL3 * p1data.currentL3 / 1000.0; // Convert to kW
        
        // // Get voltage and current from P1Data
        // voltageL1 = p1data.voltageL1;
        // voltageL2 = p1data.voltageL2;
        // voltageL3 = p1data.voltageL3;
        // currentL1 = p1data.currentL1;
        // currentL2 = p1data.currentL2;
        // currentL3 = p1data.currentL3;
        
        // // Energy readings from P1Data
        // energyDelivered = p1data.electricityDeliveredTariff1 + p1data.electricityDeliveredTariff2;
        
        // // Simplified reactive power calculations (if not available in P1Data)
        // reactivePowerL1 = powerL1 * 0.1;
        // reactivePowerL2 = powerL2 * 0.15;
        // reactivePowerL3 = powerL3 * 0.05;
        // totalReactivePower = reactivePowerL1 + reactivePowerL2 + reactivePowerL3;
        // reactiveEnergy1 = 5.151; // Default values as these may not be in P1Data
        // reactiveEnergy2 = 2109.781;
    // }

    // Use a vector of Strings instead of an array of C-style strings
    std::vector<String> rows;

    for (int i = 0; i < p1data.obisCount; i++) {
        char buffer[64];
        p1data.obisValues[i].toString(buffer, sizeof(buffer));
        rows.push_back(String(buffer));
    }
    
    // Format timestamp
    time_t now = p1data.timestamp;
    
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "0-0:1.0.0(%02d%02d%02d%02d%02d%02dW)",
             timeinfo.tm_year % 100, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    rows.push_back(String(buffer));
    
    // Calculate checksum (simple implementation)
    String checksum = "0E78"; // Fixed checksum for now
    
    // Add the rows to the data object
    dataObj.addArray("rows", rows);
    
    // Add checksum
    dataObj.add("checksum", checksum.c_str());
    
    // Add the data object to the payload with the timestamp as the key
    String dataStr = dataObj.end();
    String payloadStr = "{\"" + timestampStr + "\":" + dataStr + "}";
    
    // Use the crypto_create_jwt function from the crypto module
    String jwt = crypto_create_jwt(headerStr.c_str(), payloadStr.c_str(), privateKey);
    
    return jwt;
}