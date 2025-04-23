#include "backend_api_task.h"
#include "crypto.h"
#include "config.h"
#include "./graphql.h"
#include "json_light/json_light.h"
#include "firmware_version.h"
#include <time.h>
#include "endpoints/endpoint_mapper.h"
#include "endpoints/endpoint_types.h"
#include "endpoints/endpoints.h"

BackendApiTask::BackendApiTask(uint32_t stackSize, UBaseType_t priority) 
    : taskHandle(nullptr), stackSize(stackSize), priority(priority), shouldRun(false),
      wifiManager(nullptr), lastUpdateTime(0), lastConfigFetchTime(0), 
      stateUpdateInterval(DEFAULT_STATE_UPDATE_INTERVAL), 
      configFetchInterval(DEFAULT_CONFIG_FETCH_INTERVAL), bleActive(false) {
    // Initialize with default intervals
}

BackendApiTask::~BackendApiTask() {
    stop();
    http.end(); // Clean up the HTTP client connection
}

void BackendApiTask::begin(WifiManager* wifiManager) {
    if (taskHandle != nullptr) {
        return; // Task already running
    }
    
    this->wifiManager = wifiManager;
    shouldRun = true;
    
    // Configure HTTP client once
    http.setTimeout(10000);  // 10 second timeout
    
    // Force immediate state update and config fetch by setting lastUpdateTime to a value that will
    // trigger an immediate update in the task loop
    lastUpdateTime = 0;
    lastConfigFetchTime = 0;
    
    // Set interval to 0 to trigger immediate update on connection
    stateUpdateInterval = 0;
    
    xTaskCreatePinnedToCore(
        taskFunction,
        "BackendApiTask",
        stackSize,
        this,
        priority,
        &taskHandle,
        0  // Run on core 0
    );
}

void BackendApiTask::stop() {
    if (taskHandle == nullptr) {
        return; // Task not running
    }
    
    shouldRun = false;
    vTaskDelay(pdMS_TO_TICKS(100)); // Give the task time to exit
    
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
}

void BackendApiTask::setInterval(uint32_t interval) {
    stateUpdateInterval = interval;
}

void BackendApiTask::setConfigFetchInterval(uint32_t interval) {
    configFetchInterval = interval;
}

void BackendApiTask::setBleActive(bool active) {
    bleActive = active;
}

bool BackendApiTask::isBleActive() const {
    return bleActive;
}

