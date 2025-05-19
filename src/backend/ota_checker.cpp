#include "ota_checker.h"
#include "../firmware_version.h" 
#include "ota/ota_handler.h"    
#include "../json_light/json_light.h" // Ensure JsonParser is included
#include "ota/ota_handler.h"

static constexpr LogTag TAG = LogTag("ota_checker", ZLOG_LEVEL_DEBUG);

// Define default OTA check interval
#define DEFAULT_OTA_CHECK_INTERVAL (30 * 60 * 1000) // 30 minutes
// #define DEFAULT_OTA_CHECK_INTERVAL (3 * 60 * 1000) // 3 minutes

#define OTA_CHECK_BASE_URL "https://sleipner.srcful.dev/api/devices/sn/"
#define OTA_CHECK_ENDPOINT "/firmwares/latest"

OtaChecker::OtaChecker()
    : lastOtaCheckTime(0),
      otaCheckInterval(DEFAULT_OTA_CHECK_INTERVAL), initialCheckDone(false) {
}

void OtaChecker::begin() {
    lastOtaCheckTime = 0;
    otaCheckInterval = 1 * 60 * 1000; // we wait for one minute before the first check so that all other tasks can do their thing
    initialCheckDone = false;
    httpClient.setTimeout(15000); 
}

void OtaChecker::loop() {
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

    zap::Str deviceId = crypto_getId();
    if (deviceId.isEmpty()) {
        LOG_TE(TAG, "Device ID is empty, cannot check for OTA update.");
        return;
    }

    zap::Str url = zap::Str(OTA_CHECK_BASE_URL) + deviceId + zap::Str(OTA_CHECK_ENDPOINT);
    LOG_TI(TAG, "Checking for OTA update at: %s", url.c_str());

    httpClient.begin(url.c_str()); 
    int httpCode = httpClient.GET();

    if (httpCode > 0) {
        LOG_TI(TAG, "HTTP GET successful, code: %d", httpCode);
        if (httpCode == HTTP_CODE_OK) {
            zap::Str payload(httpClient.getString().c_str()); 
            LOG_TD(TAG, "Response payload: %s", payload.c_str());
            parseFirmwareResponse(payload);
        } else {
            LOG_TW(TAG, "HTTP GET failed with code: %d", httpCode);
        }
    } else {
        LOG_TE(TAG, "HTTP GET failed, error: %s", httpClient.errorToString(httpCode).c_str());
    }
    httpClient.end();
}

void OtaChecker::parseFirmwareResponse(const zap::Str& payload) { 
    LOG_TI(TAG, "Parsing firmware response...");
    JsonParser parser(payload.c_str()); // Use JsonParser

    zap::Str newVersion;
    if (parser.getString("version", newVersion)) { // Use JsonParser::getString
        LOG_TD(TAG, "Latest firmware version available: %s", newVersion.c_str());
        LOG_TD(TAG, "Current firmware version: %s", FIRMWARE_VERSION_STRING);

        if (newVersion != FIRMWARE_VERSION_STRING) {
            LOG_TI(TAG, "A different firmware version found");
            LOG_TD(TAG, "Found firmware version: %s", newVersion.c_str());
            
            JsonParser binaryParser(""); // Initialize with an empty string
            if (parser.getObject("binary", binaryParser)) { // Use JsonParser::getObject
                zap::Str downloadUrl;
                zap::Str hash;

                if (binaryParser.getString("downloadUrl", downloadUrl) && 
                    binaryParser.getString("hash", hash)) {
                    LOG_TD(TAG, "Download URL: %s", downloadUrl.c_str());
                    LOG_TD(TAG, "Hash: %s", hash.c_str());
                    // Example: OtaHandler::getInstance()->startOta(downloadUrl, hash);
                    extern OTAHandler g_otaHandler; // Assuming OtaHandler is defined elsewhere
                    g_otaHandler.requestOTAUpdate(downloadUrl, newVersion);
                } else {
                    LOG_TW(TAG, "Could not parse download URL or hash from binary object.");
                }
            } else {
                LOG_TW(TAG, "Binary object not found or not an object in JSON response.");
            }
        } else {
            LOG_TD(TAG, "Firmware is up to date.");
        }
    } else {
        LOG_TW(TAG, "Could not find 'version' in JSON response or it's not a string.");
    }
}

void OtaChecker::triggerOtaCheck() {
    lastOtaCheckTime = 0;
    otaCheckInterval = 0; 
    initialCheckDone = false; 
    LOG_TD(TAG, "Triggering immediate OTA check via OtaChecker");
}
