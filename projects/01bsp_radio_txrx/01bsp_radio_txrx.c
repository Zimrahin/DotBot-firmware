/**
 * @file
 * @ingroup samples_bsp
 *
 * @brief This is a short example of how to interface with the onboard Radio in the DotBot board.
 *
 * @author Said Alvarado-Marin <said-alexander.alvarado-marin@inria.fr>
 * @author Diego Badillo-San-Juan <diego.badillo-san-juan@inria.fr>
 *
 * @copyright Inria, 2022-2024
 *
 */
#include <stdio.h>
#include <nrf.h>
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
    0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10,  //
    0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E, 0x20,  //
    0x22, 0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E, 0x30,  //
    0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x3E, 0x40,  //
    0x42, 0x44, 0x46, 0x48, 0x4A, 0x4C, 0x4E, 0x50,  //
    0x52, 0x54, 0x56, 0x58, 0x5A, 0x5C, 0x5E, 0x60,  //
    0x62, 0x64, 0x66, 0x68, 0x6A, 0x6C, 0x6E, 0x70,  //
    0x72, 0x74, 0x76, 0x78, 0x7A, 0x7C, 0x7E, 0x80,  //
    0x82, 0x84, 0x86, 0x88, 0x8A, 0x8C, 0x8E, 0x90,  //
    0x92, 0x94, 0x96, 0x98, 0x9A, 0x9C, 0x9E, 0xA0,  //
    0xA2, 0xA4, 0xA6, 0xA8, 0xAA, 0xAC, 0xAE, 0xB0,  //
    0xB2, 0xB4, 0xB6, 0xB8, 0xBA, 0xBC, 0xBE, 0xC0,  //
    0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE, 0xD0,  //
    0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE, 0xE0,  //
    0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE, 0xF0,  //
};

static const gpio_t _dbg_pin = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };

//=========================== functions =========================================

static void radio_callback(uint8_t *packet, uint8_t length) {
    (void)packet;
    (void)length;
    db_gpio_toggle(&_dbg_pin);
    // printf("(%dB): %s, RSSI: %i\n", length, (char *)packet, db_radio_rssi());
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

    db_radio_init(&radio_callback, DOTBOT_GW_RADIO_MODE);
    db_radio_set_frequency(FREQUENCY);
    // db_radio_rx();

    while (1) {
        db_radio_disable();
        db_radio_tx((uint8_t *)packet_tx, sizeof(packet_tx) / sizeof(packet_tx[0]));
        db_timer_hf_delay_ms(0, DELAY_MS);
    }
}
