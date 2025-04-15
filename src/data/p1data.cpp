#include "p1data.h"
#include <string.h>


void P1Data::setDeviceId(const char *szDeviceId) {

    strncpy(this->szDeviceId, szDeviceId, BUFFER_SIZE);
}