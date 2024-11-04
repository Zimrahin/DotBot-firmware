#include <nrf.h>
#include "radio.h"

#define DOTBOT_GW_RADIO_MODE DB_RADIO_IEEE802154_250Kbit
#define DELAY_MS             (200)  // Wait DELAY_MS ms between each send
#define FREQUENCY            (25)   // (2400 + FREQUENCY) MHz
#define TIMER_DEV            (0)
