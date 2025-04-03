#include "crypto.h"

// Private function declarations
static bool hex_string_to_bytes(const char* hex_string, uint8_t* bytes, size_t length);
static void bytes_to_hex_string(const uint8_t* bytes, size_t length, char* hex_string);
static String base64url_encode(const uint8_t* data, size_t length);
static int get_random(uint8_t *dest, unsigned size);
static void calculate_sha256(const char* input, uint8_t* output);
static const struct uECC_Curve_t* create_curve(void);
static bool create_signature(const char* data, const char* private_key_hex, uint8_t* signature_out);

String crypto_get_public_key(const char* private_key_hex) {
    const struct uECC_Curve_t* curve = create_curve();
    if (!curve) {
        return String();
    }

    uint8_t private_key[32];
    uint8_t public_key[64];
    
    if (!hex_string_to_bytes(private_key_hex, private_key, 32)) {
        return String();
    }
    
    if (!uECC_compute_public_key(private_key, public_key, curve)) {
        return String();
    }
    
    char public_key_hex[129];
    bytes_to_hex_string(public_key, 64, public_key_hex);
    return String(public_key_hex);
}

String crypto_create_jwt(const char* header, const char* payload, const char* private_key_hex) {
    // Create base64url encoded parts
    String header_b64url = base64url_encode((const uint8_t*)header, strlen(header));
    String payload_b64url = base64url_encode((const uint8_t*)payload, strlen(payload));
    
    if (header_b64url.length() == 0 || payload_b64url.length() == 0) {
        return String();
    }
    
    // Create message to sign
    String message = header_b64url + "." + payload_b64url;
    
    // Create signature
    String signature = crypto_create_signature_base64url(message.c_str(), private_key_hex);
    if (signature.length() == 0) {
        return String();
    }
    
    // Combine all parts
    return message + "." + signature;
}

String crypto_create_signature_base64url(const char* data, const char* private_key_hex) {
    uint8_t signature[64];
    
    if (!create_signature(data, private_key_hex, signature)) {
        return String();
    }
    
    // Convert to base64url
    return base64url_encode(signature, 64);
}

String crypto_create_signature_hex(const char* data, const char* private_key_hex) {
    uint8_t signature[64];
    
    if (!create_signature(data, private_key_hex, signature)) {
        return String();
    }
    
    // Convert to hex string
    char signature_hex[129];  // 64 bytes * 2 chars per byte + null terminator
    bytes_to_hex_string(signature, 64, signature_hex);
    return String(signature_hex);
}

// Private helper functions
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

static void bytes_to_hex_string(const uint8_t* bytes, size_t length, char* hex_string) {
    for (size_t i = 0; i < length; i++) {
        sprintf(hex_string + 2*i, "%02x", bytes[i]);
    }
    hex_string[length * 2] = '\0';
}

static String base64url_encode(const uint8_t* data, size_t length) {
    size_t b64len = (length + 2) / 3 * 4 + 1;
    char b64[b64len];
    size_t olen;
    
    int result = mbedtls_base64_encode((unsigned char*)b64, b64len, &olen, data, length);
    if (result != 0) {
        return String();
    }
    
    String b64url = String(b64);
    b64url.replace("+", "-");
    b64url.replace("/", "_");
    b64url.replace("=", "");
    
    return b64url;
}

static int get_random(uint8_t *dest, unsigned size) {
    esp_fill_random(dest, size);
    return 1;
}

static void calculate_sha256(const char* input, uint8_t* output) {
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char*)input, strlen(input));
    mbedtls_md_finish(&ctx, output);
    mbedtls_md_free(&ctx);
}

static const struct uECC_Curve_t* create_curve(void) {
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    if (curve) {
        uECC_set_rng(&get_random);
    }
    return curve;
}

// Add the common signature creation function
static bool create_signature(const char* data, const char* private_key_hex, uint8_t* signature_out) {
    const struct uECC_Curve_t* curve = create_curve();
    if (!curve) {
        return false;
    }

    uint8_t private_key[32];
    uint8_t hash[32];
    
    if (!hex_string_to_bytes(private_key_hex, private_key, 32)) {
        return false;
    }
    
    // Calculate hash
    calculate_sha256(data, hash);
    
    // Sign the hash
    return uECC_sign(private_key, hash, 32, signature_out, curve);
} 