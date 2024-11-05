/**
 * @file
 * @ingroup samples_bsp
 *
 * @brief This script makes a square signal for synching two transmitters
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
#include "timer.h"

//=========================== variables =========================================

static const gpio_t _pin_square_out = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };

//=========================== callbacks =========================================

static void _toggle_callback(void) {
    db_gpio_toggle(&_pin_square_out);  // Toggle slave triggering LED
}

//=========================== main ==============================================
int main(void) {
    // Turn ON the DotBot board regulator
    db_board_init();

    // Initialize GPIO and timer
    db_gpio_init(&_pin_square_out, DB_GPIO_OUT);
    db_timer_init(0);

    // Set timer callbacks
    db_timer_set_periodic_ms(TIMER_DEV, 0, DELAY_MS, &_toggle_callback);

    while (1) {
        __WFI();  // Wait for interruption
    }
}
