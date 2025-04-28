#include "p1data.h"
#include <string.h>


void P1Data::setDeviceId(const char *szDeviceId) {
    // Use DEVICE_ID_LEN instead of BUFFER_SIZE
    strncpy(this->szDeviceId, szDeviceId, DEVICE_ID_LEN - 1);
    this->szDeviceId[DEVICE_ID_LEN - 1] = '\0'; // Ensure null termination
}