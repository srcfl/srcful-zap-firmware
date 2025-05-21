#include "crypto.h"
#include <NimBLEDevice.h>
#include <esp_random.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/sha256.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#include <esp_efuse.h>
#include <esp_system.h>
// #include <Arduino.h>
#include <esp_random.h>
#include <soc/efuse_reg.h>

#include "zap_log.h"

static const char* TAG = "crypto";

static int crypto_convert_to_der(const uint8_t *signature, uint8_t *der, size_t *der_len);

// Helper functions
static bool hex_string_to_bytes(const char* hex_string, uint8_t* bytes, size_t length) {
    if (strlen(hex_string) != length * 2) {
        return false;
    }
    
    for (size_t i = 0; i < length; i++) {
        int value;
        if (sscanf(hex_string + 2*i, "%2x", &value) != 1) {
            return false;
        }
        bytes[i] = (uint8_t)value;
    }
    return true;
}

void bytes_to_hex_string(const uint8_t* bytes, size_t length, char* hex_string) {
    for (size_t i = 0; i < length; i++) {
        sprintf(hex_string + 2*i, "%02x", bytes[i]);
    }
    hex_string[length * 2] = '\0';
}

// Base64URL encoding implementation
zap::Str base64url_encode(const char* data, size_t length) {
    static const char base64url_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    zap::Str result;
    result.reserve(((length + 2) / 3) * 4);
    
    for (size_t i = 0; i < length; i += 3) {
        uint32_t octet_a = i < length ? (uint8_t)data[i] : 0;
        uint32_t octet_b = i + 1 < length ? (uint8_t)data[i + 1] : 0;
        uint32_t octet_c = i + 2 < length ? (uint8_t)data[i + 2] : 0;
        
        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        result += base64url_chars[(triple >> 18) & 0x3F];
        result += base64url_chars[(triple >> 12) & 0x3F];
        result += base64url_chars[(triple >> 6) & 0x3F];
        result += base64url_chars[triple & 0x3F];
    }
    
    int mod = length % 3;
    if (mod) {
        result = result.substring(0, result.length() - (3 - mod));
        // Remove padding - don't add '=' characters
        // while (mod++ < 3) result += '=';
    }
    
    return result;
}

// Crypto class implementation
bool Crypto::generateKeyPair(uint8_t* privateKey, uint8_t* publicKey) {
    // Use mbedtls to derive the public key from the private key
    
    // Initialize mbedtls structures
    mbedtls_ecp_keypair keypair;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    int ret;
    
    // Initialize the structures
    mbedtls_ecp_keypair_init(&keypair);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    size_t olen = 64;  // Initialize with the buffer size

    // Debug: Print the public key point coordinates
    char buf[1024];
    size_t len = 0;
    
    // Set up the entropy source and random number generator
    const char *pers = "ecdsa_keygen";
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        LOG_E(TAG, "Failed to seed random number generator");
        goto cleanup;
    }
    
    // Set up the ECP group (SECP256R1 curve)
    ret = mbedtls_ecp_group_load(&keypair.grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0) {
        LOG_E(TAG, "Failed to load ECP group");
        goto cleanup;
    }
    
    // Import the private key
    ret = mbedtls_mpi_read_binary(&keypair.d, privateKey, 32);
    if (ret != 0) {
        LOG_E(TAG, "Failed to import private key");
        goto cleanup;
    }
    
    // Derive the public key from the private key
    ret = mbedtls_ecp_mul(&keypair.grp, &keypair.Q, &keypair.d, &keypair.grp.G, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        LOG_E(TAG, "Failed to derive public key");
        goto cleanup;
    }
    
    // Export X coordinate
    ret = mbedtls_mpi_write_binary(&keypair.Q.X, publicKey, 32);
    if (ret != 0) {
        LOG_E(TAG, "Failed to export X coordinate");
        goto cleanup;
    }
    
    // Export Y coordinate
    ret = mbedtls_mpi_write_binary(&keypair.Q.Y, publicKey + 32, 32);
    if (ret != 0) {
        LOG_E(TAG, "Failed to export Y coordinate");
        goto cleanup;
    }
    
    // Clean up
    mbedtls_ecp_keypair_free(&keypair);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    
    return true;
    
cleanup:
    mbedtls_ecp_keypair_free(&keypair);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    
    return false;
}

