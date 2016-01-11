#pragma once
#include <stdint.h>

// Returns time (in milliseconds) since some time point.
// Use it for intervals calculation.
uint64_t hGetTimeMS();

// System independent sleep. Input - time period, in milliseconds.
void hSleep( unsigned int ms );
