/**
 * @file
 * @ingroup samples_bsp
 *
 * @brief This example shows how to transmit a sine wave over the radio
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

// Include BSP packages
#include "board.h"
#include "board_config.h"
#include "gpio.h"
#include "radio.h"
#include "timer_hf.h"
#include "clock.h"

//=========================== defines ===========================================

#define PPI_CH_TXENABLE_SYNCH (0)  // PPI channel destined to set TXENABLE in synch with master clock
#define PPI_CH_READY          (1)  // PPI channel destined to radio TX_READY event debugging
#define PPI_CH_DISABLE        (2)  // PPI channel destined to radio DISABLE task debugging
#define PPI_CH_DISABLED       (3)  // PPI channel destined to radio DISABLED event debugging
#define PPI_CH_TIMER_START    (4)  // PPI channel destined to start the timer

#define GPIOTE_CH_OUT (1)  // GPIOTE channel for RADIO TX visualization
#define GPIOTE_CH_IN  (2)  // GPIOTE channel for master clock TX synchronisation

//=========================== variables =========================================

static const gpio_t _pin_square_in        = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };  // Square signal from master
static const gpio_t _pin_radio_events_out = { .port = DB_LED2_PORT, .pin = DB_LED2_PIN };  // Show radio events in digital analyser

//=========================== functions =========================================

void _gpiote_setup(const gpio_t *gpio_in, const gpio_t *gpio_out) {
    // Configure input GPIO pin for enabling a synched transmission to a master clock
    NRF_GPIOTE->CONFIG[GPIOTE_CH_IN] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
                                       (gpio_in->pin << GPIOTE_CONFIG_PSEL_Pos) |
                                       (gpio_in->port << GPIOTE_CONFIG_PORT_Pos) |
                                       (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);

    // Configure output GPIO pins for RADIO events visualisation on the digital analyser
    NRF_GPIOTE->CONFIG[GPIOTE_CH_OUT] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                        (gpio_out->pin << GPIOTE_CONFIG_PSEL_Pos) |
                                        (gpio_out->port << GPIOTE_CONFIG_PORT_Pos) |
                                        (GPIOTE_CONFIG_POLARITY_None << GPIOTE_CONFIG_POLARITY_Pos) |
                                        (GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos);
}

void _ppi_setup(void) {
    // Enable PPI channels
    NRF_PPI->CHENSET = (1 << PPI_CH_TXENABLE_SYNCH) |
                       (1 << PPI_CH_READY) |
                       (1 << PPI_CH_DISABLE) |
                       (1 << PPI_CH_DISABLED) |
                       (1 << PPI_CH_TIMER_START);

    // Define GPIOTE tasks for transmission visualisation in digital analyser
    uint32_t gpiote_tasks_set = (uint32_t)&NRF_GPIOTE->TASKS_SET[GPIOTE_CH_OUT];  // Set to (1)
    uint32_t gpiote_tasks_clr = (uint32_t)&NRF_GPIOTE->TASKS_CLR[GPIOTE_CH_OUT];  // Set to (0)

    // Set event and task endpoints to start timer
    NRF_PPI->CH[PPI_CH_TIMER_START].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[GPIOTE_CH_IN];
    NRF_PPI->CH[PPI_CH_TIMER_START].TEP = (uint32_t)&NRF_TIMER0->TASKS_START;

#if DELAY_us
    // Start transmission after a delay
    NRF_PPI->CH[PPI_CH_TXENABLE_SYNCH].EEP = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[0];
#else
    // Start transmission immediately
    NRF_PPI->CH[PPI_CH_TXENABLE_SYNCH].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[GPIOTE_CH_IN];
#endif
    NRF_PPI->CH[PPI_CH_TXENABLE_SYNCH].TEP   = (uint32_t)&NRF_RADIO->TASKS_TXEN;
    NRF_PPI->FORK[PPI_CH_TXENABLE_SYNCH].TEP = gpiote_tasks_set;  // (1)

    // Set event and task endpoints for radio TX_READY event (0)
    NRF_PPI->CH[PPI_CH_READY].EEP = (uint32_t)&NRF_RADIO->EVENTS_TXREADY;
    NRF_PPI->CH[PPI_CH_READY].TEP = gpiote_tasks_clr;  // (0)

    // Set event and task endpoints for radio DISABLE task (after timer) (1)
    NRF_PPI->CH[PPI_CH_DISABLE].EEP   = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[1];
    NRF_PPI->CH[PPI_CH_DISABLE].TEP   = (uint32_t)&NRF_RADIO->TASKS_DISABLE;
    NRF_PPI->FORK[PPI_CH_DISABLE].TEP = gpiote_tasks_set;  // (1)

    // Set event and task endpoints for radio DISABLED event (0)
    NRF_PPI->CH[PPI_CH_DISABLED].EEP = (uint32_t)&NRF_RADIO->EVENTS_DISABLED;
    NRF_PPI->CH[PPI_CH_DISABLED].TEP = gpiote_tasks_clr;  // (0)
}

void _hf_timer_init(uint32_t delay_us, uint32_t sine_us) {
    db_hfclk_init();                    // Start the high frequency clock if not already on
    uint32_t txru_us             = 40;  // 40.5 us of ramp up measured with the analyser
    uint32_t disable_disabled_us = 6;   // 6.375 us between DISABLE and DISABLED measured with the analyser

    NRF_TIMER0->MODE        = (TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos);
    NRF_TIMER0->TASKS_CLEAR = (TIMER_TASKS_CLEAR_TASKS_CLEAR_Trigger << TIMER_TASKS_CLEAR_TASKS_CLEAR_Pos);  // Clear timer
    NRF_TIMER0->BITMODE     = (TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos);                    // 16 bits should be enough (65 ms in total)
    NRF_TIMER0->PRESCALER   = (4 << TIMER_PRESCALER_PRESCALER_Pos);                                          // 16/2⁴= 1MHz
    NRF_TIMER0->CC[0]       = delay_us;
    NRF_TIMER0->CC[1]       = delay_us + sine_us + txru_us - disable_disabled_us;

    NRF_TIMER0->SHORTS = (TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos) |
                         (TIMER_SHORTS_COMPARE1_STOP_Enabled << TIMER_SHORTS_COMPARE1_STOP_Pos);
}

//=========================== main ==============================================

int main(void) {
    // Initialise the TIMER0 at channel 0
    _hf_timer_init(DELAY_us, SINE_us);

    // Configure Radio
    db_radio_init(NULL, DB_RADIO_BLE_1MBit);                       // Placeholder, technically not necessary for sending a sine wave
    db_radio_set_frequency(FREQUENCY);                             // Set transmission frequency
    NRF_RADIO->TXPOWER = (TX_POWER << RADIO_TXPOWER_TXPOWER_Pos);  // Set transmission power

    // Set PPI and GPIOTE
    _gpiote_setup(&_pin_square_in, &_pin_radio_events_out);
    _ppi_setup();

    while (1) {
        __WFI();  // Wait for interruption
    }
}
