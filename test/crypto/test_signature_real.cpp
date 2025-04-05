#include <unity.h>
#include <Arduino.h>
#include "crypto.h"

// Test-specific constants
const char* TEST_PRIVATE_KEY_HEX = "4cc43b88635b9eaf81655ed51e062fab4a46296d72f01fc6fd853b08f0c2383a";

void test_crypto_create_signature(void) {
    // Test data
    const char* testMessage = "test_message_for_signature_verification";
    
    // Generate signature using the actual implementation
    String hexSignature = crypto_create_signature_der_hex(testMessage, TEST_PRIVATE_KEY_HEX);
    
    // Print the signature for verification
    Serial.print("Test message: ");
    Serial.println(testMessage);
    Serial.print("test message hex: ");
    // Convert testMessage to hex string manually since toHexString() is undefined
    String hexMsg;
    for(size_t i = 0; i < strlen(testMessage); i++) {
        char hex[3];
        sprintf(hex, "%02x", (unsigned char)testMessage[i]);
        hexMsg += hex;
    }
    Serial.println(hexMsg);
    Serial.print("Generated signature (der format): ");
    Serial.println(hexSignature);
    TEST_MESSAGE("Hello world");
    
    // Basic validation
    TEST_ASSERT_NOT_EQUAL(0, hexSignature.length());
    TEST_ASSERT_EQUAL(128, hexSignature.length()); // Expected length for a 64-byte signature in hex

    TEST_ASSERT_EQUAL_STRING(hexSignature.c_str(), "b30c309bb2530a74ece980021de0fc29fd030aba3dbb6c631f29b80a9d083bdd70d43913fb4dfd07c98d6bc87304ba306df4d1e60b1f23baffa4e6714315ab52");
} 