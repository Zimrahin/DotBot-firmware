# Radio RX Board Support Package Usage Example

Load this app onto an nRF board to receive packets using the protocol and frequency specified in the `conf.h` file.

Received packets are then parsed via serial communication to a computer.

This app works with `01bsp_radio_tx` and `01bsp_radio_blocker` to test signal blocking.
