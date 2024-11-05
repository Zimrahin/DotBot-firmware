#include <nrf.h>
#include "radio.h"

#define DOTBOT_GW_RADIO_MODE DB_RADIO_IEEE802154_250Kbit
#define TX_POWER             RADIO_TXPOWER_TXPOWER_0dBm  // PosXdBm, 0dBm, NegXdBm
#define DELAY_US             (100)                       // Wait DELAY_US before sending
#define FREQUENCY            (50)                        // (2400 + FREQUENCY) MHz
#define TIMER_DEV            (0)
