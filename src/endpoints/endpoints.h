#pragma once

#include "endpoint_types.h"




// Endpoint handler functions
EndpointResponse handleWiFiConfig(const EndpointRequest& request);
EndpointResponse handleSystemInfo(const EndpointRequest& request);
EndpointResponse handleWiFiReset(const EndpointRequest& request);
EndpointResponse handleCryptoInfo(const EndpointRequest& request);
EndpointResponse handleNameInfo(const EndpointRequest& request);
EndpointResponse handleWiFiStatus(const EndpointRequest& request);
EndpointResponse handleWiFiScan(const EndpointRequest& request);
EndpointResponse handleCryptoSign(const EndpointRequest& request);
EndpointResponse handleInitializeForm(const EndpointRequest& request);
EndpointResponse handleInitialize(const EndpointRequest& request); 