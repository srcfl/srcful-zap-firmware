#include "graphql.h"
#include "esp_heap_caps.h"  // Add this for heap functions

// At the top of the file, outside any function
static uint8_t* sslBuffer = nullptr;
const size_t SSL_BUFFER_SIZE = 16 * 1024;  // 16KB buffer

// Define static SSL client
WiFiClientSecure sslClient;

// Add this function
void initSSL() {
    // Configure SSL client early to reserve memory
    sslClient.setInsecure();
}

String prepareGraphQLQuery(const String& rawQuery) {
    String prepared = rawQuery;
    prepared.replace("\"", "\\\"");
    prepared.replace("\n", "\\n");
    return prepared;
}

bool makeGraphQLRequest(const String& query, String& responseData, const char* endpoint) {
    // Print memory info
    Serial.printf("Free heap before request: %d\n", ESP.getFreeHeap());
    
    HTTPClient http;
    http.setTimeout(10000);
    
    if (http.begin(sslClient, endpoint)) {
        http.addHeader("Content-Type", "application/json");
        
        // Construct JSON manually to avoid ArduinoJson memory overhead
        String preparedQuery = prepareGraphQLQuery(query);
        String requestBody = "{\"query\":\"" + preparedQuery + "\"}";
        
        Serial.println("Sending GraphQL request:");
        Serial.println(requestBody);
        
        int httpResponseCode = http.POST(requestBody);
        
        if (httpResponseCode == 200) {
            // Stream the response instead of loading it all at once
            WiFiClient* stream = http.getStreamPtr();
            responseData.reserve(512); // Pre-allocate space
            
            while (stream->available()) {
                responseData += (char)stream->read();
            }
            
            Serial.println("Response received");
            http.end();
            return true;
        } else {
            Serial.printf("HTTP Error: %d\n", httpResponseCode);
        }
        
        http.end();
    }
    
    Serial.printf("Free heap after request: %d\n", ESP.getFreeHeap());
    return false;
}

String fetchGatewayName(const String& serialNumber) {
    String query = R"({
        gatewayConfiguration {
          gatewayName(id:")" + serialNumber + R"(") {
            name
          }
        }
    })";
    
    String response;
    extern const char* API_URL;
    if (makeGraphQLRequest(query, response, API_URL)) {
        // Parse JSON response manually to avoid ArduinoJson
        int dataPos = response.indexOf("\"data\":");
        if (dataPos >= 0) {
            int nameStart = response.indexOf("\"name\":\"", dataPos);
            if (nameStart >= 0) {
                nameStart += 8; // Length of "\"name\":\""
                int nameEnd = response.indexOf("\"", nameStart);
                if (nameEnd >= 0) {
                    return response.substring(nameStart, nameEnd);
                }
            }
        }
    }
    return "";
}

bool initializeGateway(const String& idAndWallet, const String& signature, bool dryRun, String& responseData) {
    String mutation = String(R"(
      mutation {
        gatewayInception {
          initialize(gatewayInitialization:{)") + 
          (dryRun ? "dryRun:true, " : "") +
          "idAndWallet:\"" + idAndWallet + "\", " +
          "signature:\"" + signature + "\"}) {" + R"(
            initialized
          }
        }
      }
    )";
    
    extern const char* API_URL;
    return makeGraphQLRequest(mutation, responseData, API_URL);
} 