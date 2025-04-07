#pragma once

#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFi.h>
#include <vector>
#include <Arduino.h>
#include "endpoints/endpoint_types.h"
#include "endpoints/endpoint_mapper.h"

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