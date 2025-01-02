#include <nrf.h>
#include "radio.h"

typedef struct {
    db_radio_mode_t radio_mode;  // DB_RADIO_BLE_1MBit, DB_RADIO_IEEE802154_250Kbit
    uint8_t         frequency;   // (2400 + frequency) MHz
} radio_config_t;

static const radio_config_t configs[] = {
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
    { DB_RADIO_IEEE802154_250Kbit, 25 },
};
