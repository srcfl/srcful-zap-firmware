#include <Arduino.h>
#include <unity.h>

// Include configuration
#include "crypto/config.cpp"

// Include implementation files
#include "../src/crypto.cpp"
#include "../src/endpoints/handleCryptoSign.cpp"
#include "../src/endpoints/wifi_endpoint_handlers.cpp"
#include "../src/wifi/wifi_manager.cpp"
#include "../src/json_light/json_light.cpp"


// Include test files
#include "crypto/test_signature_real.cpp"
#include "crypto/test_crypto_get_public_key.cpp"
#include "crypto/test_crypto_sign_endpoint.cpp"
#include "json_light/test_json_light.cpp"
#include "endpoints/test_wifi_endpoint_handlers.cpp"


// Define the CryptoSignHandler instance that's referenced in the tests
CryptoSignHandler g_cryptoSignHandler;

void setup() {
    delay(2000);  // Give serial monitor time to connect
    Serial.begin(115200);
    
    UNITY_BEGIN();
    RUN_TEST(test_crypto_create_signature);
    RUN_TEST(test_crypto_get_public_key);
    RUN_TEST(test_handle_crypto_sign_endpoint);
    RUN_TEST(test_wifi_status_handler);
    RUN_TEST(test_json_builder);
    RUN_TEST(test_json_parser);

    test_all_json_light();

    UNITY_END();

    
}

void loop() {
    delay(1000);
} 