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
#include "timer_hf.h"
#include "clock.h"

#define PPI_CH_MASTER_CLOCK  (7)
#define GPIOTE_CH_OUT_MASTER (3)  // GPIOTE channel for master clock synthesis

//=========================== variables =========================================

static const gpio_t _pin_square_out = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };

//=========================== functions =========================================

void _gpiote_setup(const gpio_t *gpio_out) {
    // Configure output GPIO for master clock
    NRF_GPIOTE->CONFIG[GPIOTE_CH_OUT_MASTER] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                               (gpio_out->pin << GPIOTE_CONFIG_PSEL_Pos) |
                                               (gpio_out->port << GPIOTE_CONFIG_PORT_Pos) |
                                               (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                                               (GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos);
}

void _ppi_setup(void) {
    // Enable PPI channels
    NRF_PPI->CHENSET = (1 << PPI_CH_MASTER_CLOCK);

    // Define GPIOTE tasks for toggling master clock
    uint32_t gpiote_tasks_toggle = (uint32_t)&NRF_GPIOTE->TASKS_OUT[GPIOTE_CH_OUT_MASTER];  // Toggle

    // Set event and task endpoints to start timer
    NRF_PPI->CH[PPI_CH_MASTER_CLOCK].EEP   = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[1];
    NRF_PPI->CH[PPI_CH_MASTER_CLOCK].TEP   = (uint32_t)&NRF_TIMER1->TASKS_START;
    NRF_PPI->FORK[PPI_CH_MASTER_CLOCK].TEP = gpiote_tasks_toggle;  // (1)
}

void _hf_timer_init(uint32_t ms) {
    db_hfclk_init();  // Start the high frequency clock if not already on

    NRF_TIMER1->MODE        = (TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos);
    NRF_TIMER1->TASKS_CLEAR = (TIMER_TASKS_CLEAR_TASKS_CLEAR_Trigger << TIMER_TASKS_CLEAR_TASKS_CLEAR_Pos);  // Clear timer
    NRF_TIMER1->BITMODE     = (TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos);                    // 32 bits
    NRF_TIMER1->PRESCALER   = (4 << TIMER_PRESCALER_PRESCALER_Pos);                                          // 16/2⁴= 1MHz
    NRF_TIMER1->CC[1]       = 1000 * ms;                                                                     // Set the master clock period

    // Disable and clear the timer immediately after EVENTS_COMPARE[0] event
    NRF_TIMER1->SHORTS = (TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos) |
                         (TIMER_SHORTS_COMPARE1_STOP_Enabled << TIMER_SHORTS_COMPARE1_STOP_Pos);

    NRF_TIMER1->TASKS_START = (TIMER_TASKS_START_TASKS_START_Trigger << TIMER_TASKS_START_TASKS_START_Pos);
}

//=========================== main ==============================================
int main(void) {
    // Initialise the TIMER1 at channel 1
    _hf_timer_init(DELAY_ms);
    _gpiote_setup(&_pin_square_out);
    _ppi_setup();

    while (1) {
        __WFI();  // Wait for interruption
    }
}
