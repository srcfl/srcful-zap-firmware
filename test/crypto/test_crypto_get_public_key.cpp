#include <unity.h>
#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include "../src/endpoints/endpoint_types.h"
#include "config.h"
#include "crypto.h"


// Test the handleCryptoSign endpoint
void test_crypto_get_public_key(void) {
    String publicKey = crypto_get_public_key(PRIVATE_KEY_HEX);
    Serial.println("Public key: ");
    Serial.println(publicKey);
    TEST_ASSERT_EQUAL_STRING(EXPECTED_PUBLIC_KEY_HEX, publicKey.c_str());
} 