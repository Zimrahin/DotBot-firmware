#include <nrf.h>
#include "radio.h"

#define TX_POWER  RADIO_TXPOWER_TXPOWER_0dBm  // PosXdBm, 0dBm, NegXdBm
#define DELAY_us  (250)                       // Wait DELAY_us before sending
#define SINE_us   (645)                       // Duration of sinusoidal wave
#define FREQUENCY (25)                        // (2400 + FREQUENCY) MHz
