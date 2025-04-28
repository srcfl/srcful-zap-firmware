#pragma once
#include <Arduino.h>
#include "../zap_str.h"
#include "decoding/p1data.h"

// Function to create the P1 meter JWT with P1Data included
zap::Str createP1JWT(const char* privateKey, const zap::Str& deviceId, const P1Data& p1data);