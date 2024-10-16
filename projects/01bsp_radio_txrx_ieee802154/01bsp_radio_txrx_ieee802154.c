/**
 * @file
 * @ingroup samples_bsp
 * @author Said Alvarado-Marin <said-alexander.alvarado-marin@inria.fr>
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
// Include BSP packages
#include "board.h"
#include "board_config.h"
#include "gpio.h"
#include "radio_ieee802154.h"
#include "timer_hf.h"

//=========================== defines ===========================================

#define DELAY_MS           (100)  // Wait 100ms between each send
#define IEEE802154_CHANNEL (15)   // Set the frequency to 2425 MHz (channel 15)

//=========================== variables =========================================

static const uint8_t packet_tx[] = {
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,  // AAAAAAAA
    0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D,  // --------
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,  // AAAAAAAA
    0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D,  // --------
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,  // AAAAAAAA
    0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D,  // --------
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,  // AAAAAAAA
    0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D,  // --------
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,  // AAAAAAAA
    0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D,  // --------
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,  // AAAAAAAA
    0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D,  // --------
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,  // AAAAAAAA
    0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D, 0x2D,  // --------
    0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,  // AAAAAAAA
    0x31, 0x32, 0x33, 0x34, 0x35,                    // 12345
};

static const gpio_t _dbg_pin = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };

//=========================== functions =========================================

static void _radio_callback(uint8_t *packet, uint8_t length, bool crc) {
    db_gpio_toggle(&_dbg_pin);
    printf("crc %d (%X), (%dB): %s, RSSI: %i\n", crc, NRF_RADIO->RXCRC, length, (char *)packet, db_radio_ieee802154_rssi());
}

//=========================== main ==============================================

/**
 *  @brief The program starts executing here.
 */
int main(void) {

    // Turn ON the DotBot board regulator
    db_board_init();

    //=========================== Initialize GPIO and timer =====================

    db_gpio_init(&_dbg_pin, DB_GPIO_OUT);
    db_timer_hf_init(0);

    //=========================== Configure Radio ===============================

    db_radio_ieee802154_init(&_radio_callback);
    db_radio_ieee802154_set_channel(IEEE802154_CHANNEL);
    db_radio_ieee802154_rx();

    while (1) {
        db_radio_ieee802154_disable();
        db_radio_ieee802154_tx((uint8_t *)packet_tx, sizeof(packet_tx) / sizeof(packet_tx[0]));
        db_timer_hf_delay_ms(0, DELAY_MS);
    }
}
