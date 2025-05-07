#include "state_handler.h"
#include "crypto.h"
#include "config.h"
#include "json_light/json_light.h"
#include "firmware_version.h"
#include <time.h>
#include "zap_log.h"
#include "backend/graphql.h" // For GQL functions

// Define TAG for logging
static const char* TAG = "state_handler";

// Define default state update interval (moved from BackendApiTask)
#define DEFAULT_STATE_UPDATE_INTERVAL (5 * 60 * 1000) // 5 minutes

StateHandler::StateHandler()
    : wifiManagerInstance(nullptr), lastUpdateTime(0),
      stateUpdateInterval(DEFAULT_STATE_UPDATE_INTERVAL), initialUpdateDone(false) {
}

void StateHandler::begin(WifiManager* wifiManager) {
    this->wifiManagerInstance = wifiManager;
    // Force immediate state update by setting lastUpdateTime to a value that will
    // trigger an immediate update in the loop, and interval to 0.
    lastUpdateTime = 0;
    stateUpdateInterval = 0; // Trigger immediate update on first connection
    initialUpdateDone = false;
}

void StateHandler::loop() {
    if (!wifiManagerInstance || !wifiManagerInstance->isConnected()) {
        // LOG_D(TAG, "WiFi not connected, skipping state update loop.");
        return;
    }

    unsigned long currentTime = millis();

    if (isTimeForStateUpdate(currentTime)) {
        if (!initialUpdateDone) {
            stateUpdateInterval = DEFAULT_STATE_UPDATE_INTERVAL;
            initialUpdateDone = true;
        }
        lastUpdateTime = currentTime;
        sendStateUpdate();
    }
}

bool StateHandler::isTimeForStateUpdate(unsigned long currentTime) const {
    return (currentTime - lastUpdateTime >= stateUpdateInterval);
}

void StateHandler::sendStateUpdate() {
    if (!wifiManagerInstance || !wifiManagerInstance->isConnected()) {
        LOG_W(TAG, "WiFi not connected, cannot send state update.");
        return;
    }

    LOG_I(TAG, "Preparing state update");

    // Use JsonBuilder to create JWT header
    JsonBuilder headerBuilder;
    headerBuilder.beginObject()
        .add("alg", "ES256")
        .add("typ", "JWT")
        .add("device", crypto_getId().c_str())
        .add("subKey", "state");
    zap::Str header = headerBuilder.end();

    // Get current epoch time in milliseconds
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t epochTimeMs = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;

    // Use JsonBuilder to create JWT payload
    JsonBuilder payloadBuilder;
    payloadBuilder.beginObject()
        .beginObject("status")
            .add("uptime", millis())
            .add("version", FIRMWARE_VERSION_STRING)
        .endObject()
        .beginObject("network")
            .beginObject("wifi")
                .add("connected", wifiManagerInstance->isConnected() ? wifiManagerInstance->getConfiguredSSID().c_str() : "")
                .addArray("ssids", wifiManagerInstance->getLastScanResults())
            .endObject()
            .beginObject("address")
                .add("ip", wifiManagerInstance->isConnected() ? wifiManagerInstance->getLocalIP().c_str() : "")
                .add("port", 80)
                .add("wlan0_mac", wifiManagerInstance->getMacAddress().c_str())
                .beginObject("interfaces")
                    .add("wlan0", wifiManagerInstance->isConnected() ? wifiManagerInstance->getLocalIP().c_str() : "")
                .endObject()
            .endObject()
        .endObject()
        .add("timestamp", epochTimeMs);
    zap::Str payload = payloadBuilder.end();

    // External signature key (from config.h or a secure storage)
    extern const char* PRIVATE_KEY_HEX;

    // Sign and generate JWT
    zap::Str jwt = crypto_create_jwt(header.c_str(), payload.c_str(), PRIVATE_KEY_HEX);

    if (jwt.length() == 0) {
        LOG_E(TAG, "Failed to create JWT");
        return;
    }

    LOG_I(TAG, "JWT created successfully");

    // Send the JWT using GraphQL
    GQL::BoolResponse response = GQL::setConfiguration(jwt);

    // Handle the response
    if (response.isSuccess() && response.data) {
        LOG_I(TAG, "State update sent successfully");
    } else {
        // Handle different error cases
        switch (response.status) {
            case GQL::Status::NETWORK_ERROR:
                LOG_E(TAG, "Network error sending state update");
                break;
            case GQL::Status::GQL_ERROR:
                LOG_E(TAG, "GraphQL error in state update");
                break;
            case GQL::Status::OPERATION_FAILED:
                LOG_E(TAG, "Server rejected state update");
                break;
            default:
                LOG_E(TAG, "Failed to send state update: %s", response.error.c_str());
                break;
        }

        // Retry sooner on failure (after 1 minute instead of default)
        // Ensure stateUpdateInterval is not already very short to avoid busy loop
        if (stateUpdateInterval > 60000) { // Check if current interval is more than 1 min
             lastUpdateTime = millis() - (stateUpdateInterval - 60000); // effectively sets next update in 1 min
        } else {
            // if interval is already short, just use current time to retry at that short interval
            lastUpdateTime = millis(); 
        }
    }
}

void StateHandler::triggerStateUpdate() {
    // we need to set both to 0 to trigger immediate update because of unsigned variables
    lastUpdateTime = 0;
    stateUpdateInterval = 0;
    initialUpdateDone = false; // Ensure default interval is set after this triggered update
    LOG_I(TAG, "Triggering immediate state update via StateHandler");
}

void StateHandler::setInterval(uint32_t interval) {
    stateUpdateInterval = interval;
    // If a new interval is set, we might want to reset initialUpdateDone 
    // if the intention is to override the "first update" logic.
    // For now, assume setInterval is for ongoing adjustments.
    // If interval is 0, it implies an immediate update is desired next.
    if (interval == 0) {
        lastUpdateTime = 0; // Ensure next check of isTimeForStateUpdate passes
        initialUpdateDone = false;
    }
}