void BackendApiTask::taskFunction(void* parameter) {
    BackendApiTask* task = static_cast<BackendApiTask*>(parameter);
    
    while (task->shouldRun) {
        unsigned long currentTime = millis();
        
        // Check if it's time to send a state update
        if (currentTime - task->lastUpdateTime > task->stateUpdateInterval) {
            // After first update, restore to default interval if currently zero
            if (task->stateUpdateInterval == 0) {
                task->stateUpdateInterval = DEFAULT_STATE_UPDATE_INTERVAL;
            }
            
            task->lastUpdateTime = currentTime;
            
            // Only send state update if WiFi is connected and BLE is not active
            if (task->wifiManager && task->wifiManager->isConnected() && !task->bleActive) {
                Serial.println("Backend API task: Sending state update...");
                task->sendStateUpdate();
            } else {
                if (task->wifiManager && task->wifiManager->isConnected()) {
                    Serial.println("Backend API task: WiFi connected but BLE is active, not sending state update");
                } else {
                    Serial.println("Backend API task: WiFi not connected, not sending state update");
                }
            }
        }
        
        // Check if it's time to fetch configuration
        if (currentTime - task->lastConfigFetchTime > task->configFetchInterval) {
            task->lastConfigFetchTime = currentTime;
            
            // Only fetch configuration if WiFi is connected
            if (task->wifiManager && task->wifiManager->isConnected()) {
                Serial.println("Backend API task: Fetching configuration...");
                task->fetchConfiguration();
            } else {
                Serial.println("Backend API task: WiFi not connected, not fetching configuration");
            }
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}

void BackendApiTask::sendStateUpdate() {
    Serial.println("Backend API task: Preparing state update");
    
    // Use JsonBuilder to create JWT header
    JsonBuilder headerBuilder;
    headerBuilder.beginObject()
        .add("alg", "ES256")
        .add("typ", "JWT")
        .add("device", crypto_getId().c_str())
        .add("subKey", "state");
    zap::Str header = headerBuilder.end();
    
    // Get current epoch time in milliseconds
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t epochTimeMs = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
    
    // Use JsonBuilder to create JWT payload
    JsonBuilder payloadBuilder;
    payloadBuilder.beginObject()
        .beginObject("status")
            .add("uptime", millis())
            .add("version", FIRMWARE_VERSION_STRING)
        .endObject()
        .add("timestamp", epochTimeMs);
    zap::Str payload = payloadBuilder.end();
    
    // External signature key (from config.h)
    extern const char* PRIVATE_KEY_HEX;
    
    // Sign and generate JWT
    zap::Str jwt = crypto_create_jwt(header.c_str(), payload.c_str(), PRIVATE_KEY_HEX);
    
    if (jwt.length() == 0) {
        Serial.println("Backend API task: Failed to create JWT");
        return;
    }
    
    Serial.println("Backend API task: JWT created successfully");
    
    // Send the JWT using GraphQL
    GQL::BoolResponse response = GQL::setConfiguration(jwt);
    
    // Handle the response
    if (response.isSuccess() && response.data) {
        Serial.println("Backend API task: State update sent successfully");
    } else {
        // Handle different error cases
        switch (response.status) {
            case GQL::Status::NETWORK_ERROR:
                Serial.println("Backend API task: Network error sending state update");
                break;
            case GQL::Status::GQL_ERROR:
                Serial.println("Backend API task: GraphQL error in state update");
                break;
            case GQL::Status::OPERATION_FAILED:
                Serial.println("Backend API task: Server rejected state update");
                break;
            default:
                Serial.print("Backend API task: Failed to send state update: ");
                Serial.println(response.error.c_str());
                break;
        }
        
        // Retry sooner on failure (after 1 minute instead of 5)
        lastUpdateTime = millis() - (stateUpdateInterval - 60000);
    }
}

void BackendApiTask::fetchConfiguration() {
    // First, try to fetch the device configuration
    GQL::StringResponse configResponse = GQL::getConfiguration("request");
    if (configResponse.isSuccess()) {
        Serial.println("Backend API task: Device configuration fetched successfully");
        processConfiguration(configResponse.data.c_str());
    } else {
        Serial.print("Backend API task: Failed to fetch device configuration: ");
        Serial.println(configResponse.error.c_str());
    }
}

void BackendApiTask::processConfiguration(const char* configData) {
    Serial.println("Backend API task: Processing configuration data");
    Serial.println(configData);
    
    JsonParser parser(configData);
    
    // Check if this is a request to handle
    if (parser.contains("id") && parser.contains("path") && parser.contains("method")) {
        // This appears to be a request, so handle it
        handleRequest(configData);
    } else {
        // This is some other type of configuration data
        Serial.println("Backend API task: Received non-request configuration");
        // Process other configuration types here if needed
    }
    
    Serial.println("Backend API task: Configuration processing completed");
}

void BackendApiTask::handleRequest(const char* configData) {
    JsonParser parser(configData);

    zap::Str body; parser.getString("body", body);parser.reset();

    zap::Str id; parser.getString("id", id);parser.reset();
    zap::Str path; parser.getString("path", path);parser.reset();
    zap::Str method; parser.getString("method", method);parser.reset();
    zap::Str query; parser.getString("query", query);parser.reset();
    zap::Str headers; parser.getString("headers", headers);parser.reset();
    uint64_t timestamp; parser.getUInt64("timestamp", timestamp);parser.reset();
    
    Serial.print("Backend API task: Processing request id=");
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
    if (timestamp < (currentTimeMs / 1000 - 60)) {
        sendErrorResponse(id, "Request too old");
        return;
    }
    
    // Use the EndpointMapper to find and execute the appropriate endpoint handler
    const Endpoint& endpoint = EndpointMapper::toEndpoint(path, method);
    
    if (endpoint.type == Endpoint::UNKNOWN) {
        // No endpoint found for this path/method combination
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

void BackendApiTask::sendResponse(const zap::Str& requestId, int statusCode, const zap::Str& responseData) {
    Serial.print("Backend API task: Sending response for request ");
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

    zap::Str escapedResponse = responseData;
    escapedResponse.replace("\"", "\\\""); // Escape double quotes in JSON string


    JsonBuilder payloadBuilder;
    payloadBuilder.beginObject()
        .add("id", requestId.c_str())
        .add("timestamp", epochTimeMs)
        .add("code", statusCode)
        .add("response", escapedResponse.c_str());
    zap::Str payload = payloadBuilder.end();
    
    // External signature key (from config.h)
    extern const char* PRIVATE_KEY_HEX;
    
    // Sign and generate JWT
    zap::Str jwt = crypto_create_jwt(header.c_str(), payload.c_str(), PRIVATE_KEY_HEX);
    
    if (jwt.length() == 0) {
        Serial.println("Backend API task: Failed to create JWT for response");
        return;
    }
    
    // Send the JWT using GraphQL
    GQL::BoolResponse response = GQL::setConfiguration(jwt);
    
    // Handle the response
    if (response.isSuccess() && response.data) {
        Serial.print("Backend API task: Response for request ");
        Serial.print(requestId.c_str());
        Serial.println(" sent successfully");
    } else {
        Serial.print("Backend API task: Failed to send response for request ");
        Serial.println(requestId.c_str());
    }
}

void BackendApiTask::sendErrorResponse(const zap::Str& requestId, const char* errorMessage) {
    JsonBuilder errorBuilder;
    errorBuilder.beginObject()
        .add("error", errorMessage);
    zap::Str errorJson = errorBuilder.end();
    
    sendResponse(requestId, 400, errorJson);
}

void BackendApiTask::triggerStateUpdate() {
    // Only send state update if WiFi is connected and BLE is not active
    if (wifiManager && wifiManager->isConnected() && !bleActive) {
        Serial.println("Backend API task: Triggering immediate state update...");
        sendStateUpdate();
    } else {
        Serial.println("Backend API task: Cannot trigger update - WiFi not connected or BLE active");
    }
}

void BackendApiTask::triggerConfigFetch() {
    // Only fetch configuration if WiFi is connected
    if (wifiManager && wifiManager->isConnected()) {
        Serial.println("Backend API task: Triggering immediate configuration fetch...");
        fetchConfiguration();
    } else {
        Serial.println("Backend API task: Cannot fetch configuration - WiFi not connected");
    }
}