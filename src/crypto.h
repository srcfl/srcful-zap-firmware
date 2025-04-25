#pragma once


#include "zap_str.h"

// Helper function declarations
zap::Str base64url_encode(const char* data, size_t length);

class Crypto {
public:
    static bool generateKeyPair(uint8_t* privateKey, uint8_t* publicKey);
    static bool verifySignature(const uint8_t* publicKey, const uint8_t* message, size_t messageLen, const uint8_t* signature);
    static bool signMessage(const uint8_t* privateKey, const uint8_t* message, size_t messageLen, uint8_t* signature);
};


void bytes_to_hex_string(const uint8_t* bytes, size_t length, char* hex_string);

bool crypto_create_private_key(uint8_t* privateKey);    // this need to be 32 bytes

// Legacy functions for backward compatibility
zap::Str crypto_get_public_key(const char* private_key_hex);
zap::Str crypto_create_jwt(const char* header, const char* payload, const char* private_key_hex);
zap::Str crypto_create_signature_base64url(const char* data, const char* private_key_hex);

// Create a signature and return it as hex string
zap::Str crypto_create_signature_hex(const char* data, const char* private_key_hex);

// Create a signature and return it as DER string in hex format
zap::Str crypto_create_signature_der_hex(const char* data, const char* private_key_hex);

zap::Str crypto_getId();