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
#include <nrf52840_bitfields.h>
#include <nrf52840.h>

// Include BSP packages
#include "board.h"
#include "board_config.h"
#include "gpio.h"
#include "radio.h"
#include "uart.h"
#include "hdlc.h"

//=========================== defines ===========================================

#define DB_UART_MAX_BYTES (255U)       // max bytes in UART buffer
#define DB_UART_BAUDRATE  (1000000UL)  // UART baudrate
#define MAX_PAYLOAD_SIZE  (120U)       // Maximum message size

#if defined(NRF5340_XXAA) && defined(NRF_APPLICATION)
#define DB_UART_INDEX (1)  ///< Index of UART peripheral to use
#else
#define DB_UART_INDEX (0)  ///< Index of UART peripheral to use
#endif

#define NUM_CONFIGS (sizeof(configs) / sizeof(configs[0]))

typedef struct __attribute__((packed)) {
    uint32_t msg_id;                    // Message ID
    uint8_t  buffer[MAX_PAYLOAD_SIZE];  // Buffer containing the radio packet
    uint8_t  length;                    // Length of the radio packet
    int8_t   rssi;                      // Received Signal Strength Indicator in dBm
    bool     crc;                       // Cyclic Redundancy Check
    uint8_t  rx_freq;                   // Frequency of the affected network
    uint8_t  radio_mode;                // Radio protocol of the affected network (BLE or IEEE 802.15.4)
    uint32_t config_state;              // Counter to inform Python code current state
} radio_packet_t;

//=========================== variables =========================================

static const gpio_t _dbg_pin             = { .port = 1, .pin = 11 };
static const gpio_t _pin_in_config_state = { .port = 1, .pin = 10 };  // Toggling this pin changes the state

static uint8_t hdlc_tx_buffer[DB_UART_MAX_BYTES];
static size_t  hdlc_tx_buffer_size;

static radio_packet_t _radio_packet = { 0 };

//=========================== prototypes =========================================

void _init_configurations(void);

static void _radio_callback(uint8_t *packet, uint8_t length);
static void _gpio_callback_change_state(void *ctx);

//=========================== main ==============================================

int main(void) {
    // Initialize GPIO LED
    db_gpio_init(&_dbg_pin, DB_GPIO_OUT);

    // Enable interrupts for changing configuration state
    db_gpio_init(&_pin_in_config_state, DB_GPIO_IN_PD);
    db_gpio_init_irq(&_pin_in_config_state, DB_GPIO_IN_PD, GPIOTE_CONFIG_POLARITY_LoToHi, _gpio_callback_change_state, NULL);

    // Initialize UART
    // db_uart_init(DB_UART_INDEX, &db_uart_rx, &db_uart_tx, DB_UART_BAUDRATE, &_uart_callback);
    db_uart_init(DB_UART_INDEX, &db_uart_rx, &db_uart_tx, DB_UART_BAUDRATE, NULL);

    // Initialise state configurations
    _init_configurations();

    while (1) {
        __WFI();  // Wait for interrupt
    }
}

//=========================== functions =========================================

void _init_configurations(void) {
    // Configure Radio
    db_radio_disable();
    db_radio_init(&_radio_callback, configs[_radio_packet.config_state].radio_mode);
    db_radio_set_frequency(configs[_radio_packet.config_state].frequency);
    db_radio_rx();
}

//=========================== callbacks =========================================

static void _radio_callback(uint8_t *packet, uint8_t length) {
    db_gpio_toggle(&_dbg_pin);  // Toggle LED for visualization of received packet

    // When blocking with the same protocol, it can happen that the blocker is detected
    // which is at the same time being interfered by the main transmitter
    if (length > (MAX_PAYLOAD_SIZE + sizeof(uint32_t))) {
        return;
    }

    // Fill _radio_packet structure
    memset(_radio_packet.buffer, 0, MAX_PAYLOAD_SIZE);                                   // Clear the buffer before filling it with new data in case of leftovers
    memcpy(_radio_packet.buffer, packet + sizeof(uint32_t), length - sizeof(uint32_t));  // length is supposed to be at most 124B

    memcpy(&_radio_packet.msg_id, packet, sizeof(uint32_t));
    _radio_packet.length     = length;
    _radio_packet.rssi       = db_radio_rssi();
    _radio_packet.crc        = NRF_RADIO->CRCSTATUS;
    _radio_packet.rx_freq    = configs[_radio_packet.config_state].frequency;
    _radio_packet.radio_mode = configs[_radio_packet.config_state].radio_mode;

    hdlc_tx_buffer_size = db_hdlc_encode((uint8_t *)&_radio_packet, sizeof(radio_packet_t), hdlc_tx_buffer);
    db_uart_write(DB_UART_INDEX, hdlc_tx_buffer, hdlc_tx_buffer_size);
}

static void _gpio_callback_change_state(void *ctx) {
    (void)ctx;
    _radio_packet.config_state += 1;
    if (_radio_packet.config_state < NUM_CONFIGS) {
        _init_configurations();
    } else {
        // Stop receiving
        db_radio_disable();
    }
}
