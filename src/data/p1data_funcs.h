#pragma once
#include <Arduino.h>

#include "p1data.h"

// Function to create the P1 meter JWT with P1Data included
String createP1JWT(const char* privateKey, const String& deviceId, const P1Data& p1data);