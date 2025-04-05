#pragma once

#include <Arduino.h>
#include <esp_random.h>

// Helper function declarations
String base64url_encode(const char* data, size_t length);

class Crypto {
public:
    static bool generateKeyPair(uint8_t* privateKey, uint8_t* publicKey);
    static bool computeSharedSecret(const uint8_t* privateKey, const uint8_t* publicKey, uint8_t* sharedSecret);
    static bool verifySignature(const uint8_t* publicKey, const uint8_t* message, size_t messageLen, const uint8_t* signature);
    static bool signMessage(const uint8_t* privateKey, const uint8_t* message, size_t messageLen, uint8_t* signature);
};

// Legacy functions for backward compatibility
String crypto_get_public_key(const char* private_key_hex);
String crypto_create_jwt(const char* header, const char* payload, const char* private_key_hex);
String crypto_create_signature_base64url(const char* data, const char* private_key_hex);
String crypto_create_signature_hex(const char* data, const char* private_key_hex); 