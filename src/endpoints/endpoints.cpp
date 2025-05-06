#include <Arduino.h>
#include "json_light/json_light.h"
#include "endpoint_types.h"
#include "endpoints.h"
#include <WiFi.h>
#include <esp_system.h>
#include "crypto.h"
#include "../backend/graphql.h"
#include "firmware_version.h"
#include "wifi/wifi_manager.h"
#include "endpoint_handlers.h"
#include "system_endpoint_handlers.h"

// External variable declarations
extern WifiManager wifiManager;
extern BLEHandler bleHandler;

WiFiConfigHandler g_wifiConfigHandler(wifiManager);
SystemInfoHandler g_systemInfoHandler(wifiManager);
SystemRebootHandler g_systemRebootHandler;
WiFiResetHandler g_wifiResetHandler(wifiManager);
CryptoInfoHandler g_cryptoInfoHandler;
NameInfoHandler g_nameInfoHandler;
WiFiStatusHandler g_wifiStatusHandler(wifiManager);
WiFiScanHandler g_wifiScanHandler(wifiManager);
InitializeHandler g_initializeHandler;
CryptoSignHandler g_cryptoSignHandler;
OTAUpdateHandler g_otaUpdateHandler;
EchoHandler g_echoHandler;
DebugHandler g_debugHandler;
BLEStopHandler g_bleStopHandler(bleHandler);


