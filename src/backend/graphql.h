#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Declare the pre-allocated SSL client
extern WiFiClientSecure sslClient;
void initSSL();

// Prepare GraphQL query by escaping quotes and newlines
String prepareGraphQLQuery(const String& rawQuery);

// Make a GraphQL request to the specified endpoint
bool makeGraphQLRequest(const String& query, String& responseData, const char* endpoint);

// Fetch gateway name from the API
String fetchGatewayName(const String& serialNumber);