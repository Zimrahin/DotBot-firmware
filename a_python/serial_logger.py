import serial
import time
import jsonlines
import argparse
import sys
import os
import matplotlib.pyplot as plt
from hdlc import HDLCHandler, HDLCState
from config import DEFAULT_USB_PORT, DEFAULT_BAUDRATE, tx_power_values, radio_modes, configs

plt.ion()


# Class to handle serial communication between the gateway and the computer
class SerialReader:
    def __init__(self, port, baudrate, print_flag, save_flag, plot_flag):
        # Inputs
        self.port = f"/dev/ttyACM{port}"
        self.baudrate = baudrate
        self.print_flag = print_flag
        self.save_flag = save_flag
        self.plot_flag = plot_flag

        # Serial constants
        self.timeout = 1
        self._sleep_seconds = 0.1
        self.hdlc_handler = HDLCHandler()

        # Initialize JSON writer
        self.json_writer = None

    def read_data(self):
        try:
            with serial.Serial(self.port, self.baudrate, timeout=self.timeout) as ser:
                while True:
                    incoming_data = ser.read()
                    if incoming_data:
                        self.process_incoming_data(incoming_data)
                    else:
                        time.sleep(self._sleep_seconds)
        except serial.SerialException as e:
            print(f"Error opening or reading from {self.port}: {e}")
        finally:
            if self.json_writer:
                self.json_writer.close()

    def process_incoming_data(self, data):
        self.hdlc_handler.handle_byte(data)
        if self.hdlc_handler.state == HDLCState.READY:
            payload = self.hdlc_handler.payload

            if payload:
                # Handle payload variables
                length = int.from_bytes(payload[-9].to_bytes(1, "little"), "little", signed=False)
                rssi = int.from_bytes(payload[-8].to_bytes(1, "little"), "little", signed=True)
                crc = int.from_bytes(payload[-7].to_bytes(1, "little"), "little", signed=True)
                rx_freq = int.from_bytes(payload[-6].to_bytes(1, "little"), "little", signed=True) + 2400  # MHz
                rx_mode_index = int.from_bytes(payload[-5].to_bytes(1, "little"), "little", signed=False)
                self.config_state = int.from_bytes(payload[-4:], "little", signed=False)
                message = payload[4:length]  # The size of msg_id is 4 bytes and it is included in the message
                msg_id = int.from_bytes(payload[:4], "little", signed=False)

                # Store payload in a dictionary
                self.payload_data = {
                    "id": msg_id,
                    "message": list(message),
                    "length": length,  # length includes the 4 bytes used by the identifier
                    "rssi": rssi,
                    "crc": crc,
                    "config_state": self.config_state,
                }

                # Ensure radio link mode is within valid range
                if 0 <= rx_mode_index < (len(radio_modes) - 1):  # -1 because added tone mode
                    rx_mode = radio_modes[rx_mode_index]
                    self.handle_json_file(rx_mode, configs)
                else:
                    print(f"Invalid radio mode index: {rx_mode_index}")
                    sys.exit(1)

                # Print or/and store payload
                self.store_payload(self.payload_data)

    def handle_json_file(self, rx_mode, configs):
        os.makedirs("experiment_data", exist_ok=True)
        self.json_file = f"experiment_data/{rx_mode}_"

        # Unpack current state configurations
        cfg = configs[self.config_state]

        # Assert that tx_mode == rx_mode
        if cfg.tx_mode != rx_mode:
            print("Exiting program because tx_mode != rx_mode")
            sys.exit(1)

        self.json_file += f"{cfg.block_mode}_{cfg.tx_power}dBm{cfg.block_power}dBm_{cfg.tx_freq}MHz{cfg.block_freq}MHz_delay{cfg.delay_us}us_"
        self.json_file += f"tx{cfg.tx_packet_size}B_"
        block_info = f"block{cfg.tone_blocker_us}us" if cfg.block_mode == "tone" else f"block{cfg.block_packet_size}B"
        self.json_file += block_info + ".jsonl"

    def store_payload(self, payload_data):
        if self.save_flag:
            with jsonlines.open(self.json_file, mode="a") as json_writer:
                json_writer.write(payload_data)
        if self.print_flag:
            print(payload_data)
        if self.plot_flag:
            self.plot_payload(payload_data)

    def plot_payload(self, payload_data):
        plt.clf()  # Clear the figure to refresh it
        plt.plot(payload_data["message"], marker="o", linestyle="-", color="b")
        plt.title("Received data")
        plt.xlabel("Byte index")
        plt.ylabel("Message")

        # Create a box with payload details
        crc_text = "OK" if payload_data["crc"] else "ERROR"
        info_text = f"ID: {payload_data['id']}\nLength: {payload_data['length']} B\nRSSI: {payload_data['rssi']} dBm\nCRC: {crc_text}\nCurrent state: {self.config_state}"
        plt.gca().text(1.05, 0.55, info_text, fontsize=10, ha="left", va="top", transform=plt.gca().transAxes, bbox=dict(facecolor="white", alpha=0.5))

        # Show grid and update the plot
        plt.grid()
        plt.tight_layout()
        plt.pause(0.001)  # Pause briefly to allow plot to update

    def __del__(self):
        if self.json_writer:
            if not self.json_writer.closed:
                self.json_writer.close()


if __name__ == "__main__":
    # Set up argument parser
    parser = argparse.ArgumentParser(description="Serial Reader CLI")
    parser.add_argument("-port", "--port", type=str, default=f"/dev/ttyACM{DEFAULT_USB_PORT}", help=f"USB port (default: /dev/ttyACM{DEFAULT_USB_PORT})")
    parser.add_argument("-b", "--baudrate", type=int, default=DEFAULT_BAUDRATE, help=f"Baud rate (default: {DEFAULT_BAUDRATE})")
    parser.add_argument("-print", "--print", action="store_true", help="Print data to console")
    parser.add_argument("-s", "--save", action="store_true", help="Save data in a JSONL file")
    parser.add_argument("-plot", "--plot", action="store_true", help="Plot payload data")

    # Parse arguments
    args = parser.parse_args()

    # Initialise SerialReader with CLI arguments or default values
    usb_reader = SerialReader(args.port, args.baudrate, args.print, args.save, args.plot)
    usb_reader.read_data()
