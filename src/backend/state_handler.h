#ifndef STATE_HANDLER_H
#define STATE_HANDLER_H

#include <stdint.h>
#include "wifi/wifi_manager.h" // For WifiManager type

// Forward declaration if preferred and possible, but full include for simplicity here
// class WifiManager; 

class StateHandler {
public:
    StateHandler();
    void begin(WifiManager* wifiManager);
    void loop();
    void triggerStateUpdate();
    void setInterval(uint32_t interval);

private:
    bool isTimeForStateUpdate(unsigned long currentTime) const;
    void sendStateUpdate();

    WifiManager* wifiManagerInstance;
    unsigned long lastUpdateTime;
    uint32_t stateUpdateInterval;
    bool initialUpdateDone; // To manage setting default interval after first run
};

#endif // STATE_HANDLER_H
