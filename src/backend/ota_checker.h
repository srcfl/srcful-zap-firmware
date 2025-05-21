#ifndef OTA_CHECKER_H
#define OTA_CHECKER_H

#include <stdint.h>
#include "wifi/wifi_manager.h" // For WifiManager type
#include "HTTPClient.h"      // For HTTPClient
#include "json_light/json_light.h" // For JSON parsing
#include "../zap_log.h"      // For logging
#include "../crypto.h"       // For crypto_getId()
#include "../zap_str.h"      // Include zap_str.h

class OtaChecker {
public:
    OtaChecker();
    void begin();
    void loop(const unsigned long currentTime);
    void triggerOtaCheck();

private:
    bool isTimeForOtaCheck(unsigned long currentTime) const;
    void checkForUpdate();
    void parseFirmwareResponse(const zap::Str& payload); // Changed to zap::Str

    HTTPClient httpClient;
    unsigned long lastOtaCheckTime;
    uint32_t otaCheckInterval;
    bool initialCheckDone; 
};

#endif // OTA_CHECKER_H
