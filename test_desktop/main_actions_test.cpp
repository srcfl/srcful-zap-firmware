#include "../src/main_actions.h"
#include <assert.h>
#include <Arduino.h>

namespace main_actions_test {

    int get_action_ix(MainActions::Type action) {
       for (int i = 0; i < MainActions::numActions; i++) {
            if (MainActions::actionStates[i].type == action) {
                return i;
            }
        }
        return -1;
    }

    void reset_action_states() {
        for (int i = 0; i < MainActions::numActions; i++) {
            MainActions::actionStates[i].requested = false;
            MainActions::actionStates[i].triggerTime = 0;
            
        }
    }

    int test_trigger_action() {
        reset_action_states();

        MainActions::triggerAction(MainActions::Type::REBOOT, 100);
        
        const int action_ix = get_action_ix(MainActions::Type::REBOOT);
        assert(action_ix != -1);
        assert(MainActions::actionStates[action_ix].triggerTime == millis() + 100);
        assert(MainActions::actionStates[action_ix].requested == true);

        return 0;
    }

    int test_retrigger_action() {

        test_trigger_action();
        const int action_ix = get_action_ix(MainActions::Type::REBOOT);

        int original_trigger_time = MainActions::actionStates[action_ix].triggerTime;

        // now some time has passed but we have 11 msec left to trigger
        millis_return_value = original_trigger_time - 11;

        // The original trigger time should now trigger in 50ms
        // if we add a new trigger time of 10ms it should be the new trigger time
        MainActions::triggerAction(MainActions::Type::REBOOT, 10);
        assert(MainActions::actionStates[action_ix].triggerTime == millis() + 10);
        assert(MainActions::actionStates[action_ix].requested == true);


        // now add a new trigger time of 11ms
        // it should be ignored since the original trigger time is 11ms
        MainActions::triggerAction(MainActions::Type::REBOOT, 11);
        assert(MainActions::actionStates[action_ix].triggerTime == millis() + 10);
        assert(MainActions::actionStates[action_ix].requested == true);

        millis_return_value = original_trigger_time;
        return 0;
    }

    int run() {
        
        test_trigger_action();
        test_retrigger_action();
    
        return 0;
    }
}

