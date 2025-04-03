#include "ble_constants.h"

// BLE Service and Characteristic UUIDs
const char* SRCFUL_SERVICE_UUID = "0fda92b2-44a2-4af2-84f5-fa682baa2b8d";
const char* SRCFUL_REQUEST_CHAR_UUID = "51ff12bb-3ed8-46e5-b4f9-d64e2fec021b";
const char* SRCFUL_RESPONSE_CHAR_UUID = "51ff12bb-3ed8-46e5-b4f9-d64e2fec021c";

// Protocol Constants
const char* EGWTP_VERSION = "EGWTTP/1.1";
const size_t MAX_BLE_PACKET_SIZE = 512;
const int SRCFUL_GW_API_REQUEST_TIMEOUT = 300000; // 5 minutes in milliseconds 