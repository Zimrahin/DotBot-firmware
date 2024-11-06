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
#include "clock.h"

//=========================== defines ===========================================

#define MAX_PAYLOAD_SIZE (16)  // Maximum message size

#define PPI_CH_TXENABLE_SYNCH     (0)  // PPI channel destined to set TXENABLE in synch with master clock
#define PPI_CH_READY              (1)  // PPI channel destined to radio TX_READY event debugging
#define PPI_CH_ADDRESS_FRAMESTART (2)  // PPI channel destined to radio ADDRESS or FRAMESTART event debugging
#define PPI_CH_PAYLOAD            (3)  // PPI channel destined to radio PAYLOAD event debugging
#define PPI_CH_END                (4)  // PPI channel destined to radio PHYEND event debugging
#define PPI_CH_PHYEND             (5)  // PPI channel destined to radio DISABLED event debugging
#define PPI_CH_TIMER_START        (6)  // PPI channel destined to start the timer

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
                       (1 << PPI_CH_END) |
                       (1 << PPI_CH_PHYEND) |
                       (1 << PPI_CH_TIMER_START);

    // Define GPIOTE tasks for transmission visualisation in digital analyser
    uint32_t gpiote_tasks_set = (uint32_t)&NRF_GPIOTE->TASKS_SET[GPIOTE_CH_OUT];  // Set to (1)
    uint32_t gpiote_tasks_clr = (uint32_t)&NRF_GPIOTE->TASKS_CLR[GPIOTE_CH_OUT];  // Set to (0)

    // Set event and task endpoints to start timer
    NRF_PPI->CH[PPI_CH_TIMER_START].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[GPIOTE_CH_IN];
    NRF_PPI->CH[PPI_CH_TIMER_START].TEP = (uint32_t)&NRF_TIMER0->TASKS_START;

    // Set event and task endpoints to enable transmission
    NRF_PPI->CH[PPI_CH_TXENABLE_SYNCH].EEP   = (uint32_t)&NRF_TIMER0->EVENTS_COMPARE[0];
    NRF_PPI->CH[PPI_CH_TXENABLE_SYNCH].TEP   = (uint32_t)&NRF_RADIO->TASKS_TXEN;
    NRF_PPI->FORK[PPI_CH_TXENABLE_SYNCH].TEP = gpiote_tasks_set;  // (1)

    // Set event and task endpoints for radio TX_READY event (0)
    NRF_PPI->CH[PPI_CH_READY].EEP = (uint32_t)&NRF_RADIO->EVENTS_TXREADY;
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
    NRF_PPI->CH[PPI_CH_PAYLOAD].EEP = (uint32_t)&NRF_RADIO->EVENTS_PAYLOAD;
    NRF_PPI->CH[PPI_CH_PAYLOAD].TEP = gpiote_tasks_clr;  // (0)

    // Set event and task endpoints for radio END event (1)
    NRF_PPI->CH[PPI_CH_END].EEP = (uint32_t)&NRF_RADIO->EVENTS_END;
    NRF_PPI->CH[PPI_CH_END].TEP = gpiote_tasks_set;  // (1)

    // Set event and task endpoints for radio PHYEND event (0)
    NRF_PPI->CH[PPI_CH_PHYEND].EEP = (uint32_t)&NRF_RADIO->EVENTS_PHYEND;
    NRF_PPI->CH[PPI_CH_PHYEND].TEP = gpiote_tasks_clr;  // (0)
}

void _hf_timer_init(uint32_t us) {
    db_hfclk_init();  // Start the high frequency clock if not already on

    NRF_TIMER0->MODE        = (TIMER_MODE_MODE_Timer << TIMER_MODE_MODE_Pos);
    NRF_TIMER0->TASKS_CLEAR = (TIMER_TASKS_CLEAR_TASKS_CLEAR_Trigger << TIMER_TASKS_CLEAR_TASKS_CLEAR_Pos);  // Clear timer
    NRF_TIMER0->BITMODE     = (TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos);                    // 16 bits should be enough (65 ms in total)
    NRF_TIMER0->PRESCALER   = (4 << TIMER_PRESCALER_PRESCALER_Pos);                                          // 16/2⁴= 1MHz
    NRF_TIMER0->CC[0]       = us;                                                                            // Set the number of 1MHz ticks to wait for enabling EVENTS_COMPARE[0]

    // Disable and clear the timer immediately after EVENTS_COMPARE[0] event
    NRF_TIMER0->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos) |
                         (TIMER_SHORTS_COMPARE0_STOP_Enabled << TIMER_SHORTS_COMPARE0_STOP_Pos);
}
//=========================== main ==============================================

int main(void) {
    // Initialise the TIMER0 at channel 0
    _hf_timer_init(DELAY_US);

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
