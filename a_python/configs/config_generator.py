from itertools import product
import sys
import random

# Possible power values supported by the nRF
# tx_power_values = [-40, -20, -16, -12, -8, -4, 0, 2, 3, 4, 5, 6, 7, 8]

same_protocol_blocking = 0  # (0), (1) BLE1MBit, (2) IEEE802154250Kbit
pseudo_random_sequence = 10120

tx_freq = 2425  # MHz
BLOCKER_DELAY_US = {"BLE1MBit": 255, "IEEE802154250Kbit": 940}  # (e.g., delay for a blocker when transmitting BLE is 255 µs)
TONE_BLOCKER_US = {"BLE1MBit": 645, "IEEE802154250Kbit": 1088}  # (e.g., duration for a tone blocker when transmitting BLE is 645 µs)
TX_PACKET_SIZE = {"BLE1MBit": 120, "IEEE802154250Kbit": 80}  # (e.g., packet size for BLE transmitter is 120 Bytes)

# Configure Transmission and Blocking Modes Based on the Same Protocol Blocking Parameter
if same_protocol_blocking == 0:  # Different protocol blocking
    # Constants
    BLOCK_PACKET_SIZE = {"BLE1MBit": 8, "IEEE802154250Kbit": 120}  # (e.g., packet size for a 15.4 blocker when transmitting BLE is 8 Bytes)
    tx_power = 0  # dBm
    blocker_powers = [-4, 0, 2, 4, 6, 8]
    freq_offsets = [-2, -1, 0, 1, 2]

    tx_modes = ["BLE1MBit", "IEEE802154250Kbit"]
    block_modes = ["BLE1MBit", "IEEE802154250Kbit", "tone"]
    tx_modes_dotbot = ["DB_RADIO_BLE_1MBit", "DB_RADIO_IEEE802154_250Kbit"]


else:  # Same protocol blocking
    # Constants
    BLOCK_PACKET_SIZE = {"BLE1MBit": 64, "IEEE802154250Kbit": 32}  # (e.g., packet size for a BLE blocker when transmitting BLE is 64 Bytes)
    tx_power = -20  # dBm
    blocker_powers = [-40, -20, -16, -12, -8, -4, 0, 2, 4, 6, 8]

    if same_protocol_blocking == 1:  # BLE1MBit
        freq_offsets = [-2, 0, 2]

        tx_modes = ["BLE1MBit"]
        block_modes = ["BLE1MBit"]
        tx_modes_dotbot = ["DB_RADIO_BLE_1MBit"]

    elif same_protocol_blocking == 2:  # IEEE802154250Kbit
        freq_offsets = [-5, 0, 5]

        tx_modes = ["IEEE802154250Kbit"]
        block_modes = ["IEEE802154250Kbit"]
        tx_modes_dotbot = ["DB_RADIO_IEEE802154_250Kbit"]

    else:
        sys.exit("Invalid value for same_protocol_blocking. Exiting.")

mode_mapping = dict(zip(tx_modes, tx_modes_dotbot))  # Map block_modes to tx_modes_dotbot


# Generate configurations
def generate_configs():
    configs = []
    state_index = 0

    for tx_mode, block_mode, block_power, freq_offset in product(tx_modes, block_modes, blocker_powers, freq_offsets):
        # Skip same protocol blocking (using a different set of constants for that case)
        if same_protocol_blocking == 0:
            if tx_mode == block_mode:
                continue

        block_freq = tx_freq + freq_offset

        if block_mode == "tone":
            tone_blocker_us = TONE_BLOCKER_US[tx_mode]
            block_packet_size = 0
        else:
            tone_blocker_us = 0
            block_packet_size = BLOCK_PACKET_SIZE[tx_mode]

        config = {
            "state_index": state_index,
            "tx_mode": tx_mode,
            "block_mode": block_mode,
            "tx_freq": tx_freq,
            "block_freq": block_freq,
            "tx_power": tx_power,
            "block_power": block_power,
            "delay_us": BLOCKER_DELAY_US[tx_mode],
            "tone_blocker_us": tone_blocker_us,
            "tx_packet_size": TX_PACKET_SIZE[tx_mode],
            "block_packet_size": block_packet_size,
        }
        configs.append(config)
        state_index += 1

    return configs


# Format Python receiver configs
def format_python_configs(configs):
    return [
        f"    RadioConfig(\"{c['tx_mode']}\", \"{c['block_mode']}\", {c['tx_freq']}, {c['block_freq']}, {c['tx_power']}, {c['block_power']}, {c['delay_us']}, {c['tone_blocker_us']}, {c['tx_packet_size']}, {c['block_packet_size']}),\n"
        for c in configs
    ]


