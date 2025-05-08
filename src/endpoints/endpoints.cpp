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
extern OTAHandler g_otaHandler;


SystemRebootHandler g_systemRebootHandler;

WiFiResetHandler g_wifiResetHandler(wifiManager);
WiFiStatusHandler g_wifiStatusHandler(wifiManager);
WiFiScanHandler g_wifiScanHandler(wifiManager);
WiFiConfigHandler g_wifiConfigHandler(wifiManager);
SystemInfoHandler g_systemInfoHandler(wifiManager);


CryptoInfoHandler g_cryptoInfoHandler;
NameInfoHandler g_nameInfoHandler;

InitializeHandler g_initializeHandler;
CryptoSignHandler g_cryptoSignHandler;

OTAUpdateHandler g_otaUpdateHandler(g_otaHandler);
OTAStatusHandler g_otaStatusHandler(g_otaHandler);

EchoHandler g_echoHandler;
DebugHandler g_debugHandler;
BLEStopHandler g_bleStopHandler(bleHandler);


