#include "webserver.h"
#include "html.h"
#include "wifi/wifi_manager.h"

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
    Serial.println("Setting up endpoints...");

    // Print server configuration
    Serial.print("Server port: ");
    Serial.println(80);  // Default port
    
    // Handle root path separately as it serves HTML
    Serial.println("Registering root (/) endpoint...");
    server.on("/", HTTP_GET, [this]() {
        Serial.println("Handling root request");
        
        // If already provisioned, redirect to system info
        server.sendHeader("Location", "/api/system/info", true);
        server.send(302, "text/plain", "");
    });

    // loop through all endpoints and register them
    for (const Endpoint& endpoint : endpointMapper) {
        server.on(endpoint.path, verbToHttpMethod(endpoint.verb), [this, endpoint]() {
            Serial.println(("Handling for  " + EndpointMapper::verbToString(endpoint.verb) + " " + endpoint.path + " request").c_str());
            EndpointRequest request(endpoint);
            request.content = server.arg("plain").c_str();
            request.offset = 0;

            EndpointResponse response = EndpointMapper::route(request);
            server.send(response.statusCode, response.contentType.c_str(), response.data.c_str());
        });
    }

    // // Handle API endpoints using the endpoint mapper
    // server.on("/api/wifi", HTTP_POST, [this]() {
    //     Serial.println("Handling POST /api/wifi request");
    //     EndpointRequest request;
    //     request.method = Endpoint::Verb::POST;
    //     request.endpoint = Endpoint::WIFI_CONFIG;
    //     request.content = server.arg("plain");
    //     request.offset = 0;

    //     EndpointResponse response = EndpointMapper::route(request);
    //     server.send(response.statusCode, response.contentType, response.data);
    // });

    // server.on("/api/system/info", HTTP_GET, [this]() {
    //     EndpointRequest request;
    //     request.method = Endpoint::Verb::GET;
    //     request.endpoint = Endpoint::SYSTEM_INFO;
    //     request.content = "";
    // });

    // server.on("/api/wifi", HTTP_DELETE, [this]() {
    //     EndpointRequest request;
    //     request.method = Endpoint::Verb::DELETE;
    //     request.endpoint = Endpoint::WIFI_RESET;
    //     request.content = "";
    //     request.offset = 0;

    //     EndpointResponse response = EndpointMapper::route(request);
    //     server.send(response.statusCode, response.contentType, response.data);
    // });

    // server.on("/api/crypto", HTTP_GET, [this]() {
    //     Serial.println("Handling GET /api/crypto request");
    //     EndpointRequest request;
    //     request.method = Endpoint::Verb::GET;
    //     request.endpoint = Endpoint::CRYPTO_INFO;
    //     request.content = "";
    //     request.offset = 0;

    //     EndpointResponse response = EndpointMapper::route(request);
    //     server.send(response.statusCode, response.contentType, response.data);
    // });

    // server.on("/api/name", HTTP_GET, [this]() {
    //     EndpointRequest request;
    //     request.method = Endpoint::Verb::GET;
    //     request.endpoint = Endpoint::NAME_INFO;
    //     request.content = "";
    //     request.offset = 0;

    //     EndpointResponse response = EndpointMapper::route(request);
    //     server.send(response.statusCode, response.contentType, response.data);
    // });

    // server.on("/api/wifi", HTTP_GET, [this]() {
    //     EndpointRequest request;
    //     request.method = Endpoint::Verb::GET;
    //     request.endpoint = Endpoint::WIFI_STATUS;
    //     request.content = "";
    //     request.offset = 0;

    //     EndpointResponse response = EndpointMapper::route(request);
    //     server.send(response.statusCode, response.contentType, response.data);
    // });

    // server.on("/api/wifi/scan", HTTP_GET, [this]() {
    //     EndpointRequest request;
    //     request.method = Endpoint::Verb::GET;
    //     request.endpoint = Endpoint::WIFI_SCAN;
    //     request.content = "";
    //     request.offset = 0;

    //     EndpointResponse response = EndpointMapper::route(request);
    //     server.send(response.statusCode, response.contentType, response.data);
    // });

    // server.on("/api/ota/update", HTTP_POST, [this]() {
    //     EndpointRequest request;
    //     request.method = Endpoint::Verb::POST;
    //     request.endpoint = Endpoint::OTA_UPDATE;
    //     request.content = server.arg("plain");
    //     request.offset = 0;

    //     EndpointResponse response = EndpointMapper::route(request);
    //     server.send(response.statusCode, response.contentType, response.data);
    // });

    // server.on("/api/ble/stop", HTTP_POST, [this]() {
    //     Serial.println("Handling POST /api/ble/stop request");
    //     EndpointRequest request;
    //     request.method = Endpoint::Verb::POST;
    //     request.endpoint = Endpoint::BLE_STOP;
    //     request.content = "";
    //     request.offset = 0;

    //     EndpointResponse response = EndpointMapper::route(request);
    //     server.send(response.statusCode, response.contentType, response.data);
    // });

    // // Handle not found
    // server.onNotFound([this]() {
    //     Serial.println("404 - Not found: " + server.uri());
    //     EndpointRequest request;
    //     request.method = server.method() == HTTP_GET ? Endpoint::Verb::GET : Endpoint::Verb::POST;
    //     request.endpoint = EndpointMapper::pathToEndpoint(server.uri());
    //     request.content = server.arg("plain");
    //     request.offset = 0;

    //     EndpointResponse response = EndpointMapper::route(request);
    //     server.send(response.statusCode, response.contentType, response.data);
    // });
} 