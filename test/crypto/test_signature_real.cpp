#include <unity.h>
#include <Arduino.h>
#include "crypto.h"
#include "./config.h"
// Test-specific constants

void test_crypto_create_signature(void) {
    // Test data
    const char* testMessage = "test_message_for_signature_verification";
    
    // Generate signature using the actual implementation
    zap::Str hexSignature = crypto_create_signature_der_hex(testMessage, PRIVATE_KEY_HEX);
    
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
    Serial.println(hexSignature.c_str());
    
    // Basic validation
    TEST_ASSERT_NOT_EQUAL(0, hexSignature.length());
    TEST_ASSERT_GREATER_THAN(128, hexSignature.length()); // Expected length for a 64-byte signature in hex

} 