bool Crypto::signMessage(const uint8_t* privateKey, const uint8_t* message, size_t messageLen, uint8_t* signature) {
    // Use mbedtls for ECDSA signatures
    
    // Initialize mbedtls structures
    mbedtls_ecdsa_context ecdsa;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_mpi r, s, d;
    unsigned char hash[32];
    int ret;
    
    // Initialize the structures
    mbedtls_ecdsa_init(&ecdsa);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);
    mbedtls_mpi_init(&d);
    
    // Set up the entropy source and random number generator
    const char *pers = "ecdsa_sign";
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        LOG_E(TAG, "Failed to seed random number generator");
        goto cleanup;
    }
    
    // Set up the ECDSA context
    ret = mbedtls_ecp_group_load(&ecdsa.grp, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0) {
        LOG_E(TAG, "Failed to load ECP group");
        goto cleanup;
    }
    
    // Import the private key
    ret = mbedtls_mpi_read_binary(&d, privateKey, 32);
    if (ret != 0) {
        LOG_E(TAG, "Failed to import private key");
        goto cleanup;
    }
    
    // Hash the message
    mbedtls_sha256(message, messageLen, hash, 0);
    
    // Sign the hash
    ret = mbedtls_ecdsa_sign(&ecdsa.grp, &r, &s, &d, hash, 32, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        LOG_E(TAG, "Failed to sign message");
        goto cleanup;
    }
    
    // Export the signature
    ret = mbedtls_mpi_write_binary(&r, signature, 32);
    if (ret != 0) {
        LOG_E(TAG, "Failed to export r value");
        goto cleanup;
    }
    
    ret = mbedtls_mpi_write_binary(&s, signature + 32, 32);
    if (ret != 0) {
        LOG_E(TAG, "Failed to export s value");
        goto cleanup;
    }
    
    // Clean up
    mbedtls_mpi_free(&d);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    mbedtls_ecdsa_free(&ecdsa);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    
    return true;
    
cleanup:
    mbedtls_mpi_free(&d);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&s);
    mbedtls_ecdsa_free(&ecdsa);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    
    return false;
}

// Legacy function implementations
zap::Str crypto_get_public_key(const char* private_key_hex) {
    uint8_t privateKey[32];
    uint8_t publicKey[64];
    
    if (!hex_string_to_bytes(private_key_hex, privateKey, 32)) {
        LOG_E(TAG, "Failed to convert private key hex to bytes");
        return zap::Str();
    }
    
    if (!Crypto::generateKeyPair(privateKey, publicKey)) {
        LOG_E(TAG, "Failed to generate key pair");
        return zap::Str();
    }
    
    char hex_result[129];  // Changed to 129 to accommodate 64 bytes (128 hex chars + null terminator)
    bytes_to_hex_string(publicKey, 64, hex_result);
    return zap::Str(hex_result);
}

zap::Str crypto_create_jwt(const char* header, const char* payload, const char* private_key_hex) {
    uint8_t privateKey[32];
    if (!hex_string_to_bytes(private_key_hex, privateKey, 32)) {
        return zap::Str();
    }
    
    // Create the JWT parts
    zap::Str encodedHeader = base64url_encode(header, strlen(header));
    zap::Str encodedPayload = base64url_encode(payload, strlen(payload));
    zap::Str signatureInput = encodedHeader + "." + encodedPayload;
    
    // Create signature
    uint8_t signature[64];  // Changed to 64 bytes
    if (!Crypto::signMessage(privateKey, (const uint8_t*)signatureInput.c_str(), signatureInput.length(), signature)) {
        return zap::Str();
    }
    
    // Encode signature
    zap::Str encodedSignature = base64url_encode((const char*)signature, 64);  // Changed to 64 bytes
    
    // Combine all parts
    return encodedHeader + "." + encodedPayload + "." + encodedSignature;
}

zap::Str crypto_create_signature_base64url(const char* data, const char* private_key_hex) {
    uint8_t privateKey[32];
    if (!hex_string_to_bytes(private_key_hex, privateKey, 32)) {
        return zap::Str();
    }
    
    uint8_t signature[64];  // Changed to 64 bytes
    if (!Crypto::signMessage(privateKey, (const uint8_t*)data, strlen(data), signature)) {
        return zap::Str();
    }
    
    return base64url_encode((const char*)signature, 64);  // Changed to 64 bytes
}

