#include "config.h"

// WiFi Configuration
const char* WIFI_SSID = "";
const char* WIFI_PSK = "";
// Device Configuration
const char* AP_SSID = "ZAP_Setup_V2";  // Name of the WiFi network created by ESP32
const char* AP_PASSWORD = "12345678";     // Password for the setup network
const char* MDNS_NAME = "zap";        // mDNS name - device will be accessible as zap.local

const char* METER_SN = "zap";        // Serial number of the meter, this is hard coded in firmware but can be changed in the app if the user desires


// API Configuration
const char* API_URL = "https://api.srcful.dev/";
const char* DATA_URL = "https://mainnet.srcful.dev/gw/data/";

// Cryptographic Configuration
// const char* PRIVATE_KEY_HEX = "4cc43b88635b9eaf81655ed51e062fab4a46296d72f01fc6fd853b08f0c2383a";
// const char* EXPECTED_PUBLIC_KEY_HEX = "3e70c4705ff5945bfea058aaa68128e6f7d54fd7e08c640f4791668f8267a6e8c36ee19214698f1956e948bf339492fb11e0dc5a79a76dd0c235b431ee5aa782";


const char* PRIVATE_KEY_HEX = "";
const char* EXPECTED_PUBLIC_KEY_HEX = "";