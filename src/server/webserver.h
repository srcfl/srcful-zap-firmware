#pragma once

#include <WebServer.h>
#include <vector>
#include <Arduino.h>
#include "endpoint_types.h"
#include "endpoint_mapper.h"

class WebServerHandler {
public:
    WebServerHandler(int port = 80);
    void begin();
    void handleClient();
    void setupEndpoints();
    WebServer& getServer() { return server; }

private:
    WebServer server;
}; 