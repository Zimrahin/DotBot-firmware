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

#define PPI_CH_MASTER_CLOCK   (0)
#define PPI_CH_CONFIG_TOGGLE  (1)
#define PPI_CH_DELAY_AFTER_TX (2)
#define PPI_CH_EVENT_CHANGE   (3)

#define GPIOTE_CH_OUT_MASTER (0)  // GPIOTE channel for master clock synthesis
#define GPIOTE_CH_OUT_CONFIG (1)  // GPIOTE channel for toggling the config pin

static const gpio_t _pin_out_square       = { .port = 1, .pin = 11 };
static const gpio_t _pin_out_config_state = { .port = 1, .pin = 10 };

//=========================== functions =========================================

void _gpiote_setup(const gpio_t *gpio_out, uint8_t gpiote_channel, uint32_t out_init) {
    NRF_GPIOTE->CONFIG[gpiote_channel] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                         (gpio_out->pin << GPIOTE_CONFIG_PSEL_Pos) |
                                         (gpio_out->port << GPIOTE_CONFIG_PORT_Pos) |
                                         (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
                                         (out_init << GPIOTE_CONFIG_OUTINIT_Pos);
}

void _ppi_setup(void) {
    // Enable PPI channels
    NRF_PPI->CHENSET = (1 << PPI_CH_MASTER_CLOCK) |
                       (1 << PPI_CH_CONFIG_TOGGLE) |
                       (1 << PPI_CH_DELAY_AFTER_TX) |
                       (1 << PPI_CH_EVENT_CHANGE);

    // Define GPIOTE tasks
    uint32_t gpiote_tasks_toggle_master = (uint32_t)&NRF_GPIOTE->TASKS_OUT[GPIOTE_CH_OUT_MASTER];
    uint32_t gpiote_tasks_set_config    = (uint32_t)&NRF_GPIOTE->TASKS_SET[GPIOTE_CH_OUT_CONFIG];  // Set to (1)
    uint32_t gpiote_tasks_clr_config    = (uint32_t)&NRF_GPIOTE->TASKS_CLR[GPIOTE_CH_OUT_CONFIG];  // Set to (0)

    // Master clock toggling PPI channel
    NRF_PPI->CH[PPI_CH_MASTER_CLOCK].EEP   = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[0];
    NRF_PPI->CH[PPI_CH_MASTER_CLOCK].TEP   = gpiote_tasks_toggle_master;
    NRF_PPI->FORK[PPI_CH_MASTER_CLOCK].TEP = (uint32_t)&NRF_TIMER2->TASKS_COUNT;  // Count transmissions

    // After a number of transmissions, start TIMER 0 and stop transmission to wait for state change
    NRF_PPI->CH[PPI_CH_CONFIG_TOGGLE].EEP   = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[0];
    NRF_PPI->CH[PPI_CH_CONFIG_TOGGLE].TEP   = (uint32_t)&NRF_TIMER0->TASKS_START;
    NRF_PPI->FORK[PPI_CH_CONFIG_TOGGLE].TEP = (uint32_t)&NRF_TIMER1->TASKS_STOP;  // Stop master tx clock

    // Set event and task endpoints to change state
    NRF_PPI->CH[PPI_CH_DELAY_AFTER_TX].EEP = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[0];
    NRF_PPI->CH[PPI_CH_DELAY_AFTER_TX].TEP = gpiote_tasks_set_config;

    NRF_PPI->CH[PPI_CH_EVENT_CHANGE].EEP   = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[1];
    NRF_PPI->CH[PPI_CH_EVENT_CHANGE].TEP   = gpiote_tasks_clr_config;
    NRF_PPI->FORK[PPI_CH_EVENT_CHANGE].TEP = (uint32_t)&NRF_TIMER1->TASKS_START;  // Re-start transmission after changing state
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
    db_hfclk_init();  // Start the high frequency clock if not already on

    // Timer 2: Counts master clock toggles
    NRF_TIMER2->MODE        = TIMER_MODE_MODE_Counter;
    NRF_TIMER2->TASKS_CLEAR = TIMER_TASKS_CLEAR_TASKS_CLEAR_Trigger;
    NRF_TIMER2->BITMODE     = TIMER_BITMODE_BITMODE_32Bit;
    NRF_TIMER2->CC[0]       = count;

    // Shortcuts: Reset counter at CC[0]
    NRF_TIMER2->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;

    NRF_TIMER2->TASKS_START = TIMER_TASKS_START_TASKS_START_Trigger;
}

void _hf_timer0_init(uint32_t msCC0, uint32_t msCC1) {
    db_hfclk_init();  // Start the high frequency clock if not already on

    NRF_TIMER0->MODE        = (TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos);
    NRF_TIMER0->TASKS_CLEAR = (TIMER_TASKS_CLEAR_TASKS_CLEAR_Trigger << TIMER_TASKS_CLEAR_TASKS_CLEAR_Pos);  // Clear timer
    NRF_TIMER0->BITMODE     = (TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos);                    // 16 bits should be enough (65 ms in total)
    NRF_TIMER0->PRESCALER   = (4 << TIMER_PRESCALER_PRESCALER_Pos);                                          // 16/2⁴= 1MHz
    NRF_TIMER0->CC[0]       = 1000 * msCC0;                                                                  // Set the number of 1MHz ticks to wait for enabling EVENTS_COMPARE[0]
    NRF_TIMER0->CC[1]       = 1000 * msCC1 + 1000 * msCC0;

    // Disable and clear the timer immediately after EVENTS_COMPARE[1] event
    NRF_TIMER0->SHORTS = (TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos) |
                         (TIMER_SHORTS_COMPARE1_STOP_Enabled << TIMER_SHORTS_COMPARE1_STOP_Pos);
}

//=========================== main ==============================================

int main(void) {
    // Set GPIO pins as output
    db_gpio_init(&_pin_out_square, DB_GPIO_OUT);
    db_gpio_init(&_pin_out_config_state, DB_GPIO_OUT);

    // Set up GPIOTE channels
    _gpiote_setup(&_pin_out_square, GPIOTE_CH_OUT_MASTER, GPIOTE_CONFIG_OUTINIT_Low);
    _gpiote_setup(&_pin_out_config_state, GPIOTE_CH_OUT_CONFIG, GPIOTE_CONFIG_OUTINIT_Low);

    // Set up PPI channels
    _ppi_setup();

    // Initialise timers
    _hf_timer1_init(DELAY_ms);                   // Master TX clock
    _hf_timer2_init(MAX_TX_PER_CONFIG);          // TX counter
    _hf_timer0_init(DELAY_ms, CHANGE_STATE_ms);  // Time to change state

    while (1) {
        __WFI();  // Wait for interruption
    }
}
