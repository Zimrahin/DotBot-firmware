#ifndef __PPI_FUNCTIONS_H
#define __PPI_FUNCTIONS_H

#include <stdint.h>
#include <nrf.h>

#include <nrf.h>
#include <stdbool.h>
#include "gpio.h"
#include "radio.h"

// Function prototypes
void _gpiote_setup(const gpio_t *gpio_pin);
void _ppi_setup(db_radio_mode_t mode);

#endif
