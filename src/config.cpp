#include "config.h"

const char* MDNS_NAME = "zap";        // mDNS name - device will be accessible as zap.local
const char* METER_SN = "zap";        // Serial number of the meter, this is hard coded in firmware but can be changed in some future release


// API Configuration
const char* API_URL = "https://api.srcful.dev/";
const char* DATA_URL = "https://mainnet.srcful.dev/gw/data/";

const char* PRIVATE_KEY_HEX = "";
const char* EXPECTED_PUBLIC_KEY_HEX = "";