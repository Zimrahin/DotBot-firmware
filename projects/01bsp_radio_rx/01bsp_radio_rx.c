/**
 * @file
 * @ingroup samples_bsp
 *
 * @brief This is an example of reading received radio packets on a computer via serial
 *
 * @author Diego Badillo-San-Juan <diego.badillo-san-juan@inria.fr>
 *
 * @copyright Inria, 2024
 *
 */
#include <nrf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include BSP packages
#include "board.h"
#include "board_config.h"
#include "gpio.h"
#include "radio.h"
#include "timer_hf.h"
#include "uart.h"
#include "hdlc.h"

//=========================== defines ===========================================

#define DB_UART_MAX_BYTES (255U)       // max bytes in UART buffer
#define DB_UART_BAUDRATE  (1000000UL)  // UART baudrate

#if defined(NRF5340_XXAA) && defined(NRF_APPLICATION)
#define DB_UART_INDEX (1)  ///< Index of UART peripheral to use
#else
#define DB_UART_INDEX (0)  ///< Index of UART peripheral to use
#endif

typedef struct __attribute__((packed)) {
    uint8_t buffer[DB_IEEE802154_PAYLOAD_MAX_LENGTH];  // Buffer containing the radio packet
    uint8_t length;                                    // Length of the radio packet
    int8_t  rssi;                                      // Received Signal Strength Indicator in dBm
    bool    crc;                                       // Cyclic Redundancy Check
    uint8_t rx_freq;                                   // Frequency of the affected network
    uint8_t radio_mode;                                // Radio protocol of the affected network (BLE or IEEE 802.15.4)
} radio_packet_t;

//=========================== variables =========================================

static const gpio_t _dbg_pin = { .port = DB_LED1_PORT, .pin = DB_LED1_PIN };

static uint8_t hdlc_tx_buffer[DB_UART_MAX_BYTES];
static size_t  hdlc_tx_buffer_size;

static radio_packet_t _radio_packet = { 0 };

//=========================== callbacks =========================================

static void _radio_callback(uint8_t *packet, uint8_t length) {
    db_gpio_toggle(&_dbg_pin);  // Toggle LED for visualization of received packet

    // Fill _radio_packet structure
    memset(_radio_packet.buffer, 0, DB_IEEE802154_PAYLOAD_MAX_LENGTH);  // Clear the buffer before filling it with new data in case of leftovers
    memcpy(_radio_packet.buffer, packet, length);
    _radio_packet.length     = length;
    _radio_packet.rssi       = db_radio_rssi();
    _radio_packet.crc        = NRF_RADIO->CRCSTATUS;
    _radio_packet.rx_freq    = FREQUENCY;
    _radio_packet.radio_mode = DOTBOT_GW_RADIO_MODE;

    hdlc_tx_buffer_size = db_hdlc_encode((uint8_t *)&_radio_packet, sizeof(radio_packet_t), hdlc_tx_buffer);
    db_uart_write(DB_UART_INDEX, hdlc_tx_buffer, hdlc_tx_buffer_size);
}

//=========================== main ==============================================

int main(void) {

    // Turn ON the DotBot board regulator
    db_board_init();

    //=========================== Initialize GPIO and timer =====================

    db_gpio_init(&_dbg_pin, DB_GPIO_OUT);
    db_timer_hf_init(0);

    //=========================== Configure Radio ===============================

    db_radio_init(&_radio_callback, DOTBOT_GW_RADIO_MODE);
    db_radio_set_frequency(FREQUENCY);
    db_radio_rx();

    //=========================== Initialize UART ===============================

    // db_uart_init(DB_UART_INDEX, &db_uart_rx, &db_uart_tx, DB_UART_BAUDRATE, &_uart_callback);
    db_uart_init(DB_UART_INDEX, &db_uart_rx, &db_uart_tx, DB_UART_BAUDRATE, NULL);

    while (1) {
        __WFI();  // Wait for interrupt
    }
}
