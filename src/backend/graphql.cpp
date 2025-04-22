#include "graphql.h"
#include "config.h"
#include "json_light/json_light.h"
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

    // Parse JSON response to check success status
    zap::Str name;

    JsonParser parser(response.data.c_str());
    if (!parser.getStringByPath("data.gatewayConfiguration.gatewayName.name", name)) {
        return StringResponse::invalidResponse("Invalid response structure");;
    }
    
    return StringResponse::ok(name);
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
    bool successValue;

    JsonParser parser(response.data.c_str());
    if (!parser.getBoolByPath("data.setConfiguration.success", successValue)) {
        return BoolResponse::invalidResponse("No success field in response");
    }
    
    if (!successValue) {
        return BoolResponse::operationFailed("Server reported operation failure");
    }
    
    return BoolResponse::ok(true);
}