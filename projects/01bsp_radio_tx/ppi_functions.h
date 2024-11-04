#ifndef __PPI_FUNCTIONS_H
#define __PPI_FUNCTIONS_H

#include <stdint.h>
#include <nrf.h>

#include <nrf.h>
#include <stdbool.h>
#include "gpio.h"
#include "radio.h"

#define PPI_CH_ADDRESS (0)  // PPI channel destined to radio address event debugging
#define PPI_CH_END     (1)  // PPI channel destined to radio end event debugging
#define GPIOTE_CH_OUT  (1)  // GPIOTE channel for RADIO TX

// Function prototypes
void _gpiote_setup(const gpio_t *gpio_pin);
void _ppi_setup(db_radio_mode_t mode);

#endif
