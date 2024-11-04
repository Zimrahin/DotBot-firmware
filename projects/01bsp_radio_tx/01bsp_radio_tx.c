/**
 * @file
 * @ingroup samples_bsp
 *
 * @brief This example shows how to transmit packets over the radio
 *
 * @author Diego Badillo-San-Juan <diego.badillo-san-juan@inria.fr>
 *
 * @copyright Inria, 2024
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
#include "radio.h"
#include "timer_hf.h"
#include "ppi_functions.h"

//=========================== defines ===========================================

#define MAX_PAYLOAD_SIZE (120)  // Maximum message size

typedef struct __attribute__((packed)) {
    uint32_t msg_id;                     // Message ID (starts at 0 and increments by 1 for each message)
    uint8_t  message[MAX_PAYLOAD_SIZE];  // Actual message
} _radio_pdu_t;

//=========================== variables =========================================

static const uint8_t packet_tx[] = {
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,  //
    0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E,  //
    0x20, 0x22, 0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E,  //
    0x30, 0x32, 0x34, 0x36, 0x38, 0x3A, 0x3C, 0x3E,  //
    0x40, 0x42, 0x44, 0x46, 0x48, 0x4A, 0x4C, 0x4E,  //
    0x50, 0x52, 0x54, 0x56, 0x58, 0x5A, 0x5C, 0x5E,  //
    0x60, 0x62, 0x64, 0x66, 0x68, 0x6A, 0x6C, 0x6E,  //
    0x70, 0x72, 0x74, 0x76, 0x78, 0x7A, 0x7C, 0x7E,  //
    0x80, 0x82, 0x84, 0x86, 0x88, 0x8A, 0x8C, 0x8E,  //
    0x90, 0x92, 0x94, 0x96, 0x98, 0x9A, 0x9C, 0x9E,  //
    0xA0, 0xA2, 0xA4, 0xA6, 0xA8, 0xAA, 0xAC, 0xAE,  //
    0xB0, 0xB2, 0xB4, 0xB6, 0xB8, 0xBA, 0xBC, 0xBE,  //
    0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,  //
    0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,  //
    0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,  //
};  // 120 Bytes long

static const gpio_t _dbg_pin     = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };
static const gpio_t _dbg_pin_ppi = { .port = DB_LED2_PORT, .pin = DB_LED2_PIN };

static bool         _tx_flag   = false;
static _radio_pdu_t _radio_pdu = { 0 };

//=========================== callbacks =========================================

static void _tx_callback(void) {
    _tx_flag = true;
}

//=========================== functions =========================================

//=========================== main ==============================================

int main(void) {
    // Set PPI and GPIOTE
    _gpiote_setup(&_dbg_pin_ppi);
    _ppi_setup(DOTBOT_GW_RADIO_MODE);

    // Turn ON the DotBot board regulator
    db_board_init();

    // Initialize message to _radio_pdu_t struct
    memcpy(_radio_pdu.message, packet_tx, sizeof(packet_tx));

    //=========================== Initialize GPIO and timer =====================

    db_gpio_init(&_dbg_pin, DB_GPIO_OUT);
    db_timer_hf_init(0);

    //=========================== Configure Radio ===============================

    db_radio_init(NULL, DOTBOT_GW_RADIO_MODE);
    db_radio_set_frequency(FREQUENCY);

    // Set timer callbacks
    db_timer_hf_set_periodic_us(TIMER_DEV, 0, 1000 * DELAY_MS, &_tx_callback);

    while (1) {
        if (_tx_flag) {
            // Send packet
            db_gpio_toggle(&_dbg_pin);  // Toggle LED
            db_radio_disable();
            db_radio_tx((uint8_t *)&_radio_pdu, sizeof(_radio_pdu));

            _radio_pdu.msg_id += 1;
            _tx_flag = false;
        }
        __WFI();  // Wait for interruption
    }
}
