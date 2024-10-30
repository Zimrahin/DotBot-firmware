#include <nrf.h>
#include "radio.h"

#define DOTBOT_GW_RADIO_MODE DB_RADIO_BLE_1MBit
#define DELAY_US             (100)  // Wait DELAY_US before sending
#define FREQUENCY            (5)    // (2400 + FREQUENCY) MHz
#define TIMER_DEV            (0)
