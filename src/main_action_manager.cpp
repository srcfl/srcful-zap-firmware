#include <esp_system.h>   // For ESP.restart()

#include "main_action_manager.h"
#include "main_actions.h"
#include "wifi/wifi_manager.h"
#include "backend/backend_api_task.h"
#include "ble/ble_handler.h"


#include "zap_log.h" 

static const char* TAG = "main_action_manager"; // Tag for logging


// --- MainActionManager Implementation ---

void MainActionManager::executeReboot() {
    delay(100); // Short delay for potential log flushing
    ESP.restart();
}

void MainActionManager::executeWifiDisconnect(WifiManager& wifiManager) {
    wifiManager.disconnect();
}

void MainActionManager::executeStateUpdate(BackendApiTask& backendApiTask) {
    backendApiTask.triggerStateUpdate();
}

void MainActionManager::executeBleDisconnect(BLEHandler& bleHandler) {
    bleHandler.stop();
}


void MainActionManager::checkAndExecute(const unsigned long currentTime, WifiManager& wifiManager, BackendApiTask& backendApiTask, BLEHandler& bleHandler) {


    // Loop through all defined actions
    for (size_t i = 0; i < MainActions::numActions; ++i) {
        // Use non-volatile local copies for checks
        // Need to read volatile vars inside the loop before checking 'requested'
        if (MainActions::actionStates[i].requested) {
            const bool requested = MainActions::actionStates[i].requested; // Re-read after check for atomicity (though unlikely needed here)
            const unsigned long triggerTime = MainActions::actionStates[i].triggerTime;
            const MainActions::Type actionType = MainActions::actionStates[i].type;


            if (triggerTime <= currentTime) {
               
                MainActions::actionStates[i].requested = false;
                MainActions::actionStates[i].triggerTime = 0; // Clear time info

                // Execute the action based on its type using a switch statement
                switch (actionType) {
                    case MainActions::Type::REBOOT:
                        executeReboot();
                        break; // Technically unreachable, but good practice

                    case MainActions::Type::WIFI_DISCONNECT:
                        executeWifiDisconnect(wifiManager);
                        break;
                    case MainActions::Type::SEND_STATE:
                        executeStateUpdate(backendApiTask);
                        break;
                    case MainActions::Type::BLE_DISCONNECT:
                        executeBleDisconnect(bleHandler);
                        break;

                    // Add cases for other actions here

                    case MainActions::Type::NONE:
                    default:
                        LOG_E(TAG, "Unknown or NONE action type %d encountered during execution.", static_cast<int>(actionType));
                        break;
                }
            }
        }
    }
}
