#pragma once

class WifiManager; // Forward declaration
class BackendApiTask; // Forward declaration

class MainActionManager {
    public:
        // Public constructor
        MainActionManager() {};

        /**
         * @brief Checks if any requested action's delay (set via main_actions.h)
         *        has passed and executes it.
         *        This should be called repeatedly in the main loop.
         */
        void checkAndExecute(WifiManager& wifiManager, BackendApiTask& backendApiTask);

    private:
        void executeReboot();
        void executeWifiDisconnect(WifiManager& wifiManager);
        void executeStateUpdate(BackendApiTask& backendApiTask);

        
};

