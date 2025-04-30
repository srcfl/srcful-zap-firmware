#include "p1data_funcs.h"
#include "json_light/json_light.h"
#include "crypto.h"
#include "config.h"


bool createP1JWTPayload(const P1Data& p1data, char* outBuffer, size_t outBufferSize) {
    
    zap::Str timestampStr;
    
    timestampStr = zap::Str(static_cast<long>(p1data.timestamp));
    timestampStr += "000"; // Append '000' to the timestamp as in milliseconds
    
    // Start the payload object
    JsonBuilderFixed payload(outBuffer, outBufferSize);
    payload.beginObject();
    
    // Create a nested object for the timestamp
    JsonBuilder dataObj;
    payload.beginObject(timestampStr.c_str());

    payload.add("serial_number", METER_SN);

    // Use a vector of Str instead of an array of C-style strings
    std::vector<zap::Str> rows;

    // Iterate through the obisStrings array
    for (int i = 0; i < p1data.obisStringCount; i++) {
        // Directly add the string from the obisStrings array
        rows.push_back(zap::Str(p1data.obisStrings[i]));
    }
    
    // Format timestamp
    // TODO: this is somewhat redundant as we get the timestamp in obis format (esp for ascii)
    // but we also need it in msek for the jwt format
    time_t now = p1data.timestamp;
    
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "0-0:1.0.0(%02d%02d%02d%02d%02d%02dW)",
             timeinfo.tm_year % 100, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    rows.push_back(zap::Str(buffer));
    
    zap::Str checksum = "DEAD"; // Fixed checksum for now
    
    // Add the rows to the data object
    payload.addArray("rows", rows);
    
    // TODO: Calculate checksum idk if this is really needed
    payload.add("checksum", "DEAD");
    
    // End timestamp sub object
    payload.endObject();
    payload.end();

    return payload.hasOverflow();
}

