#include "ota_handler.h"
#include "firmware_version.h"
#include "json_light/json_light.h"

OTAHandler::OTAHandler() {
}

OTAHandler::~OTAHandler() {
    stop();
}

void OTAHandler::begin() {
    otaTask.begin();
}

void OTAHandler::stop() {
    otaTask.stop();
}

EndpointResponse OTAHandler::handleOTAUpdate(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::POST) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }

    // Parse the request body
    JsonParser parser(request.content.c_str());
    char url[256] = {0};
    char version[64] = {0};
    bool hasUrl = parser.getString("url", url, sizeof(url));
    bool hasVersion = parser.getString("version", version, sizeof(version));
    
    if (!hasUrl) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"Missing firmware URL\"}";
        return response;
    }

    // Check if update is already in progress
    if (otaTask.isUpdateInProgress()) {
        response.statusCode = 409; // Conflict
        response.data = "{\"status\":\"error\",\"message\":\"Update already in progress\"}";
        return response;
    }

    // Queue the update request
    bool requestSuccess = otaTask.requestUpdate(
        String(url),
        hasVersion ? String(version) : ""
    );
    
    if (requestSuccess) {
        // otaTask.begin();    // TODO: we likely will need to stop some other tasks also...
        response.statusCode = 202; // Accepted
        response.data = "{\"status\":\"success\",\"message\":\"Update request accepted\"}";
    } else {
        response.statusCode = 500;
        response.data = "{\"status\":\"error\",\"message\":\"Failed to queue update request\"}";
    }
    
    return response;
}

EndpointResponse OTAHandler::handleOTAStatus(const EndpointRequest& request) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    if (request.endpoint.verb != Endpoint::Verb::GET) {
        response.statusCode = 405;
        response.data = "{\"status\":\"error\",\"message\":\"Method not allowed\"}";
        return response;
    }
    
    // Check if update is in progress
    bool inProgress = otaTask.isUpdateInProgress();
    int progress = otaTask.getUpdateProgress();
    
    // Check if there's a result available
    OTAUpdateResult result;
    bool hasResult = otaTask.getUpdateResult(&result);
    
    // Build response
    JsonBuilder builder;
    builder.beginObject()
        .add("status", "success")
        .add("in_progress", inProgress)
        .add("progress", progress);
    
    if (hasResult) {
        builder.beginObject("result")
            .add("success", result.success)
            .add("message", result.message)
            .add("status_code", result.statusCode)
        .endObject();
    }
    
    response.statusCode = 200;
    response.data = builder.end();
    
    return response;
}