# Format C transmitter configs
def format_c_configs_tx(configs):
    formatted_configs = []
    for c in configs:
        tx_mode = mode_mapping[c["tx_mode"]]

        tx_freq = c["tx_freq"] - 2400
        if c["tx_power"] > 0:
            tx_power = f"RADIO_TXPOWER_TXPOWER_Pos{c['tx_power']}dBm"
        elif c["tx_power"] == 0:
            tx_power = "RADIO_TXPOWER_TXPOWER_0dBm"
        else:  # c['tx_power'] < 0
            tx_power = f"RADIO_TXPOWER_TXPOWER_Neg{abs(c['tx_power'])}dBm"
        delay_us = 0  # Always 0 delay for main transmitter
        tone_blocker_us = 0  # Main transmitter is never a tone
        increase_id = 1  # (0) Don't increase (Blocker), (1) Increase (Main transmitter)
        tx_packet_size = c["tx_packet_size"]
        increase_packet_offset = 0 # Keep main transmitter packet constant

        # Format the string with the above variables
        config_str = f"    {{ {tx_mode}, {tx_freq}, {tx_power}, {delay_us}, {tone_blocker_us}, {increase_id}, {tx_packet_size}, {increase_packet_offset} }},\n"
        formatted_configs.append(config_str)

    return formatted_configs


# Format C blocker configs
def format_c_configs_blocker(configs):
    formatted_configs = []
    for c in configs:
        if c["block_mode"] == "tone":
            tx_mode = tx_modes_dotbot[0]  # Set BLE as default when in tone mode
        else:
            tx_mode = mode_mapping[c["block_mode"]]

        tx_freq = c["block_freq"] - 2400
        if c["block_power"] > 0:
            tx_power = f"RADIO_TXPOWER_TXPOWER_Pos{c['block_power']}dBm"
        elif c["block_power"] == 0:
            tx_power = "RADIO_TXPOWER_TXPOWER_0dBm"
        else:  # c['block_power'] < 0
            tx_power = f"RADIO_TXPOWER_TXPOWER_Neg{abs(c['block_power'])}dBm"
        delay_us = c["delay_us"]  # Delay for blocker
        tone_blocker_us = c["tone_blocker_us"]
        increase_id = 0  # (0) Don't increase (Blocker), (1) Increase (Main transmitter)
        tx_packet_size = c["block_packet_size"]
        increase_packet_offset = 1 if pseudo_random_sequence != False else 0

        # Format the string with the above variables
        config_str = f"    {{ {tx_mode}, {tx_freq}, {tx_power}, {delay_us}, {tone_blocker_us}, {increase_id}, {tx_packet_size}, {increase_packet_offset} }},\n"
        formatted_configs.append(config_str)

    return formatted_configs


# Format C receiver configs
def format_c_configs_rx(configs):
    formatted_configs = []
    for c in configs:
        tx_mode = mode_mapping[c["tx_mode"]]
        tx_freq = c["tx_freq"] - 2400

        # Format the string with the above variables
        config_str = f"    {{ {tx_mode}, {tx_freq} }},\n"
        formatted_configs.append(config_str)

    return formatted_configs

# Generate a random packet
def generate_random_packet(pseudo_random_sequence):
    if pseudo_random_sequence == False:
        return
    packet = [random.randint(0, 255) for _ in range(pseudo_random_sequence)]
    lines = []
    for i in range(0, len(packet), 8):
        line = ", ".join(f"0x{byte:02X}" for byte in packet[i:i + 8])
        lines.append(f"    {line},  //")
    return "\n".join(lines)

# Write configurations to files
def write_configs():
    configs = generate_configs()

    # Receiver (Python)
    with open("receiver_config.py", "w") as f:
        f.write("configs = [\n")
        f.writelines(format_python_configs(configs))
        f.write("]\n")

    # Main transmitter (C)
    with open("transmitter_config.c", "w") as f:
        f.write("static const radio_config_t configs[] = {\n")
        f.writelines(format_c_configs_tx(configs))
        f.write("};\n")

    # Blocker (C)
    with open("blocker_config.c", "w") as f:
        f.write("static const radio_config_t configs[] = {\n")
        f.writelines(format_c_configs_blocker(configs))
        f.write("};\n")

    # Receiver (C)
    with open("receiver_config.c", "w") as f:
        f.write("static const radio_config_t configs[] = {\n")
        f.writelines(format_c_configs_rx(configs))
        f.write("};\n")

    # Random packet (C)
    if pseudo_random_sequence != False:
        with open("random_packet.c", "w") as f:
            f.write("static const uint8_t packet_tx[] = {\n")
            f.write(generate_random_packet(pseudo_random_sequence))
            f.write("\n};  " + f"// {pseudo_random_sequence} long\n")


# Run the script
write_configs()
