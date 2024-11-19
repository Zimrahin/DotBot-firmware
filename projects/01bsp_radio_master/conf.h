#include <nrf.h>
#include "radio.h"

#define MAX_TX_PER_CONFIG (2)    // Number of master clock toggles until toggling config state
#define DELAY_ms          (100)  // Wait DELAY_ms between each toggle
