#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Declare the pre-allocated SSL client
extern WiFiClientSecure sslClient;
void initSSL();

String fetchGatewayName(const String& serialNumber);