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
#define PPI_CH_ADDRESS (0)  // PPI channel destined to radio address event debugging
#define PPI_CH_END     (1)  // PPI channel destined to radio end event debugging
#define GPIOTE_CH_OUT  (0)  // GPIOTE channel for RADIO TX

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

static const gpio_t _dbg_pin     = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };
static const gpio_t _dbg_pin_ppi = { .port = DB_LED2_PORT, .pin = DB_LED2_PIN };

//=========================== functions =========================================

static void radio_callback(uint8_t *packet, uint8_t length) {
    printf("(%dB): %s, RSSI: %i\n", length, (char *)packet, db_radio_rssi());
}

void _gpiote_setup(const gpio_t *gpio_e) {
    NRF_GPIOTE->CONFIG[GPIOTE_CH_OUT] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                        (gpio_e->pin << GPIOTE_CONFIG_PSEL_Pos) |
                                        (gpio_e->port << GPIOTE_CONFIG_PORT_Pos) |
                                        (GPIOTE_CONFIG_POLARITY_None << GPIOTE_CONFIG_POLARITY_Pos) |
                                        (GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos);
}

void _ppi_setup(void) {
    // Enable PPI channels
    NRF_PPI->CHENSET = (1 << PPI_CH_ADDRESS) | (1 << PPI_CH_END);

    // Set event and task endpoints for radio address event
    uint32_t radio_events_address   = (uint32_t)&NRF_RADIO->EVENTS_FRAMESTART;
    uint32_t gpiote_tasks_set       = (uint32_t)&NRF_GPIOTE->TASKS_SET[GPIOTE_CH_OUT];
    NRF_PPI->CH[PPI_CH_ADDRESS].EEP = radio_events_address;
    NRF_PPI->CH[PPI_CH_ADDRESS].TEP = gpiote_tasks_set;

    // Set event and task endpoints for radio end event
    uint32_t radio_events_end   = (uint32_t)&NRF_RADIO->EVENTS_PAYLOAD;
    uint32_t gpiote_tasks_clr   = (uint32_t)&NRF_GPIOTE->TASKS_CLR[GPIOTE_CH_OUT];
    NRF_PPI->CH[PPI_CH_END].EEP = radio_events_end;
    NRF_PPI->CH[PPI_CH_END].TEP = gpiote_tasks_clr;
}

//=========================== main ==============================================

/**
 *  @brief The program starts executing here.
 */
int main(void) {

    // Set PPI and GPIOTE
    _gpiote_setup(&_dbg_pin_ppi);
    _ppi_setup();

    // Turn ON the DotBot board regulator
    db_board_init();

    //=========================== Initialize GPIO and timer =====================

    db_gpio_init(&_dbg_pin, DB_GPIO_OUT);
    db_timer_hf_init(0);

    //=========================== Configure Radio ===============================

    db_radio_init(&radio_callback, DOTBOT_GW_RADIO_MODE);
    // db_radio_set_channel(CHANNEL);
    db_radio_set_frequency(50);
    db_radio_rx();

    while (1) {
        db_radio_disable();
        db_gpio_toggle(&_dbg_pin);
        db_radio_tx((uint8_t *)packet_tx, sizeof(packet_tx) / sizeof(packet_tx[0]));
        db_timer_hf_delay_ms(0, DELAY_MS);
    }
}
