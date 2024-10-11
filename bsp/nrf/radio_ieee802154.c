/**
 * @file
 * @ingroup bsp_radio_ieee802154
 *
 * @brief  nRF52833-specific definition of the "radio" bsp module for IEEE 802.15.4.
 *
 * @author Said Alvarado-Marin <said-alexander.alvarado-marin@inria.fr>
 * @author Raphael Simoes <raphael.simoes@inria.fr>
 * @author Diego Badillo-San-Juan <diego.badillo-san-juan@inria.fr>
 *
 * @copyright Inria, 2022-2024
 */
#include <nrf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "clock.h"
#include "radio_ieee802154.h"

//=========================== defines ==========================================

#if defined(NRF5340_XXAA) && defined(NRF_NETWORK)
#define NRF_RADIO NRF_RADIO_NS
#endif

#define PAYLOAD_MAX_LENGTH (127UL)  ///< Total usable payload for IEEE 802.15.4 is 127 octets (PSDU)
#if defined(NRF5340_XXAA) && defined(NRF_NETWORK)
#define RADIO_INTERRUPT_PRIORITY 2
#else
#define RADIO_INTERRUPT_PRIORITY 1
#endif

// #define RADIO_TIFS          640U  ///< Interframe spacing in us
#define RADIO_SHORTS_COMMON (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |                 \
                                (RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos) |             \
                                (RADIO_SHORTS_ADDRESS_RSSISTART_Enabled << RADIO_SHORTS_ADDRESS_RSSISTART_Pos) | \
                                (RADIO_SHORTS_DISABLED_RSSISTOP_Enabled << RADIO_SHORTS_DISABLED_RSSISTOP_Pos)
#define RADIO_INTERRUPTS (RADIO_INTENSET_DISABLED_Enabled << RADIO_INTENSET_DISABLED_Pos) | \
                             (RADIO_INTENSET_ADDRESS_Enabled << RADIO_INTENSET_ADDRESS_Pos)
#define RADIO_STATE_IDLE 0x00
#define RADIO_STATE_RX   0x01
#define RADIO_STATE_TX   0x02
#define RADIO_STATE_BUSY 0x04

typedef struct __attribute__((packed)) {
    uint8_t header;                       ///< PDU header (depends on the type of PDU - advertising physical channel or Data physical channel)
    uint8_t length;                       ///< Length of the payload + MIC (if any)
    uint8_t payload[PAYLOAD_MAX_LENGTH];  ///< Payload + MIC (if any)
} ieee802154_radio_pdu_t;

typedef struct {
    ieee802154_radio_pdu_t pdu;       ///< Variable that stores the radio PDU (protocol data unit) that arrives and the radio packets that are about to be sent.
    radio_ieee802154_cb_t  callback;  ///< Function pointer, stores the callback to use in the RADIO_Irq handler.
    uint8_t                state;     ///< Internal state of the radio
} radio_vars_t;

//=========================== variables ========================================

static radio_vars_t radio_vars = { 0 };

//========================== prototypes ========================================

static void _radio_enable(void);

//=========================== public ===========================================

