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
#include "ppi_functions.h"

void _gpiote_setup(const gpio_t *gpio_pin) {
    NRF_GPIOTE->CONFIG[GPIOTE_CH_OUT] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
                                        (gpio_pin->pin << GPIOTE_CONFIG_PSEL_Pos) |
                                        (gpio_pin->port << GPIOTE_CONFIG_PORT_Pos) |
                                        (GPIOTE_CONFIG_POLARITY_None << GPIOTE_CONFIG_POLARITY_Pos) |
                                        (GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos);
}

void _ppi_setup(db_radio_mode_t mode) {
    // Enable PPI channels
    NRF_PPI->CHENSET = (1 << PPI_CH_ADDRESS) | (1 << PPI_CH_END);

    // Set event and task endpoints for radio address event
    uint32_t radio_events_payload_start;
    if (mode == DB_RADIO_IEEE802154_250Kbit) {
        radio_events_payload_start = (uint32_t)&NRF_RADIO->EVENTS_FRAMESTART;
    } else {
        radio_events_payload_start = (uint32_t)&NRF_RADIO->EVENTS_ADDRESS;
    }
    uint32_t gpiote_tasks_set       = (uint32_t)&NRF_GPIOTE->TASKS_SET[GPIOTE_CH_OUT];
    NRF_PPI->CH[PPI_CH_ADDRESS].EEP = radio_events_payload_start;
    NRF_PPI->CH[PPI_CH_ADDRESS].TEP = gpiote_tasks_set;

    // Set event and task endpoints for radio end event
    uint32_t radio_events_payload_end = (uint32_t)&NRF_RADIO->EVENTS_PAYLOAD;
    uint32_t gpiote_tasks_clr         = (uint32_t)&NRF_GPIOTE->TASKS_CLR[GPIOTE_CH_OUT];
    NRF_PPI->CH[PPI_CH_END].EEP       = radio_events_payload_end;
    NRF_PPI->CH[PPI_CH_END].TEP       = gpiote_tasks_clr;
}
