#include "backend/request_handler.h"
#include <Arduino.h>
#include <sys/time.h>
#include "json_light/json_light.h"
#include "endpoints/endpoint_mapper.h"
#include "endpoints/endpoint_types.h"
#include "backend/graphql.h"
#include "crypto.h"
#include "config.h"

// --- RequestHandler Implementation ---

using namespace zap::backend;

void RequestHandler::handleRequestTask(JsonParser& configData) {
    Serial.println("RequestHandler: Processing configuration data");

    // extract the data string and convert it to a json string
    zap::Str data;
    if (!configData.getString("data", data)) {
        Serial.println("RequestHandler: Failed to extract data from configuration");
        return;
    }
    Serial.println("RequestHandler: Received data: ");
    Serial.println(data.c_str());
    // Replace escaped characters - TODO: Improve this handling
    data.replace("\\u0022", "\"");

    JsonParser dataDoc(data.c_str());

    // Check if this is a request to handle
    if (dataDoc.contains("id") && dataDoc.contains("path") && dataDoc.contains("method")) {
        // This appears to be a request, so handle it
        handleRequest(dataDoc);
    } else {
        // This is some other type of configuration data
        Serial.println("RequestHandler: Received non-request configuration");
        // Process other configuration types here if needed
    }

    Serial.println("RequestHandler: Configuration processing completed");
}

void RequestHandler::handleRequest(JsonParser& requestData) {
    zap::Str id; requestData.getString("id", id);
    zap::Str path; requestData.getString("path", path);
    zap::Str method; requestData.getString("method", method);
    zap::Str body; requestData.getString("body", body);
    zap::Str query; requestData.getString("query", query);
    zap::Str headers; requestData.getString("headers", headers);
    uint64_t timestamp; requestData.getUInt64("timestamp", timestamp);

    Serial.print("RequestHandler: Processing request id=");
    Serial.print(id.c_str());
    Serial.print(", path=");
    Serial.print(path.c_str());
    Serial.print(", method=");
    Serial.print(method.c_str());
    Serial.print(", body=");
    Serial.print(body.c_str());
    Serial.println(", end");

    // Validate timestamp - reject requests older than 1 minute
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t currentTimeMs = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
    // Check timestamp against current time in milliseconds
    if (timestamp < (currentTimeMs - 60000)) { // 60 seconds * 1000 ms/sec
        Serial.printf("RequestHandler: Request too old. Timestamp: %llu, Current: %llu\n", timestamp, currentTimeMs);
        sendErrorResponse(id, "Request too old");
        return;
    }

    // Use the EndpointMapper to find and execute the appropriate endpoint handler
    const Endpoint& endpoint = EndpointMapper::toEndpoint(path, method);

    if (endpoint.type == Endpoint::UNKNOWN) {
        // No endpoint found for this path/method combination
        Serial.println("RequestHandler: Endpoint not found");
        sendErrorResponse(id, "Endpoint not found");
        return;
    }

    // Create an endpoint request
    EndpointRequest request(endpoint);
    request.content = body;

    // Route the request to the appropriate handler
    EndpointResponse response = EndpointMapper::route(request);

    // Send the response
    sendResponse(id, response.statusCode, response.data);
}

void RequestHandler::sendResponse(const zap::Str& requestId, int statusCode, const zap::Str& responseData) {
    Serial.print("RequestHandler: Sending response for request ");
    Serial.println(requestId.c_str());

    // Get current epoch time in milliseconds
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t epochTimeMs = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;

    // Use JsonBuilder to create JWT header
    JsonBuilder headerBuilder;
    headerBuilder.beginObject()
        .add("alg", "ES256")
        .add("typ", "JWT")
        .add("device", crypto_getId().c_str())
        .add("subKey", "response");
    zap::Str header = headerBuilder.end();

    // Use JsonBuilder to create JWT payload
    // Need to properly escape the responseData if it's intended to be a JSON string within the payload
    zap::Str escapedResponse = responseData;
    // Basic escaping - might need more robust JSON string escaping
    escapedResponse.replace("\"", "\\\"");

    JsonBuilder payloadBuilder;
    payloadBuilder.beginObject()
        .add("id", requestId.c_str())
        .add("timestamp", epochTimeMs)
        .add("code", statusCode)
        // Assuming responseData is already a valid JSON object/array string or needs to be treated as a simple string
        // If responseData is meant to be a JSON string *within* the payload, it needs escaping.
        // If responseData is a JSON object/array itself, it shouldn't be quoted.
        // Let's assume it's a string for now, requiring escaping.
        .add("response", escapedResponse.c_str()); // Treat as string, needs escaping
    zap::Str payload = payloadBuilder.end();

    // Sign and generate JWT
    zap::Str jwt = crypto_create_jwt(header.c_str(), payload.c_str(), PRIVATE_KEY_HEX);

    if (jwt.length() == 0) {
        Serial.println("RequestHandler: Failed to create JWT for response");
        return;
    }

    // Send the JWT using GraphQL
    GQL::BoolResponse response = GQL::setConfiguration(jwt);

    // Handle the response
    if (response.isSuccess() && response.data) {
        Serial.print("RequestHandler: Response for request ");
        Serial.print(requestId.c_str());
        Serial.println(" sent successfully");
    } else {
        Serial.print("RequestHandler: Failed to send response for request ");
        Serial.println(requestId.c_str());
        Serial.print("Error: ");
        Serial.println(response.error.c_str());
    }
}

void RequestHandler::sendErrorResponse(const zap::Str& requestId, const char* errorMessage) {
    JsonBuilder errorBuilder;
    errorBuilder.beginObject()
        .add("error", errorMessage);
    zap::Str errorJson = errorBuilder.end();

    // Send with a 400 Bad Request status code (or choose appropriate error code)
    sendResponse(requestId, 400, errorJson);
}
