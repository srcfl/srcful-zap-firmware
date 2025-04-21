#include "graphql.h"
#include "config.h"
#include <HTTPClient.h>

// At the top of the file, outside any function
static uint8_t* sslBuffer = nullptr;
const size_t SSL_BUFFER_SIZE = 16 * 1024;  // 16KB buffer

zap::Str GQL::prepareGraphQLQuery(const zap::Str& rawQuery) {
    zap::Str prepared = rawQuery;
    prepared.replace("\"", "\\\"");
    prepared.replace("\n", "\\n");
    return prepared;
}

GQL::StringResponse GQL::makeGraphQLRequest(const zap::Str& query, const char* endpoint) {
    // Print memory info
    
    HTTPClient http;
    http.setTimeout(10000);
    
    if (!http.begin(endpoint)) {
        return StringResponse::networkError("Unable to begin HTTP connection");
    }
    
    http.addHeader("Content-Type", "application/json");
    
    // Construct JSON manually to avoid ArduinoJson memory overhead
    zap::Str preparedQuery = prepareGraphQLQuery(query);
    zap::Str requestBody = "{\"query\":\"" + preparedQuery + "\"}";
    
    Serial.println("Sending GraphQL request:");
    Serial.println(requestBody.c_str());
    
    int httpResponseCode = http.POST(requestBody.c_str());
    
    if (httpResponseCode != 200) {
        Serial.printf("HTTP Error: %d\n", httpResponseCode);
        http.end();
        return StringResponse::networkError("HTTP error: " + zap::Str(httpResponseCode));
    }
    
    // Stream the response instead of loading it all at once
    WiFiClient* stream = http.getStreamPtr();
    zap::Str responseData;
    responseData.reserve(512); // Pre-allocate space
    
    while (stream->available()) {
        responseData += (char)stream->read();
    }
    
    Serial.println("Response received");
    http.end();
    
    // Check for GraphQL errors
    if (responseData.indexOf("\"errors\":") >= 0) {
        return StringResponse::gqlError("GraphQL returned errors");
    }
    
    return StringResponse::ok(responseData);
}

GQL::StringResponse GQL::fetchGatewayName(const zap::Str& serialNumber) {
    zap::Str query = R"({
        gatewayConfiguration {
          gatewayName(id:")" + serialNumber + R"(") {
            name
          }
        }
    })";
    
    StringResponse response = makeGraphQLRequest(query, API_URL);
    
    if (!response.isSuccess()) {
        return response; // Return the error as is
    }
    
    // Parse JSON response manually to avoid ArduinoJson
    int dataPos = response.data.indexOf("\"data\":");
    if (dataPos < 0) {
        return StringResponse::invalidResponse("No data field in response");
    }
    
    int nameStart = response.data.indexOf("\"name\":\"", dataPos);
    if (nameStart < 0) {
        return StringResponse::parseError("Name field not found");
    }
    
    nameStart += 8; // Length of "\"name\":\""
    int nameEnd = response.data.indexOf("\"", nameStart);
    if (nameEnd < 0) {
        return StringResponse::parseError("Malformed name field");
    }
    
    return StringResponse::ok(response.data.substring(nameStart, nameEnd));
}

GQL::BoolResponse GQL::setConfiguration(const zap::Str& jwt) {
    zap::Str query = R"(mutation SetGatewayConfigurationWithDeviceJWT {
        setConfiguration(deviceConfigurationInputType: {
            jwt: ")" + jwt + R"("
        }) {
            success
        }
    })";
    
    StringResponse response = makeGraphQLRequest(query, API_URL);
    
    if (!response.isSuccess()) {
        // Convert StringResponse error to BoolResponse with same status
        return BoolResponse{response.status, false, response.error};
    }
    
    // Parse JSON response to check success status
    int successPos = response.data.indexOf("\"success\":");
    if (successPos < 0) {
        return BoolResponse::invalidResponse("No success field in response");
    }
    
    successPos += 10; // Length of "\"success\":"
    
    // Check if success is true in the GraphQL response
    bool operationSuccess = (response.data.indexOf("true", successPos) >= 0);
    
    if (!operationSuccess) {
        return BoolResponse::operationFailed("Server reported operation failure");
    }
    
    return BoolResponse::ok(true);
}