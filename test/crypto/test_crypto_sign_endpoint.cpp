#include <unity.h>
#include <Arduino.h>
#include "config.h"
#include "crypto.h"
#include "endpoints.h"
#include "endpoint_types.h"


// Test the handleCryptoSign endpoint
void test_handle_crypto_sign_endpoint(void) {
    // Create a test endpoint
    Endpoint endpoint(Endpoint::CRYPTO_SIGN, Endpoint::Verb::POST, "/api/crypto/sign", nullptr);
    
    // Create a test request with a message
    EndpointRequest request(endpoint);
    request.content = "{\"message\":\"Bygcy876b3bsjMvvhZxghvs3EyR5y6a7vpvAp5D62n2w\",\"timestamp\":\"2025-04-06T08:33:00Z\"}";
    
    // Call the endpoint handler
    EndpointResponse response = handleCryptoSign(request);
    
    // Print the response contents
    Serial.println("==== Crypto Sign Endpoint Test ====");
    Serial.print("Status Code: ");
    Serial.println(response.statusCode);
    Serial.print("Content Type: ");
    Serial.println(response.contentType);
    Serial.print("Response Data: ");
    Serial.println(response.data);
    Serial.println("=================================");
    
    // Basic validation
    TEST_ASSERT_EQUAL(200, response.statusCode);
    TEST_ASSERT_EQUAL_STRING("application/json", response.contentType.c_str());
    TEST_ASSERT_NOT_EQUAL(0, response.data.length());
    
    // Check that the response contains the expected fields
    TEST_ASSERT_TRUE(response.data.indexOf("message") != -1);
    TEST_ASSERT_TRUE(response.data.indexOf("sign") != -1);
    TEST_ASSERT_TRUE(response.data.indexOf("Bygcy876b3bsjMvvhZxghvs3EyR5y6a7vpvAp5D62n2w") != -1);
} 