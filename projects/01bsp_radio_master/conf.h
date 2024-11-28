#include <nrf.h>
#include "radio.h"

#define MAX_TX_PER_CONFIG (10000)  // Number of master clock toggles until toggling config state
#define DELAY_ms          (5)      // Wait DELAY_ms between each transmission
#define CHANGE_STATE_ms   (2000)   // Time given to wait for transmitters and receiver to change state
