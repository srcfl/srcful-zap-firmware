#include "p1data.h"
#include "json_light/json_light.h"
#include <vector>

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
    String timestamp = String(getCurrentTimestamp());
    
    // Start the payload object
    payload.beginObject();
    
    // Create a nested object for the timestamp
    JsonBuilder dataObj;
    dataObj.beginObject()
        .add("serial_number", "LGF5E360");
    
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
    
    // Add the rows to the data object
    dataObj.addArray("rows", rows);
    
    // Add the data object to the payload with the timestamp as the key
    // This is a bit tricky with our simple JSON builder, so we'll do it manually
    String dataStr = dataObj.end();
    String payloadStr = "{\"" + timestamp + "\":" + dataStr + "}";
    
    // Use the crypto_create_jwt function from the crypto module
    String jwt = crypto_create_jwt(headerStr.c_str(), payloadStr.c_str(), privateKey);
    
    return jwt;
} 