import serial
import time
import jsonlines
import argparse
import sys
import os
import matplotlib.pyplot as plt
from hdlc import HDLCHandler, HDLCState
from config import (
    DEFAULT_USB_PORT,
    DEFAULT_BAUDRATE,
    FREQ_INTERFERER,
    TX_POWER_INTERFERER,
    TX_POWER_LINK,
    INTERFERER_MODE,
    tx_power_values,
    radio_modes,
)

plt.ion()


# Class to handle serial communication between the gateway and the computer
class SerialReader:
    def __init__(self, port, baudrate, print_flag, save_flag, plot_flag):
        # Inputs
        self.port = port
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
                length = int.from_bytes(payload[-5].to_bytes(1, "little"), "little", signed=False)
                rssi = int.from_bytes(payload[-4].to_bytes(1, "little"), "little", signed=True)
                crc = int.from_bytes(payload[-3].to_bytes(1, "little"), "little", signed=True)
                rx_freq = int.from_bytes(payload[-2].to_bytes(1, "little"), "little", signed=True) + 2400  # MHz
                radio_mode_index = int.from_bytes(payload[-1].to_bytes(1, "little"), "little", signed=True)
                message = payload[4:length]  # The size of msg_id is 4 bytes and it is included in the message
                msg_id = int.from_bytes(payload[:4], "little", signed=False)

                # Ensure radio link mode is within valid range
                if 0 <= radio_mode_index < (len(radio_modes) - 1):  # -1 because added tone mode
                    radio_mode = radio_modes[radio_mode_index]
                    self.handle_json_file(rx_freq, radio_mode)
                else:
                    print(f"Invalid radio mode index: {radio_mode_index}")
                    return

                # Store payload in a dictionary
                payload_data = {
                    "id": msg_id,
                    "message": list(message),
                    "length": length,  # length includes the 4 bytes used by the identifier
                    "rssi": rssi,
                    "crc": crc,
                }

                # Print or/and store payload
                self.store_payload(payload_data)

    def handle_json_file(self, rx_freq, radio_mode):
        os.makedirs("experiment_data", exist_ok=True)
        freq_diff = FREQ_INTERFERER - rx_freq  # Frequency difference between interferer and link
        json_file = f"experiment_data/{radio_mode}_{INTERFERER_MODE}_{TX_POWER_LINK}dBm{TX_POWER_INTERFERER}dBm_{freq_diff}MHz.jsonl"

        # Initialize json_writer if not already done
        if not self.json_writer and self.save_flag:
            self.json_writer = jsonlines.open(json_file, mode="a")

    def store_payload(self, payload_data):
        if self.json_writer and self.save_flag:
            self.json_writer.write(payload_data)
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
        info_text = f"ID: {payload_data['id']}\nLength: {payload_data['length']}\n" f"RSSI: {payload_data['rssi']}\nCRC: {payload_data['crc']}"
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
    # Check if global defines are within range
    if not (isinstance(FREQ_INTERFERER, int) and 2400 <= FREQ_INTERFERER <= 2500):
        print("FREQ_INTERFERER must be an integer between 2400 and 2500 MHz.")
        sys.exit(1)
    if TX_POWER_INTERFERER not in tx_power_values or TX_POWER_LINK not in tx_power_values:
        print("TX_POWER_INTERFERER and TX_POWER_LINK must match the nRF's allowed transmission values.")
        sys.exit(1)
    if INTERFERER_MODE not in radio_modes:
        print("INTERFERER_MODE must match the nRF's allowed radio modes.")
        sys.exit(1)

    # Set up argument parser
    parser = argparse.ArgumentParser(description="Serial Reader CLI")
    parser.add_argument("-P", "--port", type=str, default=DEFAULT_USB_PORT, help=f"USB port (default: {DEFAULT_USB_PORT})")
    parser.add_argument("-b", "--baudrate", type=int, default=DEFAULT_BAUDRATE, help=f"Baud rate (default: {DEFAULT_BAUDRATE})")
    parser.add_argument("-p", "--print", action="store_true", help="Print data to console")
    parser.add_argument("-s", "--save", action="store_true", help="Save data in a JSONL file")
    parser.add_argument("-plot", "--plot", action="store_true", help="Plot payload data")

    # Parse arguments
    args = parser.parse_args()

    # Initialise SerialReader with CLI arguments or default values
    usb_reader = SerialReader(args.port, args.baudrate, args.print, args.save, args.plot)
    usb_reader.read_data()
