#include "main_actions.h"
#include <Arduino.h>    // For millis()

MainActions::State MainActions::actionStates[] = {
    {Type::REBOOT, false, 0, 0},
    {Type::WIFI_DISCONNECT, false, 0, 0},
    {Type::SEND_STATE, false, 0, 0},
    // Add other actions here
};

const unsigned char MainActions::numActions = (unsigned char)(sizeof(actionStates) / sizeof(actionStates[0]));

void MainActions::triggerAction(Type type, unsigned long delayMs) {
    if (type == Type::NONE) return;

    for (size_t i = 0; i < numActions; ++i) {
        if (actionStates[i].type == type) {
            // Overwrite previous request for the same action type
            actionStates[i].delayMs = delayMs;
            actionStates[i].requestTime = millis();
            actionStates[i].requested = true; // Set flag last
            // Serial.printf("MainActions: Action %d requested with delay %lu ms.\n", static_cast<int>(type), delayMs);
            return; // Action found and triggered
        }
    }
    Serial.printf("MainActions: Warning - Action type %d not found in actionStates array.\n", static_cast<int>(type));
}