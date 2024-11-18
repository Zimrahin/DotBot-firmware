#include <nrf.h>
#include "radio.h"

static const uint8_t packet_tx[] = {
    0xEE, 0xEC, 0xEA, 0xE8, 0xE6, 0xE4, 0xE2, 0xE0,  //
    0xDE, 0xDC, 0xDA, 0xD8, 0xD6, 0xD4, 0xD2, 0xD0,  //
    0xCE, 0xCC, 0xCA, 0xC8, 0xC6, 0xC4, 0xC2, 0xC0,  //
    0xBE, 0xBC, 0xBA, 0xB8, 0xB6, 0xB4, 0xB2, 0xB0,  //
    0xAE, 0xAC, 0xAA, 0xA8, 0xA6, 0xA4, 0xA2, 0xA0,  //
    0x9E, 0x9C, 0x9A, 0x98, 0x96, 0x94, 0x92, 0x90,  //
    0x8E, 0x8C, 0x8A, 0x88, 0x86, 0x84, 0x82, 0x80,  //
    0x7E, 0x7C, 0x7A, 0x78, 0x76, 0x74, 0x72, 0x70,  //
    0x6E, 0x6C, 0x6A, 0x68, 0x66, 0x64, 0x62, 0x60,  //
    0x5E, 0x5C, 0x5A, 0x58, 0x56, 0x54, 0x52, 0x50,  //
    0x4E, 0x4C, 0x4A, 0x48, 0x46, 0x44, 0x42, 0x40,  //
    0x3E, 0x3C, 0x3A, 0x38, 0x36, 0x34, 0x32, 0x30,  //
    0x2E, 0x2C, 0x2A, 0x28, 0x26, 0x24, 0x22, 0x20,  //
    0x1E, 0x1C, 0x1A, 0x18, 0x16, 0x14, 0x12, 0x10,  //
    0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02, 0x00,  //
};  // 120 Bytes long

typedef struct {
    db_radio_mode_t radio_mode;       // DB_RADIO_BLE_1MBit, DB_RADIO_IEEE802154_250Kbit
    uint8_t         frequency;        // (2400 + frequency) MHz
    uint8_t         tx_power;         // RADIO_TXPOWER_TXPOWER_XdBm: X = {PosxdBm, 0dBm, NegxdBm}
    uint32_t        delay_us;         // Wait delay_us before sending
    uint32_t        tone_blocker_us;  // (0) Normal operation, (else) Use a sinusoidal blocker for the specified amount of us
    uint8_t         increase_id;      // (0) Don't increase (Blocker), (1) Increase (Main transmitter)
    uint8_t         packet_size;      // Amount of bytes of packet_tx to send
    const uint8_t  *packet_tx;        // Pointer to a predefined constant packet
} radio_config_t;

static const radio_config_t configs[] = {
    { DB_RADIO_IEEE802154_250Kbit, 25, RADIO_TXPOWER_TXPOWER_0dBm, 255, 0, 0, 8, packet_tx },
};
