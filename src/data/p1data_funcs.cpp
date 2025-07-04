#include "p1data_funcs.h"
#include "json_light/json_light.h"
#include "crypto.h"
#include "config.h"



bool createP1JWTPayload(const P1Data& p1data, char* outBuffer, size_t outBufferSize) {
    
    
    // we get the unitx time in ms from the system clock
    struct timeval tv;
    gettimeofday(&tv, NULL);
    zap::Str timestampStr(p1data.timestamp); // Assuming timestamp is already in milliseconds
    
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
    // This is not needed anymore and should be moved to the binary decoder and just added as the obis string
    
    // Add the rows to the data object
    payload.addArray("rows", rows);
    
    // TODO: Calculate checksum idk if this is really needed
    payload.add("checksum", "DEAD");
    
    // End timestamp sub object
    payload.endObject();
    payload.end();

    return payload.hasOverflow();
}

