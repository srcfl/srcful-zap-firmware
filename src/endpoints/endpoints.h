#pragma once

#include "endpoint_types.h"


#include "endpoint_handlers.h"
#include "ota/ota_handler.h"
#include "wifi_endpoint_handlers.h"
#include "system_endpoint_handlers.h"
#include "data_reader_endpoint_handlers.h"
#include "modbus_endpoint_handlers.h"

// Create handler instances
extern WiFiConfigHandler g_wifiConfigHandler;
extern SystemInfoHandler g_systemInfoHandler;
extern SystemRebootHandler g_systemRebootHandler;
extern WiFiResetHandler g_wifiResetHandler;
extern CryptoInfoHandler g_cryptoInfoHandler;
extern NameInfoHandler g_nameInfoHandler;
extern WiFiStatusHandler g_wifiStatusHandler;
extern WiFiScanHandler g_wifiScanHandler;
extern InitializeHandler g_initializeHandler;
extern CryptoSignHandler g_cryptoSignHandler;
extern OTAUpdateHandler g_otaUpdateHandler;
extern EchoHandler g_echoHandler;
extern BLEStopHandler g_bleStopHandler;
extern DebugHandler g_debugHandler;
extern OTAUpdateHandler g_otaUpdateHandler;
extern OTAStatusHandler g_otaStatusHandler;
extern DataReaderGetHandler g_dataReaderGetHandler;
extern ModbusTcpHandler g_modbusTcpHandler;