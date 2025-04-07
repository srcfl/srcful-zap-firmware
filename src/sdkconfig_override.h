#ifndef SDKCONFIG_OVERRIDE_H
#define SDKCONFIG_OVERRIDE_H

// This file is used to suppress redefinition warnings from the ESP32 SDK
// It should be included before any other headers that might define these macros

// Suppress CONFIG_BOOTLOADER_RESERVE_RTC_SIZE redefinition
#ifdef CONFIG_BOOTLOADER_RESERVE_RTC_SIZE
#undef CONFIG_BOOTLOADER_RESERVE_RTC_SIZE
#endif

// Suppress MBEDTLS_ECP_DP_SECP256R1_ENABLED redefinition
#ifdef MBEDTLS_ECP_DP_SECP256R1_ENABLED
#undef MBEDTLS_ECP_DP_SECP256R1_ENABLED
#endif

// Suppress MBEDTLS_ECDSA_C redefinition
#ifdef MBEDTLS_ECDSA_C
#undef MBEDTLS_ECDSA_C
#endif

// Suppress NIMBLE_MAX_CONNECTIONS redefinition
#ifdef NIMBLE_MAX_CONNECTIONS
#undef NIMBLE_MAX_CONNECTIONS
#endif

#endif // SDKCONFIG_OVERRIDE_H 