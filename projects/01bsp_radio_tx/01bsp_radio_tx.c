/**
 * @file
 * @ingroup samples_bsp
 *
 * @brief This is a short example of how to interface with the onboard Radio in the DotBot board.
 *
 * @author Diego Badillo-San-Juan <diego.badillo-san-juan@inria.fr>
 *
 * @copyright Inria, 2024
 *
 */
#include <nrf.h>
#include <stdio.h>
#include <stdlib.h>
// Include BSP packages
#include "board.h"
#include "board_config.h"
#include "gpio.h"
#include "radio.h"
#include "timer_hf.h"

//=========================== defines ===========================================

// See conf.h

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
    0x31, 0x32, 0x33, 0x34, 0x00,                    // 1234 + null
};

static const gpio_t _dbg_pin = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };

//=========================== functions =========================================

static void radio_callback(uint8_t *packet, uint8_t length) {
    (void)packet;
    (void)length;
}

//=========================== main ==============================================

int main(void) {

    // Turn ON the DotBot board regulator
    db_board_init();

    //=========================== Initialize GPIO and timer =====================

    db_gpio_init(&_dbg_pin, DB_GPIO_OUT);
    db_timer_hf_init(0);

    //=========================== Configure Radio ===============================

    db_radio_init(&radio_callback, DOTBOT_GW_RADIO_MODE);
    db_radio_set_frequency(FREQUENCY);

    while (1) {
        db_radio_disable();
        db_radio_tx((uint8_t *)packet_tx, sizeof(packet_tx) / sizeof(packet_tx[0]));
        db_gpio_toggle(&_dbg_pin);
        db_timer_hf_delay_ms(0, DELAY_MS);
    }
}