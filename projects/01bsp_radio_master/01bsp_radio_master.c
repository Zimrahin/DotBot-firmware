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
#include <nrf52840_bitfields.h>
#include <nrf52840.h>

// Include BSP packages
#include "board.h"
#include "board_config.h"
#include "gpio.h"
#include "timer_hf.h"
#include "clock.h"

#define PPI_CH_MASTER_CLOCK  (0)
#define PPI_CH_CONFIG_TOGGLE (1)
#define PPI_CH_TASK_COUNT    (2)

#define GPIOTE_CH_OUT_MASTER (0)  // GPIOTE channel for master clock synthesis
#define GPIOTE_CH_OUT_CONFIG (1)  // GPIOTE channel for toggling the config pin

static const gpio_t _pin_out_square       = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };
static const gpio_t _pin_out_config_state = { .port = DB_LED3_PORT, .pin = DB_LED3_PIN };

//=========================== functions =========================================

void _gpiote_setup(const gpio_t *gpio_out, uint8_t gpiote_channel) {
    NRF_GPIOTE->CONFIG[gpiote_channel] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                         (gpio_out->pin << GPIOTE_CONFIG_PSEL_Pos) |
                                         (gpio_out->port << GPIOTE_CONFIG_PORT_Pos) |
                                         (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                                         (GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos);
}

void _ppi_setup(void) {
    // Enable PPI channels
    NRF_PPI->CHENSET = (1 << PPI_CH_MASTER_CLOCK) |
                       (1 << PPI_CH_CONFIG_TOGGLE) |
                       (1 << PPI_CH_TASK_COUNT);

    // Define GPIOTE tasks
    uint32_t gpiote_tasks_toggle_master = (uint32_t)&NRF_GPIOTE->TASKS_OUT[GPIOTE_CH_OUT_MASTER];
    uint32_t gpiote_tasks_toggle_config = (uint32_t)&NRF_GPIOTE->TASKS_OUT[GPIOTE_CH_OUT_CONFIG];

    // Master clock toggling PPI channel
    NRF_PPI->CH[PPI_CH_MASTER_CLOCK].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[0];
    NRF_PPI->CH[PPI_CH_MASTER_CLOCK].TEP = gpiote_tasks_toggle_master;

    // Config pin toggling PPI channel (triggered by TIMER2 match)
    NRF_PPI->CH[PPI_CH_CONFIG_TOGGLE].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[0];
    NRF_PPI->CH[PPI_CH_CONFIG_TOGGLE].TEP = gpiote_tasks_toggle_config;

    // Link master clock toggles to TIMER2 count
    NRF_PPI->CH[PPI_CH_TASK_COUNT].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[0];
    NRF_PPI->CH[PPI_CH_TASK_COUNT].TEP = (uint32_t)&NRF_TIMER2->TASKS_COUNT;
}

void _hf_timer1_init(uint32_t ms) {
    db_hfclk_init();  // Start the high-frequency clock

    // Timer 1: Controls master clock toggles
    NRF_TIMER1->MODE        = TIMER_MODE_MODE_Timer;
    NRF_TIMER1->TASKS_CLEAR = TIMER_TASKS_CLEAR_TASKS_CLEAR_Trigger;
    NRF_TIMER1->BITMODE     = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER1->PRESCALER   = 4;          // 1 MHz clock
    NRF_TIMER1->CC[0]       = 1000 * ms;  // Master clock period

    // Shortcuts: Clear timer at CC[0]
    NRF_TIMER1->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;

    NRF_TIMER1->TASKS_START = TIMER_TASKS_START_TASKS_START_Trigger;
}

void _hf_timer2_init(uint32_t count) {
    // Timer 2: Counts master clock toggles
    NRF_TIMER2->MODE        = TIMER_MODE_MODE_Counter;
    NRF_TIMER2->TASKS_CLEAR = TIMER_TASKS_CLEAR_TASKS_CLEAR_Trigger;
    NRF_TIMER2->BITMODE     = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER2->CC[0]       = count;

    // Shortcuts: Reset counter at CC[0]
    NRF_TIMER2->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;

    NRF_TIMER2->TASKS_START = TIMER_TASKS_START_TASKS_START_Trigger;
}

//=========================== main ==============================================
int main(void) {
    // Initialise TIMER1 for master clock
    _hf_timer1_init(DELAY_ms);

    // Initialise TIMER2 as a counter for toggles
    _hf_timer2_init(MAX_TX_PER_CONFIG);

    // Set up GPIOTE channels
    _gpiote_setup(&_pin_out_square, GPIOTE_CH_OUT_MASTER);
    _gpiote_setup(&_pin_out_config_state, GPIOTE_CH_OUT_CONFIG);

    // Set up PPI channels
    _ppi_setup();

    while (1) {
        __WFI();  // Wait for interruption
    }
}
