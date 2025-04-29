#pragma once

// WiFi Configuration
extern const char* WIFI_SSID;
extern const char* WIFI_PSK;
// Device Configuration
extern const char* AP_SSID;  // Name of the WiFi network created by ESP32
extern const char* AP_PASSWORD;     // Password for the setup network
extern const char* MDNS_NAME;        // mDNS name - device will be accessible as myesp32.local

// API Configuration
extern const char* API_URL;
extern const char* DATA_URL;

// Cryptographic Configuration
extern const char* PRIVATE_KEY_HEX;
extern const char* EXPECTED_PUBLIC_KEY_HEX;

#define LED_PIN 3

