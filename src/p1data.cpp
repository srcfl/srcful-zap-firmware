#include "p1data.h"
#include <ArduinoJson.h>

void initNTP() {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // 0, 0 = UTC, no daylight offset
    
    Serial.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println();
}

unsigned long long getCurrentTimestamp() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (unsigned long long)tv.tv_sec * 1000LL + (unsigned long long)tv.tv_usec / 1000LL;
}

String createP1JWT(const char* privateKey, const String& deviceId) {
    // Create the header
    StaticJsonDocument<512> header;
    header["alg"] = "ES256";
    header["typ"] = "JWT";
    header["device"] = deviceId;
    header["opr"] = "production";
    header["model"] = "p1homewizard";
    header["dtype"] = "p1_telnet_json";
    header["sn"] = "LGF5E360";

    String headerStr;
    serializeJson(header, headerStr);

    // Create the payload
    StaticJsonDocument<2048> payload;
    String timestamp = String(getCurrentTimestamp());
    
    JsonObject data = payload.createNestedObject(timestamp);
    data["serial_number"] = "LGF5E360";
    
    // Calculate simulated values using sine waves
    float timeInSeconds = millis() / 1000.0;
    
    // Power simulation: Base load + daily cycle + random variation
    float basePower = 1.5; // 1.5 kW base load
    
    // Phase 1: ~1 hour cycle
    float phase1Cycle = sin(2 * PI * timeInSeconds / 3600) * 0.8; // ±0.8 kW variation
    // Phase 2: ~45 minutes cycle with offset
    float phase2Cycle = sin(2 * PI * timeInSeconds / 2700 + PI/3) * 0.6; // ±0.6 kW variation
    // Phase 3: ~1.5 hour cycle with offset
    float phase3Cycle = sin(2 * PI * timeInSeconds / 5400 + 2*PI/3) * 0.7; // ±0.7 kW variation
    
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

    // Voltage simulation: 230V + sine wave variation
    float baseVoltage = 230.0;
    float voltageVariation = sin(2 * PI * timeInSeconds / 10) * 5.0; // ±5V variation every 10 seconds
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

    char buffer[64];
    JsonArray rows = data.createNestedArray("rows");

    // Format timestamp
    time_t now;
    time(&now);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    snprintf(buffer, sizeof(buffer), "0-0:1.0.0(%02d%02d%02d%02d%02d%02dW)",
             timeinfo.tm_year % 100, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    rows.add(buffer);

    // Energy readings
    snprintf(buffer, sizeof(buffer), "1-0:1.8.0(%08.3f*kWh)", lastEnergy);
    rows.add(buffer);
    rows.add("1-0:2.8.0(00000000.000*kWh)");
    rows.add("1-0:3.8.0(00000005.151*kVArh)");
    rows.add("1-0:4.8.0(00002109.781*kVArh)");

    // Power readings
    snprintf(buffer, sizeof(buffer), "1-0:1.7.0(%07.3f*kW)", totalPower);
    rows.add(buffer);
    rows.add("1-0:2.7.0(0000.000*kW)");
    rows.add("1-0:3.7.0(0000.000*kVAr)");
    snprintf(buffer, sizeof(buffer), "1-0:4.7.0(%07.3f*kVAr)", totalPower * 0.1); // 10% reactive power
    rows.add(buffer);

    // Per phase power
    snprintf(buffer, sizeof(buffer), "1-0:21.7.0(%07.3f*kW)", powerL1);
    rows.add(buffer);
    rows.add("1-0:22.7.0(0000.000*kW)");
    snprintf(buffer, sizeof(buffer), "1-0:41.7.0(%07.3f*kW)", powerL2);
    rows.add(buffer);
    rows.add("1-0:42.7.0(0000.000*kW)");
    snprintf(buffer, sizeof(buffer), "1-0:61.7.0(%07.3f*kW)", powerL3);
    rows.add(buffer);
    rows.add("1-0:62.7.0(0000.000*kW)");

    // Voltage readings
    snprintf(buffer, sizeof(buffer), "1-0:32.7.0(%05.1f*V)", voltageL1);
    rows.add(buffer);
    snprintf(buffer, sizeof(buffer), "1-0:52.7.0(%05.1f*V)", voltageL2);
    rows.add(buffer);
    snprintf(buffer, sizeof(buffer), "1-0:72.7.0(%05.1f*V)", voltageL3);
    rows.add(buffer);

    // Current readings
    snprintf(buffer, sizeof(buffer), "1-0:31.7.0(%04.1f*A)", currentL1);
    rows.add(buffer);
    snprintf(buffer, sizeof(buffer), "1-0:51.7.0(%04.1f*A)", currentL2);
    rows.add(buffer);
    snprintf(buffer, sizeof(buffer), "1-0:71.7.0(%04.1f*A)", currentL3);
    rows.add(buffer);

    // Calculate checksum (simple XOR of all characters in the rows)
    String allRows;
    for(size_t i = 0; i < rows.size() - 1; i++) {
        allRows += rows[i].as<String>() + "\n";
    }
    uint8_t checksum = 0;
    for(size_t i = 0; i < allRows.length(); i++) {
        checksum ^= allRows[i];
    }
    
    // Add checksum line
    char checksumLine[8];
    snprintf(checksumLine, sizeof(checksumLine), "!%02X", checksum);
    rows.add(checksumLine);
    
    data["checksum"] = String(checksumLine).substring(1);

    String payloadStr;
    serializeJson(payload, payloadStr);

    // Create and return the JWT
    return crypto_create_jwt(headerStr.c_str(), payloadStr.c_str(), privateKey);
} 