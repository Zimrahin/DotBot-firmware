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

// Include BSP packages
#include "board.h"
#include "board_config.h"
#include "gpio.h"
#include "radio.h"
#include "timer_hf.h"

//=========================== defines ===========================================

#define MAX_PAYLOAD_SIZE (16)  // Maximum message size
#define PPI_CH_ADDRESS   (0)   // PPI channel destined to radio address event debugging
#define PPI_CH_END       (1)   // PPI channel destined to radio end event debugging
#define GPIOTE_CH_OUT    (1)   // GPIOTE channel for RADIO TX

typedef struct __attribute__((packed)) {
    uint8_t message[MAX_PAYLOAD_SIZE];  // Actual message
} _radio_pdu_t;

//=========================== variables =========================================

static const uint8_t packet_tx[] = {
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E,  //
    0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E,  //
};  // 16 Bytes long

static const gpio_t _dbg_pin     = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };
static const gpio_t _dbg_pin_ppi = { .port = DB_LED2_PORT, .pin = DB_LED2_PIN };

static bool         _tx_flag   = false;
static _radio_pdu_t _radio_pdu = { 0 };

//=========================== callbacks =========================================

static void _tx_callback(void *ctx) {
    (void)ctx;
    _tx_flag = true;
}

//=========================== functions =========================================

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
    uint32_t radio_events_address   = (uint32_t)&NRF_RADIO->EVENTS_ADDRESS;
    uint32_t gpiote_tasks_set       = (uint32_t)&NRF_GPIOTE->TASKS_SET[GPIOTE_CH_OUT];
    NRF_PPI->CH[PPI_CH_ADDRESS].EEP = radio_events_address;
    NRF_PPI->CH[PPI_CH_ADDRESS].TEP = gpiote_tasks_set;

    // Set event and task endpoints for radio payload event
    uint32_t radio_events_end   = (uint32_t)&NRF_RADIO->EVENTS_END;
    uint32_t gpiote_tasks_clr   = (uint32_t)&NRF_GPIOTE->TASKS_CLR[GPIOTE_CH_OUT];
    NRF_PPI->CH[PPI_CH_END].EEP = radio_events_end;
    NRF_PPI->CH[PPI_CH_END].TEP = gpiote_tasks_clr;
}

//=========================== main ==============================================

int main(void) {
    // Set PPI and GPIOTE
    _gpiote_setup(&_dbg_pin_ppi);
    _ppi_setup();

    // Turn ON the DotBot board regulator
    db_board_init();

    // Initialize message to _radio_pdu_t struct
    memcpy(_radio_pdu.message, packet_tx, sizeof(packet_tx));

    //=========================== Initialize GPIO and timer =====================

    // db_gpio_init(&_dbg_pin, DB_GPIO_OUT);
    db_timer_hf_init(0);

    //=========================== Configure Radio ===============================

    db_radio_init(NULL, DOTBOT_GW_RADIO_MODE);
    db_radio_set_frequency(FREQUENCY);

    // Set interrupt callbacks
    db_gpio_init_irq(&_dbg_pin, DB_GPIO_IN, DB_GPIO_IRQ_EDGE_BOTH, _tx_callback, NULL);

    while (1) {
        if (_tx_flag) {
            // Send packet
            db_radio_disable();
            db_radio_tx((uint8_t *)&_radio_pdu, sizeof(_radio_pdu));

            _tx_flag = false;
        }
        __WFI();  // Wait for interruption
    }
}
