#pragma once

class MainActions {
    public:
        // --- Action Types ---
        enum class Type {
            NONE,
            REBOOT,
            WIFI_DISCONNECT,
            BLE_DISCONNECT,
            SEND_STATE
            // Add other action types here, do not forget to update the actionStates array in the cpp file
        };

        struct State {
                Type type;
                volatile bool requested;
                volatile unsigned long triggerTime;
        };

        // --- Action Trigger Function ---
        static void triggerAction(Type type, unsigned long delayMs);


        // Define the array of action states (without executeFunc)
        static State actionStates[];
        static const unsigned char numActions;

};


