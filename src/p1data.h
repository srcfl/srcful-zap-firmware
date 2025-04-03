#ifndef P1DATA_H
#define P1DATA_H

#include <Arduino.h>
#include "crypto.h"
#include <time.h>

// Function to create the P1 meter JWT
String createP1JWT(const char* privateKey, const String& deviceId);

// Function to get current timestamp in milliseconds
unsigned long long getCurrentTimestamp();

// Function to initialize NTP
void initNTP();

#endif 