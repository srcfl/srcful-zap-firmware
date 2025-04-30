#include <unity.h>
#include "endpoints/wifi_endpoint_handlers.h"
#include "../src/wifi/wifi_manager.h"

class MockWiFiManager : public WifiManager {
public:
    MockWiFiManager() : WifiManager("test") {
        // Initialize mock data if needed
    }

    bool isConnected() const { return true; }
    zap::Str getConfiguredSSID() const { return "helloworld"; }
    const std::vector<zap::Str>& getLastScanResults() const { 
        static std::vector<zap::Str> mockResults = {"Network1", "Network2", "Network3"};
        return mockResults;
    }
};

void test_wifi_status_handler() {
    // Create a mock WiFiManager
    MockWiFiManager wifiManager;
    
    // Create a WiFiStatusHandler instance
    WiFiStatusHandler wifiStatusHandler(wifiManager);
    
    // Call the handle method with an empty string
    EndpointResponse response = wifiStatusHandler.handle("");
    
    // Print the response contents
    Serial.println("==== WiFi Status Handler Test ====");
    Serial.print("Status Code: ");
    Serial.println(response.statusCode);
    Serial.print("Content Type: ");
    Serial.println(response.contentType.c_str());
    Serial.print("Response Data: ");
    Serial.println(response.data.c_str());
    Serial.println("=================================");
    
    // Basic validation
    TEST_ASSERT_EQUAL(200, response.statusCode);
    TEST_ASSERT_EQUAL_STRING("application/json", response.contentType.c_str());

    // Check that the response contains the expected fields
    JsonParser parser(response.data.c_str());
    char buffer[128];
    TEST_ASSERT_TRUE(parser.getString("connected", buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING("helloworld", buffer);
   
}