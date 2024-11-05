#include <nrf.h>
#include "radio.h"

#define DOTBOT_GW_RADIO_MODE DB_RADIO_BLE_1MBit
#define TX_POWER             RADIO_TXPOWER_TXPOWER_0dBm  // PosXdBm, 0dBm, NegXdBm
#define DELAY_MS             (400)                       // Wait DELAY_MS ms between each send
#define FREQUENCY            (25)                        // (2400 + FREQUENCY) MHz
#define TIMER_DEV            (0)
