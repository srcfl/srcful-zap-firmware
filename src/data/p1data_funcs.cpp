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

    // Find the 42 7 code and print the value
    for (int i = 0; i < p1data.obisCount; i++) {
        if (p1data.obisValues[i].C == 42 && p1data.obisValues[i].D == 7) {
            Serial.print("Found 42.7 code (L2 Export): ");
            Serial.println(p1data.obisValues[i].value);
            break;
        }
    }
    

    dataObj.add("serial_number", "LGF5E360");

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