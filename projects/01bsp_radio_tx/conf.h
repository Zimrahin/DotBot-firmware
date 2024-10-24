#include <nrf.h>
#include "radio.h"

#define DOTBOT_GW_RADIO_MODE DB_RADIO_BLE_1MBit
#define DELAY_MS             (1000)  // Wait DELAY_MSms between each send
#define FREQUENCY            (50)    // (2400 + FREQUENCY) MHz
#define TIMER_DEV            (0)
