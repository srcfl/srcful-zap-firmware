#include "Arduino.h"

SerialClass Serial;

const long millis_default_return_value = 17;
unsigned long millis_return_value = millis_default_return_value;



unsigned long millis() {
    return millis_return_value;
}