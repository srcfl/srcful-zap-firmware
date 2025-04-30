#pragma once
#include <Arduino.h>
#include "../zap_str.h"
#include "decoding/p1data.h"

// Function to create the P1 meter JWT with P1Data included
zap::Str createP1JWT(const char* privateKey, const zap::Str& deviceId, const char* szPayload);

// function to parse the p1 data into a json jwt payload string.
bool createP1JWTPayload(const P1Data& p1data, char* outBuffer, size_t outBufferSize);