#include <Arduino.h>
#include <unity.h>

#include "crypto/config.cpp"

// Include test files
#include "crypto/test_signature_real.cpp"
#include "crypto/test_crypto_sign_endpoint.cpp"


void setup() {
    delay(2000);  // Give serial monitor time to connect
    Serial.begin(115200);
    
    UNITY_BEGIN();
    RUN_TEST(test_crypto_create_signature);
    RUN_TEST(test_handle_crypto_sign_endpoint);
    UNITY_END();
}

void loop() {
    delay(1000);
} 