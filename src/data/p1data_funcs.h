#pragma once
#include "../zap_str.h"
#include "decoding/p1data.h"

// function to parse the p1 data into a json jwt payload string.
bool createP1JWTPayload(const P1Data& p1data, char* outBuffer, size_t outBufferSize);