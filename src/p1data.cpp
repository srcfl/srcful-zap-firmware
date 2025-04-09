#include "p1data.h"
#include "json_light/json_light.h"
#include <vector>

String createP1JWT(const char* privateKey, const String& deviceId) {
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
    String timestampStr = String(time(nullptr));
    timestampStr += "000"; // Append '000' to the timestamp as in milliseconds
    
    // Start the payload object
    payload.beginObject();
    
    // Create a nested object for the timestamp
    JsonBuilder dataObj;
    dataObj.beginObject()
        .add("serial_number", "LGF5E360");
    
    // Calculate simulated values using sine waves
    float timeInSeconds = millis() / 1000.0;
    
    // Power simulation: Base load + daily cycle + random variation
    float basePower = 0.25; // 0.25 kW base load
    
    // Phase 1: ~1 hour cycle
    float phase1Cycle = sin(2 * PI * timeInSeconds / 3600) * 0.1; // ±0.1 kW variation
    // Phase 2: ~45 minutes cycle with offset
    float phase2Cycle = sin(2 * PI * timeInSeconds / 2700 + PI/3) * 0.15; // ±0.15 kW variation
    // Phase 3: ~1.5 hour cycle with offset
    float phase3Cycle = sin(2 * PI * timeInSeconds / 5400 + 2*PI/3) * 0.05; // ±0.05 kW variation
    
    // Calculate individual phase powers
    float powerL1 = basePower/3 + phase1Cycle;
    float powerL2 = basePower/3 + phase2Cycle;
    float powerL3 = basePower/3 + phase3Cycle;
    
    // Ensure no negative power
    if(powerL1 < 0) powerL1 = 0;
    if(powerL2 < 0) powerL2 = 0;
    if(powerL3 < 0) powerL3 = 0;
    
    // Total power for other calculations
    float totalPower = powerL1 + powerL2 + powerL3;

    // Reactive power simulation
    float reactivePowerL1 = powerL1 * 0.1; // 10% of active power
    float reactivePowerL2 = powerL2 * 0.15; // 15% of active power
    float reactivePowerL3 = powerL3 * 0.05; // 5% of active power
    float totalReactivePower = reactivePowerL1 + reactivePowerL2 + reactivePowerL3;

    // Voltage simulation: 230V + sine wave variation
    float baseVoltage = 233.0;
    float voltageVariation = sin(2 * PI * timeInSeconds / 10) * 2.0; // ±2V variation every 10 seconds
    float voltageL1 = baseVoltage + voltageVariation;
    float voltageL2 = baseVoltage + voltageVariation * cos(2 * PI / 3);
    float voltageL3 = baseVoltage + voltageVariation * cos(4 * PI / 3);

    // Current simulation based on individual phase powers and voltages
    float currentL1 = (powerL1 * 1000) / voltageL1;
    float currentL2 = (powerL2 * 1000) / voltageL2;
    float currentL3 = (powerL3 * 1000) / voltageL3;

    // Calculate cumulative energy
    static float lastEnergy = 10968.132;
    lastEnergy += totalPower * (10.0 / 3600.0); // Add energy for 10-second interval in kWh
    
    // Calculate cumulative reactive energy
    static float lastReactiveEnergy1 = 5.151;
    static float lastReactiveEnergy2 = 2109.781;
    lastReactiveEnergy1 += reactivePowerL1 * (10.0 / 3600.0);
    lastReactiveEnergy2 += (reactivePowerL2 + reactivePowerL3) * (10.0 / 3600.0);

    // Use a vector of Strings instead of an array of C-style strings
    std::vector<String> rows;
    
    // Format timestamp
    time_t now;
    time(&now);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "0-0:1.0.0(%02d%02d%02d%02d%02d%02dW)",
             timeinfo.tm_year % 100, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    rows.push_back(String(buffer));

    // Energy readings
    snprintf(buffer, sizeof(buffer), "1-0:1.8.0(%08.3f*kWh)", lastEnergy);
    rows.push_back(String(buffer));
    
    // Imported energy (always 0 in this simulation)
    snprintf(buffer, sizeof(buffer), "1-0:2.8.0(00000000.000*kWh)");
    rows.push_back(String(buffer));
    
    // Reactive energy
    snprintf(buffer, sizeof(buffer), "1-0:3.8.0(%08.3f*kVArh)", lastReactiveEnergy1);
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:4.8.0(%08.3f*kVArh)", lastReactiveEnergy2);
    rows.push_back(String(buffer));
    
    // Total power readings
    snprintf(buffer, sizeof(buffer), "1-0:1.7.0(%07.3f*kW)", totalPower);
    rows.push_back(String(buffer));
    
    // Imported power (always 0 in this simulation)
    snprintf(buffer, sizeof(buffer), "1-0:2.7.0(0000.000*kW)");
    rows.push_back(String(buffer));
    
    // Reactive power
    snprintf(buffer, sizeof(buffer), "1-0:3.7.0(%07.3f*kVAr)", reactivePowerL1);
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:4.7.0(%07.3f*kVAr)", reactivePowerL2 + reactivePowerL3);
    rows.push_back(String(buffer));
    
    // Phase 1 power readings
    snprintf(buffer, sizeof(buffer), "1-0:21.7.0(%07.3f*kW)", powerL1);
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:22.7.0(0000.000*kW)");
    rows.push_back(String(buffer));
    
    // Phase 2 power readings
    snprintf(buffer, sizeof(buffer), "1-0:41.7.0(%07.3f*kW)", powerL2);
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:42.7.0(0000.000*kW)");
    rows.push_back(String(buffer));
    
    // Phase 3 power readings
    snprintf(buffer, sizeof(buffer), "1-0:61.7.0(%07.3f*kW)", powerL3);
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:62.7.0(0000.000*kW)");
    rows.push_back(String(buffer));
    
    // Phase 1 reactive power readings
    snprintf(buffer, sizeof(buffer), "1-0:23.7.0(0000.000*kVAr)");
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:24.7.0(0000.000*kVAr)");
    rows.push_back(String(buffer));
    
    // Phase 2 reactive power readings
    snprintf(buffer, sizeof(buffer), "1-0:43.7.0(0000.000*kVAr)");
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:44.7.0(%07.3f*kVAr)", reactivePowerL2);
    rows.push_back(String(buffer));
    
    // Phase 3 reactive power readings
    snprintf(buffer, sizeof(buffer), "1-0:63.7.0(0000.000*kVAr)");
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:64.7.0(%07.3f*kVAr)", reactivePowerL3);
    rows.push_back(String(buffer));
    
    // Add voltage readings
    snprintf(buffer, sizeof(buffer), "1-0:32.7.0(%05.1f*V)", voltageL1);
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:52.7.0(%05.1f*V)", voltageL2);
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:72.7.0(%05.1f*V)", voltageL3);
    rows.push_back(String(buffer));
    
    // Add current readings
    snprintf(buffer, sizeof(buffer), "1-0:31.7.0(%05.1f*A)", currentL1);
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:51.7.0(%05.1f*A)", currentL2);
    rows.push_back(String(buffer));
    snprintf(buffer, sizeof(buffer), "1-0:71.7.0(%05.1f*A)", currentL3);
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