void db_radio_ieee802154_init(radio_ieee802154_cb_t callback) {
    // Reset radio to its initial values
    NRF_RADIO->POWER = (RADIO_POWER_POWER_Disabled << RADIO_POWER_POWER_Pos);
    NRF_RADIO->POWER = (RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos);

    // General configuration of the radio
    NRF_RADIO->MODE = (RADIO_MODE_MODE_Ieee802154_250Kbit << RADIO_MODE_MODE_Pos);  // Configure IEEE 802.15.4 mode

    NRF_RADIO->TXPOWER = (RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos);  // Set transmission power to 0dBm

    // Packet configuration register 0
    NRF_RADIO->PCNF0 = (0 << RADIO_PCNF0_S1LEN_Pos) |                         // S1 field length in bits
                       (0 << RADIO_PCNF0_S0LEN_Pos) |                         // S0 field length in bytes
                       (8 << RADIO_PCNF0_LFLEN_Pos) |                         // 8-bit length field
                       (RADIO_PCNF0_PLEN_32bitZero << RADIO_PCNF0_PLEN_Pos);  // 4 bytes that are all zero for IEEE 802.15.4

    // Packet configuration register 1
    NRF_RADIO->PCNF1 = (4UL << RADIO_PCNF1_BALEN_Pos) |                           // 4-byte base address (24 bits)
                       (PAYLOAD_MAX_LENGTH << RADIO_PCNF1_MAXLEN_Pos) |           // Max payload of 127 bytes
                       (0 << RADIO_PCNF1_STATLEN_Pos) |                           // 0 bytes added to payload length
                       (RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos) |    // Little-endian format
                       (RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos);  // Enable data whitening

    // Address configuration
    NRF_RADIO->BASE0       = DEFAULT_NETWORK_ADDRESS;                                           // Configuring the on-air radio address
    NRF_RADIO->TXADDRESS   = 0UL;                                                               // Only send using logical address 0
    NRF_RADIO->RXADDRESSES = (RADIO_RXADDRESSES_ADDR0_Enabled << RADIO_RXADDRESSES_ADDR0_Pos);  // Only receive from logical address 0

    // TIFS (time interframe spacing): time between two consecutive packets
    // Enable automatic TIFS adjustment based on frame size
    // Note: The END to START shortcut should not be used with Ieee802154_250Kbit modes.
    // Rather the PHYEND to START shortcut.
    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk |  // Automatically start transmission when ready
                        RADIO_SHORTS_PHYEND_START_Msk;  // Automatically handle TIFS after END event

    // CRC Config
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos) |                  // 16-bit (2 bytes) CRC
                        (RADIO_CRCCNF_SKIPADDR_Ieee802154 << RADIO_CRCCNF_SKIPADDR_Pos);  // CRCCNF = 0x202 for IEEE 802.15.4
    NRF_RADIO->CRCINIT = 0;                                                               // The start value used by IEEE 802.15.4 is zero
    NRF_RADIO->CRCPOLY = 0x11021;                                                         // CRC polynomial: x16 + x12 + x5 + 1

    // Configure pointer to PDU for EasyDMA
    NRF_RADIO->PACKETPTR = (uint32_t)&radio_vars.pdu;

    // Assign the callback function that will be called when a radio packet is received.
    radio_vars.callback = callback;
    radio_vars.state    = RADIO_STATE_IDLE;

    // Configure the external High-frequency Clock (needed for correct operation)
    db_hfclk_init();

    // Configure the Interruptions
    NVIC_SetPriority(RADIO_IRQn, RADIO_INTERRUPT_PRIORITY);  // Set priority for Radio interrupts to 1

    // Clear all radio interruptions
    NRF_RADIO->INTENCLR = 0xffffffff;
    NVIC_EnableIRQ(RADIO_IRQn);
}

void db_radio_ieee802154_set_frequency(uint8_t freq) {
    NRF_RADIO->FREQUENCY = freq << RADIO_FREQUENCY_FREQUENCY_Pos;
}

void db_radio_ieee802154_set_channel(uint8_t channel) {
    // The IEEE 802.15.4 standard defines 16 channels [11 - 26] of 5 MHz each in the 2450 MHz frequency band.
    assert(channel >= 11 && channel <= 26 && "Channel value must be between 11 and 26 for IEEE 802.15.4");
    uint8_t freq = 5 * (channel - 10);  // Frequency offset in MHz from 2400 MHz
    db_radio_ieee802154_set_frequency(freq);
}

void db_radio_ieee802154_set_network_address(uint32_t addr) {
    NRF_RADIO->BASE0 = addr;
}

