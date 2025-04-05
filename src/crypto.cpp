#include "crypto.h"

// Private function declarations
static bool hex_string_to_bytes(const char* hex_string, uint8_t* bytes, size_t length);
static void bytes_to_hex_string(const uint8_t* bytes, size_t length, char* hex_string);
static String base64url_encode(const uint8_t* data, size_t length);
static int get_random(uint8_t *dest, unsigned size);
static void calculate_sha256(const char* input, uint8_t* output);
static const struct uECC_Curve_t* create_curve(void);
static bool create_signature(const char* data, const char* private_key_hex, uint8_t* signature_out);
static int crypto_convert_to_der(const uint8_t *signature, uint8_t *der, size_t *der_len);

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

String crypto_create_signature_der_hex(const char* data, const char* private_key_hex) {
    uint8_t signature[64];
    
    if (!create_signature(data, private_key_hex, signature)) {
        return String();
    }

    uint8_t der[64 + 6];
    size_t der_len;
    if (!crypto_convert_to_der(signature, der, &der_len)) {
        return String();
    }

    char der_hex[der_len * 2];
    bytes_to_hex_string(der, der_len, der_hex);
    return String(der_hex);
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