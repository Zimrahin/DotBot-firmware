/**
 * @file
 * @ingroup samples_bsp
 * @author Diego Badillo-San-Juan <diego.badillo-san-juan@inria.fr>
 * @brief This is a short example of how to interface with the onboard Radio in the DotBot board.
 *
 * @copyright Inria, 2022-2024
 *
 */
#include <nrf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
// Include BSP packages
#include "board.h"
#include "board_config.h"
#include "gpio.h"
#include "radio_ieee802154.h"
#include "timer.h"

//=========================== defines ===========================================

#define DELAY_MS           (500)            // Delay between sending a packet if received correctly
#define TIMEOUT_MS         (20 * DELAY_MS)  // Timeout for waiting on a matching packet
#define IEEE802154_CHANNEL (15)             // Set the frequency to 2425 MHz (channel 15)

//=========================== variables =========================================

static const uint8_t packet_tx[] = {
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,  // ABCDEFGH
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,  // ABCDEFGH
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,  // ABCDEFGH
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x00         // ABCDEFG
};

static const gpio_t _dbg_pin = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };

static bool should_send_flag = false;

//=========================== functions =========================================

// Check if the received payload matches the expected payload
static bool _payload_matches(const uint8_t *packet, uint8_t length) {
    if (length != sizeof(packet_tx)) {
        return false;
    }
    return (memcmp(packet, packet_tx, sizeof(packet_tx)) == 0);
}

// Callback function when a packet is received
static void _radio_rx_callback(uint8_t *packet, uint8_t length) {
    db_gpio_toggle(&_dbg_pin);  // Toggle the LED

    if (_payload_matches(packet, length)) {
        printf("Matching packet received (%dB): %s, RSSI: %i\n", length, (char *)packet, db_radio_ieee802154_rssi());
        should_send_flag = true;
    } else {
        printf("Non-matching packet received (%dB): %s, RSSI: %i\n", length, (char *)packet, db_radio_ieee802154_rssi());
    }
}

// Every TIMEOUT_MS send packet anyway
static void _timeout_callback(void) {
    db_radio_ieee802154_disable();
    db_radio_ieee802154_tx((uint8_t *)packet_tx, sizeof(packet_tx) / sizeof(packet_tx[0]));
}

// Every DELAY_MS send packet only if a matching packet was received
static void _tx_if_rx_callback(void) {
    if (should_send_flag) {
        db_radio_ieee802154_disable();
        db_radio_ieee802154_tx((uint8_t *)packet_tx, sizeof(packet_tx) / sizeof(packet_tx[0]));
        should_send_flag = false;
    }
}

//=========================== main ==============================================

int main(void) {

    // Turn ON the DotBot board regulator
    db_board_init();

    //=========================== Initialize GPIO and timer =====================

    db_gpio_init(&_dbg_pin, DB_GPIO_OUT);
    db_timer_init(0);  // Init the timers

    //=========================== Configure Radio ===============================

    db_radio_ieee802154_init(&_radio_rx_callback);
    db_radio_ieee802154_set_channel(IEEE802154_CHANNEL);
    db_radio_ieee802154_rx();

    // Set timer callbacks
    db_timer_set_periodic_ms(0, 0, TIMEOUT_MS, &_timeout_callback);
    db_timer_set_periodic_ms(0, 1, DELAY_MS, &_tx_if_rx_callback);

    while (1) {
        __WFI();  // Wait for interrupt
    }
}
