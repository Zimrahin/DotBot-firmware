from typing import NamedTuple, List, Optional

# Default values
DEFAULT_USB_PORT = 0 # "/dev/ttyACM0"
DEFAULT_BAUDRATE = 1000000

tx_power_values = [8, 7, 6, 5, 4, 3, 2, 0, -4, -8, -12, -16, -20, -40]  # dBm
radio_modes = ["BLE1MBit", "BLE2MBit", "BLELR125Kbit", "BLELR500Kbit", "IEEE802154250Kbit", "tone"]



# Define the radio configuration structure as a NamedTuple
class RadioConfig(NamedTuple):
    tx_mode: str                # DB_RADIO_BLE_1MBit, DB_RADIO_IEEE802154_250Kbit
    block_mode: str
    tx_freq: int                # MHz
    block_freq: int             # MHz
    tx_power: int               # dBm
    block_power: int            # dBm
    delay_us: int               # Wait delay_us before sending blocker
    tone_blocker_us: int        # 0: Normal operation; else: Blocker for the specified duration
    tx_packet_size: int         # Amount of bytes to send
    block_packet_size: int


# Define the configurations as a list of RadioConfig
configs = [
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2420, -20, -8, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2425, -20, -8, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2430, -20, -8, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2420, -20, -4, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2425, -20, -4, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2430, -20, -4, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2420, -20, 0, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2425, -20, 0, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2430, -20, 0, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2420, -20, 2, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2425, -20, 2, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2430, -20, 2, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2420, -20, 4, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2425, -20, 4, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2430, -20, 4, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2420, -20, 6, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2425, -20, 6, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2430, -20, 6, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2420, -20, 8, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2425, -20, 8, 940, 0, 80, 32),
    RadioConfig("IEEE802154250Kbit", "IEEE802154250Kbit", 2425, 2430, -20, 8, 940, 0, 80, 32),
]
