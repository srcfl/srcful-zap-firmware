#include "main_actions.h"
#include <Arduino.h>    // For millis()

#include "zap_log.h"

static constexpr LogTag MAIN_ACTIONS = LogTag("main_actions", ZLOG_LEVEL_INFO);

MainActions::State MainActions::actionStates[] = {
    {Type::REBOOT, false, 0},
    {Type::WIFI_DISCONNECT, false, 0},
    {Type::SEND_STATE, false, 0},
    {Type::BLE_DISCONNECT, false, 0},
    // Add other actions here
};

const unsigned char MainActions::numActions = (unsigned char)(sizeof(actionStates) / sizeof(actionStates[0]));

void MainActions::triggerAction(Type type, unsigned long delayMs) {
    if (type == Type::NONE) return;

    for (size_t i = 0; i < numActions; ++i) {
        if (actionStates[i].type == type) {
            const unsigned long currentTime = millis();
            if (actionStates[i].requested) {

                // check if the new time to execute is shorter than the previous one
                
                
                if (actionStates[i].triggerTime > currentTime + delayMs) {
                    // If the new delay is shorter, update it
                    actionStates[i].triggerTime = currentTime + delayMs;
                    LOG_TV(MAIN_ACTIONS, "Action type %d updated with new delay %i ms.\n", static_cast<int>(type), delayMs);
                    return;
                } 
                // If the same action is already requested with the same delay, ignore the new request
                LOG_TV(MAIN_ACTIONS, "Action type %d already requested, ignoring new request.\n", static_cast<int>(type));
                return; // Action already requested
            }
            // Overwrite previous request for the same action type
            actionStates[i].requested = true;
            actionStates[i].triggerTime = currentTime + delayMs;
            LOG_TV(MAIN_ACTIONS, "MainActions: Action %d requested with delay %i ms.\n", static_cast<int>(type), delayMs);
            return; // Action found and triggered
        }
    }
    LOG_TW(MAIN_ACTIONS, "Action type %d not found in actionStates array.\n", static_cast<int>(type));
}