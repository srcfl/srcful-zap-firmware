#include "p1data.h"
#include <string.h>
#include <Arduino.h>


void P1Data::setDeviceId(const char *szDeviceId) {
    // Use DEVICE_ID_LEN instead of BUFFER_SIZE
    strncpy(this->szDeviceId, szDeviceId, DEVICE_ID_LEN - 1);
    this->szDeviceId[DEVICE_ID_LEN - 1] = '\0'; // Ensure null termination
}

void P1Data::setTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    timestamp = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;

}