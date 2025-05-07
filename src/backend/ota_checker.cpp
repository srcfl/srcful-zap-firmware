#include "ota_checker.h"
#include "../firmware_version.h" 
#include "../ota_handler.h"    
#include "../json_light/json_light.h" // Ensure JsonParser is included

// Define TAG for logging
static const char* TAG = "OtaChecker";

// Define default OTA check interval
#define DEFAULT_OTA_CHECK_INTERVAL (30 * 60 * 1000) // 30 minutes
#define OTA_CHECK_BASE_URL "https://sleipner.srcful.dev/api/devices/sn/"
#define OTA_CHECK_ENDPOINT "/firmwares/latest"

OtaChecker::OtaChecker()
    : wifiManagerInstance(nullptr), lastOtaCheckTime(0),
      otaCheckInterval(DEFAULT_OTA_CHECK_INTERVAL), initialCheckDone(false) {
}

void OtaChecker::begin(WifiManager* wifiManager) {
    this->wifiManagerInstance = wifiManager;
    lastOtaCheckTime = 0;
    otaCheckInterval = 0; 
    initialCheckDone = false;
    httpClient.setTimeout(15000); 
}

void OtaChecker::loop() {
    if (!wifiManagerInstance || !wifiManagerInstance->isConnected()) {
        return;
    }

    unsigned long currentTime = millis();

    if (isTimeForOtaCheck(currentTime)) {
        if (!initialCheckDone) {
            otaCheckInterval = DEFAULT_OTA_CHECK_INTERVAL;
            initialCheckDone = true;
        }
        lastOtaCheckTime = currentTime;
        checkForUpdate();
    }
}

bool OtaChecker::isTimeForOtaCheck(unsigned long currentTime) const {
    return (currentTime - lastOtaCheckTime >= otaCheckInterval);
}

void OtaChecker::checkForUpdate() {
    if (!wifiManagerInstance || !wifiManagerInstance->isConnected()) {
        LOG_W(TAG, "WiFi not connected, cannot check for OTA update.");
        return;
    }

    zap::Str deviceId = crypto_getId();
    if (deviceId.isEmpty()) {
        LOG_E(TAG, "Device ID is empty, cannot check for OTA update.");
        return;
    }

    zap::Str url = zap::Str(OTA_CHECK_BASE_URL) + deviceId + zap::Str(OTA_CHECK_ENDPOINT);
    LOG_I(TAG, "Checking for OTA update at: %s", url.c_str());

    httpClient.begin(url.c_str()); 
    int httpCode = httpClient.GET();

    if (httpCode > 0) {
        LOG_I(TAG, "HTTP GET successful, code: %d", httpCode);
        if (httpCode == HTTP_CODE_OK) {
            // Use zap::Str for payload, converting from Arduino String
            zap::Str payload(httpClient.getString().c_str()); 
            LOG_D(TAG, "Response payload: %s", payload.c_str());
            parseFirmwareResponse(payload);
        } else {
            LOG_W(TAG, "HTTP GET failed with code: %d", httpCode);
        }
    } else {
        LOG_E(TAG, "HTTP GET failed, error: %s", httpClient.errorToString(httpCode).c_str());
    }
    httpClient.end();
}

void OtaChecker::parseFirmwareResponse(const zap::Str& payload) { 
    LOG_I(TAG, "Parsing firmware response...");
    JsonParser parser(payload.c_str()); // Use JsonParser

    zap::Str newVersion;
    if (parser.getString("version", newVersion)) { // Use JsonParser::getString
        LOG_I(TAG, "Latest firmware version available: %s", newVersion.c_str());
        LOG_I(TAG, "Current firmware version: %s", FIRMWARE_VERSION_STRING);

        if (newVersion != FIRMWARE_VERSION_STRING) {
            LOG_I(TAG, "A new firmware version is available: %s", newVersion.c_str());
            
            JsonParser binaryParser(""); // Initialize with an empty string
            if (parser.getObject("binary", binaryParser)) { // Use JsonParser::getObject
                zap::Str downloadUrl;
                zap::Str hash;

                if (binaryParser.getString("downloadUrl", downloadUrl) && 
                    binaryParser.getString("hash", hash)) {
                    LOG_I(TAG, "Download URL: %s", downloadUrl.c_str());
                    LOG_I(TAG, "Hash: %s", hash.c_str());
                    // Example: OtaHandler::getInstance()->startOta(downloadUrl, hash);
                } else {
                    LOG_W(TAG, "Could not parse download URL or hash from binary object.");
                }
            } else {
                LOG_W(TAG, "Binary object not found or not an object in JSON response.");
            }
        } else {
            LOG_I(TAG, "Firmware is up to date.");
        }
    } else {
        LOG_W(TAG, "Could not find 'version' in JSON response or it's not a string.");
    }
}

void OtaChecker::triggerOtaCheck() {
    lastOtaCheckTime = 0;
    otaCheckInterval = 0; 
    initialCheckDone = false; 
    LOG_I(TAG, "Triggering immediate OTA check via OtaChecker");
}

void OtaChecker::setInterval(uint32_t interval) {
    otaCheckInterval = interval;
    if (interval == 0) {
        lastOtaCheckTime = 0; 
        initialCheckDone = false;
    }
}
