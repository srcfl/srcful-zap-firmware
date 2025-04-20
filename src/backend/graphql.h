#pragma once
#include "zap_str.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Declare the pre-allocated SSL client
extern WiFiClientSecure sslClient;
void initSSL();

// Prepare GraphQL query by escaping quotes and newlines
zap::Str prepareGraphQLQuery(const zap::Str& rawQuery);

// Make a GraphQL request to the specified endpoint
bool makeGraphQLRequest(const zap::Str& query, zap::Str& responseData, const char* endpoint);

// Fetch gateway name from the API
zap::Str fetchGatewayName(const zap::Str& serialNumber);