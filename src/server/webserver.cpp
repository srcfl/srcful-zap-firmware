#include "webserver.h"
#include "wifi/wifi_manager.h"
#include "../zap_log.h" // Added for logging

// Define TAG for logging
static const char* TAG = "webserver";

WebServerHandler::WebServerHandler(int port) : server(port) {
}

void WebServerHandler::begin() {
    server.begin();
}

void WebServerHandler::handleClient() {
    server.handleClient();
}

HTTPMethod verbToHttpMethod(Endpoint::Verb verb) {
    switch (verb) {
        case Endpoint::Verb::GET: return HTTP_GET;
        case Endpoint::Verb::POST: return HTTP_POST;
        case Endpoint::Verb::DELETE: return HTTP_DELETE;
        default: return HTTP_ANY;
    }
}

void WebServerHandler::setupEndpoints() {
    LOG_I(TAG, "Setting up endpoints...");

    // Print server configuration
    LOG_I(TAG, "Server port: %d", 80);  // Default port
    
    // Handle root path separately as it serves HTML
    LOG_I(TAG, "Registering root (/) endpoint...");
    server.on("/", HTTP_GET, [this]() {
        LOG_I(TAG, "Handling root request");
        
        // If already provisioned, redirect to system info
        server.sendHeader("Location", EndpointMapper::SYSTEM_INFO_PATH, true);
        server.send(302, "text/plain", "");
    });

    // loop through all endpoints and register them
    for (const Endpoint& endpoint : endpointMapper) {
        server.on(endpoint.path, verbToHttpMethod(endpoint.verb), [this, endpoint]() {
            LOG_I(TAG, "Handling for %s %s request", EndpointMapper::verbToString(endpoint.verb).c_str(), endpoint.path); // Corrected: endpoint.path is already a const char*
            EndpointRequest request(endpoint);
            request.content = server.arg("plain").c_str();
            request.offset = 0;

            EndpointResponse response = EndpointMapper::route(request);
            server.send(response.statusCode, response.contentType.c_str(), response.data.c_str());
        });
    }

}