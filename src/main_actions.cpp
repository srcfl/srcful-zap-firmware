#include "main_actions.h"
#include <Arduino.h>    // For millis()

#include "zap_log.h"

static constexpr LogTag TAG = LogTag("main_actions", ZLOG_LEVEL_INFO);

MainActions::State MainActions::actionStates[] = {
    {Type::REBOOT, false, 0, 0},
    {Type::WIFI_DISCONNECT, false, 0, 0},
    {Type::SEND_STATE, false, 0, 0},
    {Type::BLE_DISCONNECT, false, 0, 0},
    // Add other actions here
};

const unsigned char MainActions::numActions = (unsigned char)(sizeof(actionStates) / sizeof(actionStates[0]));

void MainActions::triggerAction(Type type, unsigned long delayMs) {
    if (type == Type::NONE) return;

    for (size_t i = 0; i < numActions; ++i) {
        if (actionStates[i].type == type) {
            if (actionStates[i].requested) {
                LOG_TV(TAG, "Action type %d already requested, ignoring new request.\n", static_cast<int>(type));
                return; // Action already requested
            }
            // Overwrite previous request for the same action type
            actionStates[i].requested = true;
            actionStates[i].delayMs = delayMs;
            actionStates[i].requestTime = millis();
            LOG_TV(TAG, "MainActions: Action %d requested with delay %i ms.\n", static_cast<int>(type), delayMs);
            return; // Action found and triggered
        }
    }
    LOG_TW(TAG, "Action type %d not found in actionStates array.\n", static_cast<int>(type));
}