/**
 * @file
 * @ingroup samples_bsp
 *
 * @brief This example shows how to transmit synched interference packets over the radio
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

//=========================== defines ===========================================

#define MAX_PAYLOAD_SIZE (16)  // Maximum message size

#define PPI_CH_TXENABLE_SYNCH     (0)  // PPI channel destined to set TXENABLE in synch with master clock
#define PPI_CH_READY              (1)  // PPI channel destined to radio TX_READY event debugging
#define PPI_CH_ADDRESS_FRAMESTART (2)  // PPI channel destined to radio ADDRESS or FRAMESTART event debugging
#define PPI_CH_PAYLOAD            (3)  // PPI channel destined to radio PAYLOAD event debugging
#define PPI_CH_PHYEND             (4)  // PPI channel destined to radio PHYEND event debugging
#define PPI_CH_DISABLED           (5)  // PPI channel destined to radio DISABLED event debugging

#define GPIOTE_CH_OUT (1)  // GPIOTE channel for RADIO TX visualization
#define GPIOTE_CH_IN  (2)  // GPIOTE channel for master clock TX synchronisation

typedef struct __attribute__((packed)) {
    uint8_t message[MAX_PAYLOAD_SIZE];  // Actual message
} _radio_pdu_t;

//=========================== variables =========================================

static const uint8_t packet_tx[] = {
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,  //
    0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E,  //
};  // 16 Bytes long

static const gpio_t _pin_square_in        = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };  // Square signal from master
static const gpio_t _pin_radio_events_out = { .port = DB_LED2_PORT, .pin = DB_LED2_PIN };  // Show radio events in digital analyser

static _radio_pdu_t _radio_pdu = { 0 };

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

void _ppi_setup(db_radio_mode_t mode) {
    // Enable PPI channels
    NRF_PPI->CHENSET = (1 << PPI_CH_TXENABLE_SYNCH) |
                       (1 << PPI_CH_READY) |
                       (1 << PPI_CH_ADDRESS_FRAMESTART) |
                       (1 << PPI_CH_PAYLOAD) |
                       (1 << PPI_CH_PHYEND) |
                       (1 << PPI_CH_DISABLED);

    // Define GPIOTE tasks for transmission visualisation in digital analyser
    uint32_t gpiote_tasks_set = (uint32_t)&NRF_GPIOTE->TASKS_SET[GPIOTE_CH_OUT];  // Set to (1)
    uint32_t gpiote_tasks_clr = (uint32_t)&NRF_GPIOTE->TASKS_CLR[GPIOTE_CH_OUT];  // Set to (0)

    // Set event and task endpoints for master clock synchronisation
    uint32_t gpiote_events_toggle            = (uint32_t)&NRF_GPIOTE->EVENTS_IN[GPIOTE_CH_IN];
    uint32_t txenable_task_set               = (uint32_t)&NRF_RADIO->TASKS_TXEN;
    NRF_PPI->CH[PPI_CH_TXENABLE_SYNCH].EEP   = gpiote_events_toggle;
    NRF_PPI->CH[PPI_CH_TXENABLE_SYNCH].TEP   = txenable_task_set;
    NRF_PPI->FORK[PPI_CH_TXENABLE_SYNCH].TEP = gpiote_tasks_set;  // (1)

    // Set event and task endpoints for radio TX_READY event (0)
    uint32_t radio_events_ready   = (uint32_t)&NRF_RADIO->EVENTS_TXREADY;
    NRF_PPI->CH[PPI_CH_READY].EEP = radio_events_ready;
    NRF_PPI->CH[PPI_CH_READY].TEP = gpiote_tasks_clr;  // (0)

    // Set event and task endpoints for radio ADDRESS/FRAMESTART event (1)
    uint32_t radio_events_payload_start;
    if (mode == DB_RADIO_IEEE802154_250Kbit) {
        radio_events_payload_start = (uint32_t)&NRF_RADIO->EVENTS_FRAMESTART;
    } else {
        radio_events_payload_start = (uint32_t)&NRF_RADIO->EVENTS_ADDRESS;
    }
    NRF_PPI->CH[PPI_CH_ADDRESS_FRAMESTART].EEP = radio_events_payload_start;
    NRF_PPI->CH[PPI_CH_ADDRESS_FRAMESTART].TEP = gpiote_tasks_set;  // (1)

    // Set event and task endpoints for radio PAYLOAD event (0)
    uint32_t radio_events_payload   = (uint32_t)&NRF_RADIO->EVENTS_PAYLOAD;
    NRF_PPI->CH[PPI_CH_PAYLOAD].EEP = radio_events_payload;
    NRF_PPI->CH[PPI_CH_PAYLOAD].TEP = gpiote_tasks_clr;  // (0)

    // Set event and task endpoints for radio PHYEND event (1)
    uint32_t radio_events_phyend   = (uint32_t)&NRF_RADIO->EVENTS_PHYEND;
    NRF_PPI->CH[PPI_CH_PHYEND].EEP = radio_events_phyend;
    NRF_PPI->CH[PPI_CH_PHYEND].TEP = gpiote_tasks_set;  // (1)

    // Set event and task endpoints for radio DISABLED event (0)
    uint32_t radio_events_disabled   = (uint32_t)&NRF_RADIO->EVENTS_DISABLED;
    NRF_PPI->CH[PPI_CH_DISABLED].EEP = radio_events_disabled;
    NRF_PPI->CH[PPI_CH_DISABLED].TEP = gpiote_tasks_clr;  // (0)
}

//=========================== main ==============================================

int main(void) {
    // Initialize message to _radio_pdu_t struct
    memcpy(_radio_pdu.message, packet_tx, sizeof(packet_tx));

    // Configure Radio
    db_radio_init(NULL, DOTBOT_GW_RADIO_MODE);
    db_radio_set_frequency(FREQUENCY);
    NRF_RADIO->TXPOWER = (TX_POWER << RADIO_TXPOWER_TXPOWER_Pos);
    db_radio_memcpy2buffer((uint8_t *)&_radio_pdu, sizeof(_radio_pdu));  // Always send same blocker

    // Set PPI and GPIOTE
    _gpiote_setup(&_pin_square_in, &_pin_radio_events_out);
    _ppi_setup(DOTBOT_GW_RADIO_MODE);

    while (1) {
        __WFI();  // Wait for interruption
    }
}