void db_radio_ieee802154_tx(const uint8_t *tx_buffer, uint8_t length) {
    radio_vars.pdu.length = length;
    memcpy(radio_vars.pdu.payload, tx_buffer, length);

    NRF_RADIO->SHORTS = RADIO_SHORTS_COMMON | (RADIO_SHORTS_DISABLED_RXEN_Enabled << RADIO_SHORTS_DISABLED_RXEN_Pos);

    if (radio_vars.state == RADIO_STATE_IDLE) {
        // Enable the Radio to send the packet
        NRF_RADIO->EVENTS_DISABLED = 0;  // We must use EVENT_DISABLED, if we use EVENT_END. the interrupts will be enabled in the time between the END event and the Disable event, triggering an undesired interrupt
        NRF_RADIO->TASKS_TXEN      = RADIO_TASKS_TXEN_TASKS_TXEN_Trigger << RADIO_TASKS_TXEN_TASKS_TXEN_Pos;
        // Wait for transmission to end and the radio to be disabled
        while (NRF_RADIO->EVENTS_DISABLED == 0) {}

        // We re-enable interrupts AFTER the packet is sent, to avoid triggering an EVENT_ADDRESS and EVENT_DISABLED interrupt with the outgoing packet
        // We also clear both flags to avoid insta-triggering an interrupt as soon as we assert INTENSET
        NRF_RADIO->EVENTS_ADDRESS  = 0;
        NRF_RADIO->EVENTS_DISABLED = 0;
        _radio_enable();
    }
    radio_vars.state = RADIO_STATE_RX;
}

void db_radio_ieee802154_rx(void) {
    NRF_RADIO->SHORTS   = RADIO_SHORTS_COMMON | (RADIO_SHORTS_DISABLED_RXEN_Enabled << RADIO_SHORTS_DISABLED_RXEN_Pos);
    NRF_RADIO->INTENSET = RADIO_INTERRUPTS;

    if (radio_vars.state == RADIO_STATE_IDLE) {
        _radio_enable();
        NRF_RADIO->TASKS_RXEN = RADIO_TASKS_RXEN_TASKS_RXEN_Trigger;
    }
    radio_vars.state = RADIO_STATE_RX;
}

void db_radio_ieee802154_disable(void) {
    NRF_RADIO->INTENCLR        = RADIO_INTERRUPTS;
    NRF_RADIO->SHORTS          = 0;
    NRF_RADIO->EVENTS_TXREADY  = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE   = RADIO_TASKS_DISABLE_TASKS_DISABLE_Trigger << RADIO_TASKS_DISABLE_TASKS_DISABLE_Pos;
    while (NRF_RADIO->EVENTS_DISABLED == 0) {}
    radio_vars.state = RADIO_STATE_IDLE;
}

int8_t db_radio_ieee802154_rssi(void) {
    return (uint8_t)NRF_RADIO->RSSISAMPLE * -1;
}

//=========================== private ==========================================

static void _radio_enable(void) {
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->INTENSET        = RADIO_INTERRUPTS;
}

//=========================== interrupt handlers ===============================

/**
 * @brief Interruption handler for the Radio.
 *
 * This function will be called each time a radio packet is received.
 * it will clear the interrupt, copy the last received packet
 * and called the user-defined callback to process the package.
 *
 */
void RADIO_IRQHandler(void) {

    if (NRF_RADIO->EVENTS_ADDRESS) {
        NRF_RADIO->EVENTS_ADDRESS = 0;
        radio_vars.state |= RADIO_STATE_BUSY;
    }

    if (NRF_RADIO->EVENTS_DISABLED) {
        // Clear the Interrupt flag
        NRF_RADIO->EVENTS_DISABLED = 0;

        if (radio_vars.state == (RADIO_STATE_BUSY | RADIO_STATE_RX)) {
            if (NRF_RADIO->CRCSTATUS != RADIO_CRCSTATUS_CRCSTATUS_CRCOk) {
                puts("Invalid CRC");
            } else if (radio_vars.callback) {
                radio_vars.callback(radio_vars.pdu.payload, radio_vars.pdu.length);
            }
            radio_vars.state = RADIO_STATE_RX;
        } else {  // TX
            radio_vars.state = RADIO_STATE_RX;
        }
    }
}
