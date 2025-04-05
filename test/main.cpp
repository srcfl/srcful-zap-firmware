#include <Arduino.h>
#include <unity.h>

// Include the test file for the real implementation
#include "crypto/test_signature_real.cpp"

void setup() {
    delay(2000);  // Give serial monitor time to connect
    Serial.begin(115200);
    
    UNITY_BEGIN();
    RUN_TEST(test_crypto_create_signature);
    UNITY_END();
}

void loop() {
    delay(1000);
} 