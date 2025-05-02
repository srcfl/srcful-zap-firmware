#include "../src/zap_str.h"

zap::Str crypto_create_signature_hex(const char* data, const char* private_key_hex) {
    
    char hex_result[129]; 
    return zap::Str(hex_result);
}

zap::Str crypto_getId() {
    
    return "zap-testtesttestee";
}

zap::Str crypto_create_jwt(const char* header, const char* payload, const char* private_key_hex) {
    
    return zap::Str("a.b.c");
}