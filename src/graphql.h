#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// Declare the pre-allocated SSL client
extern WiFiClientSecure sslClient;
void initSSL();

String fetchGatewayName(const String& serialNumber);
bool initializeGateway(const String& idAndWallet, const String& signature, bool dryRun, String& responseData); 