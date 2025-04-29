#pragma once

class WifiManager; // Forward declaration

class MainActionManager {
    public:
        // Public constructor
        MainActionManager(WifiManager& wifiManager) : wifiManager(wifiManager) {};

        /**
         * @brief Checks if any requested action's delay (set via main_actions.h)
         *        has passed and executes it.
         *        This should be called repeatedly in the main loop.
         */
        void checkAndExecute();

    private:
        void executeReboot();
        void executeWifiDisconnect();

        WifiManager& wifiManager; // Reference to WifiManager instance
};

