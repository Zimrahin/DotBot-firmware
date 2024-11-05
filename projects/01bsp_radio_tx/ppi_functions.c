// #include <nrf.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <stdbool.h>
// #include <string.h>

// // Include BSP packages
// #include "board.h"
// #include "board_config.h"
// #include "gpio.h"
// #include "radio.h"
// #include "ppi_functions.h"

// #define PPI_CH_READY              (0)  // PPI channel destined to radio TX_READY event debugging
// #define PPI_CH_ADDRESS_FRAMESTART (1)  // PPI channel destined to radio ADDRESS or FRAMESTART event debugging
// #define PPI_CH_PAYLOAD            (2)  // PPI channel destined to radio PAYLOAD event debugging
// #define PPI_CH_PHYEND             (3)  // PPI channel destined to radio PHYEND event debugging

// #define GPIOTE_CH_OUT (1)  // GPIOTE channel for RADIO TX

// void _gpiote_setup(const gpio_t *gpio_pin) {
//     NRF_GPIOTE->CONFIG[GPIOTE_CH_OUT] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
//                                         (gpio_pin->pin << GPIOTE_CONFIG_PSEL_Pos) |
//                                         (gpio_pin->port << GPIOTE_CONFIG_PORT_Pos) |
//                                         (GPIOTE_CONFIG_POLARITY_None << GPIOTE_CONFIG_POLARITY_Pos) |
//                                         (GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos);
// }

// void _ppi_setup(db_radio_mode_t mode) {
//     // Enable PPI channels
//     NRF_PPI->CHENSET = (1 << PPI_CH_READY) | (1 << PPI_CH_ADDRESS_FRAMESTART) |
//                        (1 << PPI_CH_PAYLOAD) | (1 << PPI_CH_PHYEND);

//     // Define GPIOTE tasks
//     uint32_t gpiote_tasks_set = (uint32_t)&NRF_GPIOTE->TASKS_SET[GPIOTE_CH_OUT];  // Set to (1)
//     uint32_t gpiote_tasks_clr = (uint32_t)&NRF_GPIOTE->TASKS_CLR[GPIOTE_CH_OUT];  // Set to (0)

//     // Set event and task endpoints for radio TX_READY event (1)
//     uint32_t radio_events_ready   = (uint32_t)&NRF_RADIO->EVENTS_TXREADY;
//     NRF_PPI->CH[PPI_CH_READY].EEP = radio_events_ready;
//     NRF_PPI->CH[PPI_CH_READY].TEP = gpiote_tasks_set;  // (1)

//     // Set event and task endpoints for radio ADDRESS/FRAMESTART event (0)
//     uint32_t radio_events_payload_start;
//     if (mode == DB_RADIO_IEEE802154_250Kbit) {
//         radio_events_payload_start = (uint32_t)&NRF_RADIO->EVENTS_FRAMESTART;
//     } else {
//         radio_events_payload_start = (uint32_t)&NRF_RADIO->EVENTS_ADDRESS;
//     }
//     NRF_PPI->CH[PPI_CH_ADDRESS_FRAMESTART].EEP = radio_events_payload_start;
//     NRF_PPI->CH[PPI_CH_ADDRESS_FRAMESTART].TEP = gpiote_tasks_clr;  // (0)

//     // Set event and task endpoints for radio PAYLOAD event (1)
//     uint32_t radio_events_payload   = (uint32_t)&NRF_RADIO->EVENTS_PHYEND;
//     NRF_PPI->CH[PPI_CH_PAYLOAD].EEP = radio_events_payload;
//     NRF_PPI->CH[PPI_CH_PAYLOAD].TEP = gpiote_tasks_set;  // (1)

//     // Set event and task endpoints for radio PHYEND event (0)
//     uint32_t radio_events_phyend   = (uint32_t)&NRF_RADIO->EVENTS_DISABLED;
//     NRF_PPI->CH[PPI_CH_PHYEND].EEP = radio_events_phyend;
//     NRF_PPI->CH[PPI_CH_PHYEND].TEP = gpiote_tasks_clr;  // (0)
// }
