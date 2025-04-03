#ifndef CRYPTO_H
#define CRYPTO_H

#include <Arduino.h>
#include <uECC.h>
#include "mbedtls/md.h"
#include <mbedtls/base64.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the crypto module
bool crypto_init(void);

// Convert a hex string private key to public key hex string
// Returns empty string on failure
String crypto_get_public_key(const char* private_key_hex);

// Create a complete JWT from header and payload
// Returns empty string on failure
String crypto_create_jwt(const char* header, const char* payload, const char* private_key_hex);

// Create a signature for arbitrary data
// Returns empty string on failure
String crypto_create_signature_base64url(const char* data, const char* private_key_hex);

// Create a signature and return it as hex string
String crypto_create_signature_hex(const char* data, const char* private_key_hex);

#ifdef __cplusplus
}
#endif

#endif 