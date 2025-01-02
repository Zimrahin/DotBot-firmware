#include <nrf.h>
#include "radio.h"

static const uint8_t packet_tx[] = {
    0xF9, 0x6D, 0x0C, 0x88, 0x54, 0xAB, 0x11, 0xB2,  //
    0xAA, 0xDC, 0xB2, 0xE5, 0xE7, 0x24, 0x66, 0xC9,  //
    0x21, 0x8D, 0x07, 0x2C, 0x7C, 0xA5, 0x42, 0x7D,  //
    0x9C, 0xC5, 0xFE, 0xA3, 0x3F, 0x29, 0xB5, 0xB8,  //
    0x52, 0xEF, 0x22, 0xEB, 0x12, 0x39, 0x92, 0xD6,  //
    0xEB, 0xA3, 0xBA, 0x58, 0x14, 0x33, 0x76, 0x7C,  //
    0x86, 0x40, 0xCC, 0x8B, 0xF4, 0x5F, 0xCA, 0x82,  //
    0x16, 0x26, 0x2B, 0x8E, 0x5A, 0x24, 0x52, 0xA6,  //
    0x36, 0xF9, 0x24, 0x97, 0x88, 0x0D, 0x18, 0xFB,  //
    0x96, 0xA0, 0x74, 0xDE, 0x54, 0xE1, 0x7C, 0xB2,  //
    0xCD, 0x2C, 0xE7, 0x9F, 0xFF, 0x24, 0xD3, 0xEC,  //
    0x11, 0x09, 0x1E, 0x6B, 0xE3, 0x51, 0x95, 0x39,  //
    0x9A, 0xC6, 0x75, 0xF7, 0xA8, 0xC6, 0xB7, 0x2E,  //
    0x53, 0x55, 0x01, 0xD6, 0xD5, 0xCD, 0x72, 0x7F,  //
    0x0A, 0x62, 0x47, 0xFA, 0xB0, 0x9F, 0x3C, 0x89,  //
};  // 120 Bytes long

typedef struct {
    db_radio_mode_t radio_mode;              // DB_RADIO_BLE_1MBit, DB_RADIO_IEEE802154_250Kbit
    uint8_t         frequency;               // (2400 + frequency) MHz
    uint8_t         tx_power;                // RADIO_TXPOWER_TXPOWER_XdBm: X = {PosxdBm, 0dBm, NegxdBm}
    uint32_t        delay_us;                // Wait delay_us before sending
    uint32_t        tone_blocker_us;         // (0) Normal operation, (else) Use a sinusoidal blocker for the specified amount of us
    uint8_t         increase_id;             // (0) Don't increase (Blocker), (1) Increase (Main transmitter)
    uint8_t         packet_size;             // Amount of bytes of packet_tx to send
    uint8_t         increase_packet_offset;  // (0) Don't increase (always same packet), (1) Increase
} radio_config_t;

static const radio_config_t configs[] = {
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_Neg20dBm, 0, 0, 1, 80, 0 },
};
