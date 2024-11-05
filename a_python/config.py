# Default values
DEFAULT_USB_PORT = 0 # "/dev/ttyACM0"
DEFAULT_BAUDRATE = 1000000


FREQ_INTERFERER = 2450  # MHz
TX_POWER_INTERFERER = 0  # dBm
TX_POWER_LINK = 0  # dBm
INTERFERER_MODE = "IEEE802154250Kbit"
LENGTH_INTERFERER = 64  # Bytes

tx_power_values = [8, 7, 6, 5, 4, 3, 2, 0, -4, -8, -12, -16, -20, -40]  # dBm
radio_modes = ["BLE1MBit", "BLE2MBit", "BLELR125Kbit", "BLELR500Kbit", "IEEE802154250Kbit", "tone"]
