#include "graphql.h"
#include "config.h"
#include "json_light/json_light.h"
#include "crypto.h"
#include <time.h>
#include <HTTPClient.h>
#include "zap_log.h" // Added for logging

// Define TAG for logging
static const char* TAG = "graphql";

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
    
    LOG_D(TAG, "Sending GraphQL request: %s", requestBody.c_str());
    
    int httpResponseCode = http.POST(requestBody.c_str());
    
    if (httpResponseCode != 200) {
        LOG_E(TAG, "HTTP Error: %d", httpResponseCode);
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
    
    // this is the raw bytes so it is likely a size, newline the data, a 0 and then two newlines messing things up
    // but we are expecting a json object so we can pick the data between the first and last }

    //2e
    //{"data":{"setConfiguration":{"success":true}}}
    //0
    //
    //

    size_t start = responseData.indexOf('{');
    size_t end = responseData.lastIndexOf('}');
    if (start < 0 || end < 0 || start >= end) {
        http.end();
        return StringResponse::invalidResponse("Invalid response format");
    }
    responseData = responseData.substring(start, end + 1);

    LOG_D(TAG, "Response received: %s", responseData.c_str());
    http.end();
    
    // Check for GraphQL errors
    if (responseData.indexOf("\"errors\":") >= 0) {
        return StringResponse::gqlError("GraphQL returned errors: " + responseData);
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

GQL::StringResponse GQL::getConfiguration(const zap::Str& subKey) {
    // Create authentication components (device ID, timestamp, signature)
    zap::Str serialNumber = crypto_getId();
    
    // Generate timestamp in UTC format (Y-m-dTH:M:S)
    time_t now;
    time(&now);
    char timestamp[24];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", gmtime(&now));
    zap::Str timestampStr = zap::Str(timestamp);
    
    // Create the message to sign: deviceId|timestamp
    zap::Str message = serialNumber + ":" + timestampStr;

    LOG_D(TAG, "Message to sign: %s", message.c_str());
    
    // Sign the message
    extern const char* PRIVATE_KEY_HEX;
    zap::Str signature = crypto_create_signature_hex(message.c_str(), PRIVATE_KEY_HEX);
    
    // Create GraphQL query
    zap::Str query = R"({
        gatewayConfiguration {
            configuration(deviceAuth: {
                id: ")" + serialNumber + R"(",
                timestamp: ")" + timestampStr + R"(",
                signedIdAndTimestamp: ")" + signature + R"(",
                subKey: ")" + subKey + R"("
            }) {
                data
            }
        }
    })";
    
    StringResponse response = makeGraphQLRequest(query, API_URL);
    
    if (!response.isSuccess()) {
        return response; // Return the error as is
    }

    // Parse JSON response to extract configuration data
    zap::Str configData;
    JsonParser parser(response.data.c_str());

    if (parser.isFieldNullByPath("data.gatewayConfiguration.configuration.data")) {
        // Handle null data case - return empty string
        return StringResponse::ok("");
    }
    
    if (!parser.getStringByPath("data.gatewayConfiguration.configuration.data", configData)) {
        return StringResponse::invalidResponse("No configuration data in response");
    }
    // the configuration data is a json string within the json object, the " are escaped
    configData.replace("\\u0022", "\"");
    configData.replace("\\u0027", "'");

    LOG_D(TAG, "Configuration data: %s", configData.c_str());
    
    return StringResponse::ok(configData); 
}