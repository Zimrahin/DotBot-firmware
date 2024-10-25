import serial
import time
import jsonlines
import argparse
import os
from hdlc import HDLCHandler, HDLCState

# Default values
DEFAULT_USB_PORT = "/dev/ttyACM0"
DEFAULT_BAUDRATE = 1000000

radio_modes = [
    "BLE_1MBit",
    "BLE_2MBit",
    "BLE_LR125Kbit",
    "BLE_LR500Kbit",
    "IEEE802154_250Kbit"
]

# Class to handle serial communication between the gateway and the computer
class SerialReader:
    def __init__(self, port, baudrate, print_flag):
        # Inputs
        self.port = port
        self.baudrate = baudrate
        self.print_flag = print_flag

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
                message = payload[:length]

                # Ensure radio_mode_index is within valid range
                if 0 <= radio_mode_index < len(radio_modes):
                    radio_mode = radio_modes[radio_mode_index]
                    self.handle_json_file(rx_freq, radio_mode)
                else:
                    print(f"Invalid radio mode index: {radio_mode_index}")
                    return

                # Store payload in a dictionary
                payload_data = {
                    "message": message.decode(),
                    "length": length,
                    "rssi": rssi,
                    "crc": crc,
                }

                # Print or/and store payload
                self.store_payload(payload_data)

    def handle_json_file(self, rx_freq, radio_mode):
        os.makedirs(radio_mode, exist_ok=True)
        json_file = f"{radio_mode}/radio_data_{rx_freq}MHz.jsonl"

        # Initialize json_writer if not already done
        if not self.json_writer:
            self.json_writer = jsonlines.open(json_file, mode="a")

    def store_payload(self, payload_data):
        if self.json_writer:
            self.json_writer.write(payload_data)
        if self.print_flag:
            print(payload_data)

    def __del__(self):
        if self.json_writer:
            if not self.json_writer.closed:
                self.json_writer.close()


if __name__ == "__main__":
    # Set up argument parser
    parser = argparse.ArgumentParser(description="Serial Reader CLI")
    parser.add_argument("-p", "--port", type=str, default=DEFAULT_USB_PORT, help="USB port (default: /dev/ttyACM0)")
    parser.add_argument("-b", "--baudrate", type=int, default=DEFAULT_BAUDRATE, help="Baud rate (default: 1000000)")
    parser.add_argument("-P", "--print", action="store_true", help="Print data to console")

    # Parse arguments
    args = parser.parse_args()

    # Initialise SerialReader with CLI arguments or default values
    usb_reader = SerialReader(args.port, args.baudrate, args.print)
    usb_reader.read_data()
