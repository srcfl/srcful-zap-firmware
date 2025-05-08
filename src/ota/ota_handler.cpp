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

EndpointResponse OTAHandler::handleOTAUpdate(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";

    // Parse the request body
    JsonParser parser(contents.c_str());
    zap::Str url;
    zap::Str version;
    bool hasUrl = parser.getString("url", url);
    bool hasVersion = parser.getString("version", version);
    
    if (!hasUrl || !hasVersion) {
        response.statusCode = 400;
        response.data = "{\"status\":\"error\",\"message\":\"Missing firmware URL or version\"}";
        return response;
    }

    // Check if update is already in progress
    if (otaTask.isUpdateInProgress()) {
        response.statusCode = 409; // Conflict
        response.data = "{\"status\":\"error\",\"message\":\"Update already in progress\"}";
        return response;
    }

    // Queue the update request
    bool requestSuccess = requestOTAUpdate(url, version);
    
    if (requestSuccess) {
        response.statusCode = 202; // Accepted
        response.data = "{\"status\":\"success\",\"message\":\"Update request accepted\"}";
    } else {
        response.statusCode = 500;
        response.data = "{\"status\":\"error\",\"message\":\"Failed to queue update request\"}";
    }
    
    return response;
}

bool OTAHandler::requestOTAUpdate(const zap::Str& url, const zap::Str& version) {
    return otaTask.requestUpdate(url, version);
}
    

EndpointResponse OTAHandler::handleOTAStatus(const zap::Str& contents) {
    EndpointResponse response;
    response.contentType = "application/json";
    
    
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