zap::Str crypto_create_signature_hex(const char* data, const char* private_key_hex) {
    uint8_t privateKey[32];
    if (!hex_string_to_bytes(private_key_hex, privateKey, 32)) {
        return zap::Str();
    }
    
    uint8_t signature[64];  // Changed to 64 bytes
    if (!Crypto::signMessage(privateKey, (const uint8_t*)data, strlen(data), signature)) {
        return zap::Str();
    }
    
    char hex_result[129];  // Changed to 129 to accommodate 64 bytes (128 hex chars + null terminator)
    bytes_to_hex_string(signature, 64, hex_result);  // Changed to 64 bytes
    return zap::Str(hex_result);
} 


zap::Str crypto_create_signature_der_hex(const char* data, const char* private_key_hex) {
    uint8_t signature[64];

    uint8_t privateKey[32];
    if (!hex_string_to_bytes(private_key_hex, privateKey, 32)) {
        return zap::Str();
    }
    
    if (!Crypto::signMessage(privateKey, (const uint8_t*)data, strlen(data), signature)) {
        return zap::Str();
    }

    uint8_t der[64 + 6];
    size_t der_len;
    if (!crypto_convert_to_der(signature, der, &der_len)) {
        return zap::Str();
    }

    char der_hex[der_len * 2];
    bytes_to_hex_string(der, der_len, der_hex);
    return zap::Str(der_hex);
}

static int crypto_convert_to_der(const uint8_t *signature, uint8_t *der, size_t *der_len) {
    uint8_t r[32], s[32];
    memcpy(r, signature, 32);
    memcpy(s, signature + 32, 32);

    int r_len = 32, s_len = 32;

    // Remove leading zeros for DER encoding without modifying the pointer
    int r_offset = 0, s_offset = 0;

    // For r: Shift data left until there are no leading zeros
    while (r_len > 1 && r[r_offset] == 0x00 && (r[r_offset + 1] & 0x80) == 0)
    {
        r_offset++;
        r_len--;
    }

    // For s: Shift data left until there are no leading zeros
    while (s_len > 1 && s[s_offset] == 0x00 && (s[s_offset + 1] & 0x80) == 0)
    {
        s_offset++;
        s_len--;
    }

    // DER format
    uint8_t *p = der;
    *p++ = 0x30;                  // SEQUENCE
    *p++ = 2 + r_len + 2 + s_len; // Total length
    *p++ = 0x02;
    *p++ = r_len;                   // INTEGER (r)
    memcpy(p, r + r_offset, r_len); // Copy the modified r
    p += r_len;
    *p++ = 0x02;
    *p++ = s_len;                   // INTEGER (s)
    memcpy(p, s + s_offset, s_len); // Copy the modified s
    p += s_len;

    *der_len = p - der;
    return 1;
} 

zap::Str crypto_getId() {
  // Debug flag
  uint64_t chipId = ESP.getEfuseMac();
  
  // Convert 64-bit chipId to 16 hex characters
  char serial[17]; // 16 hex chars + null terminator
  for (int i = 0; i < 8; i++) {
    uint8_t byte = (chipId >> ((7-i) * 8)) & 0xFF;
    char hexDigits[] = "0123456789abcdef"; // lowercase for consistency
    serial[i*2] = hexDigits[(byte >> 4) & 0xF];
    serial[i*2+1] = hexDigits[byte & 0xF];
  }
  serial[16] = '\0';
  
  zap::Str id = "zap-" + zap::Str(serial);
  
  while (id.length() < 18) {
    // Pad with 'e' if shorter
    id += 'e';
  }
  
  return id;
}

bool crypto_create_private_key(uint8_t* privateKey) {
    // Use mbedtls to generate a new private key
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ecp_keypair keypair;
    int ret;

    // Initialize the structures
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_ecp_keypair_init(&keypair);

    // Set up the random number generator
    const char *pers = "ecdsa_genkey";
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        LOG_E(TAG, "Failed to seed random number generator");
        return false;
    }

    // Generate the key pair
    ret = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, &keypair, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        LOG_E(TAG, "Failed to generate key pair");
        return false;
    }

    // Export the private key
    ret = mbedtls_mpi_write_binary(&keypair.d, privateKey, 32);
    if (ret != 0) {
        LOG_E(TAG, "Failed to export private key");
        return false;
    }

    // Clean up
    mbedtls_ecp_keypair_free(&keypair);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);

    return true;    
}