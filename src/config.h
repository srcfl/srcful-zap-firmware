#pragma once

#include <Arduino.h>

// Device Configuration
extern const char* MDNS_NAME;       // mDNS name - device will be accessible as zap.local
extern const char* METER_SN;        // Serial number of the meter, this is hard coded in firmware but can be changed in the app if the user desires

// API Configuration
extern const char* API_URL;
extern const char* DATA_URL;

// Cryptographic Configuration
extern const char* PRIVATE_KEY_HEX;
extern const char* EXPECTED_PUBLIC_KEY_HEX;

// MQTT Configuration
extern const char* MQTT_SERVER;
extern const uint16_t MQTT_PORT;
extern const bool MQTT_USE_SSL;
extern const char* MQTT_CA_CERT;
// Note: MQTT_USERNAME and MQTT_PASSWORD are set dynamically in main.cpp

#define LED_PIN 3

