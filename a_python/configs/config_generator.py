from itertools import product

# Parameters
tx_modes = ["BLE1MBit", "IEEE802154250Kbit"]
block_modes = ["BLE1MBit", "IEEE802154250Kbit", "tone"]
# blocker_powers = [-20, -16, -12, -8, -4, 0, 2, 3, 4, 5, 6, 7, 8]
blocker_powers = [-4, 2]

tx_freq = 2450  # MHz
# freq_offsets = [-2, -1, 0, 1, 2]
freq_offsets = [0, 1]

# Constants
BLOCKER_DELAY_US = {"BLE1MBit": 255, "IEEE802154250Kbit": 940}
TONE_BLOCKER_US = {"BLE1MBit": 645, "IEEE802154250Kbit": 1088}
BLOCK_PACKET_SIZE = {"BLE1MBit": 8, "IEEE802154250Kbit": 120}
TX_PACKET_SIZE = {"BLE1MBit": 120, "IEEE802154250Kbit": 80}
TX_POWER = 0  # dBm


# Generate configurations
def generate_configs():
    configs = []
    state_index = 0

    for tx_mode, block_mode, block_power, freq_offset in product(
        tx_modes, block_modes, blocker_powers, freq_offsets
    ):
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
            "tx_power": TX_POWER,
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
        tx_mode = f"DB_RADIO_{c['tx_mode']}"
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
        packet = "packet_tx"

        # Format the string with the above variables
        config_str = f"    {{ {tx_mode}, {tx_freq}, {tx_power}, " f"{delay_us}, {tone_blocker_us}, {increase_id}, {tx_packet_size}, {packet} }},\n"
        formatted_configs.append(config_str)

    return formatted_configs


# Format C blocker configs
def format_c_configs_blocker(configs):
    formatted_configs = []
    for c in configs:
        if c["block_mode"] == block_modes[2]:  # "tone"
            tx_mode = f"DB_RADIO_{block_modes[0]}"  # Set BLE as default when in tone mode
        else:
            tx_mode = f"DB_RADIO_{c['block_mode']}"
        tx_freq = c["tx_freq"] - 2400
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
        packet = "packet_tx"

        # Format the string with the above variables
        config_str = f"    {{ {tx_mode}, {tx_freq}, {tx_power}, " f"{delay_us}, {tone_blocker_us}, {increase_id}, {tx_packet_size}, {packet} }},\n"
        formatted_configs.append(config_str)

    return formatted_configs


# Write configurations to files
def write_configs():
    configs = generate_configs()

    # Receiver (Python)
    with open("receiver_config.py", "w") as f:
        f.write("configs = [\n")
        f.writelines(format_python_configs(configs))
        f.write("]\n")

    # Main transmitter (C)
    with open("main_tx_config.c", "w") as f:
        f.write("static const radio_config_t configs[] = {\n")
        f.writelines(format_c_configs_tx(configs))
        f.write("};\n")

    # Blocker (C)
    with open("blocker_config.c", "w") as f:
        f.write("static const radio_config_t configs[] = {\n")
        f.writelines(format_c_configs_blocker(configs))
        f.write("};\n")


# Run the script
write_configs()
