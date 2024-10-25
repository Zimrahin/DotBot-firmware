import serial
import time
import jsonlines
from hdlc import HDLCHandler, HDLCState

USB_PORT = "/dev/ttyACM0"
BAUDRATE = 1000000
JSON_FILE = "radio_data.jsonl"


# Class to handle serial communication between the gateway and the computer
class SerialReader:
    def __init__(self, port, baudrate, json_file, print_flag):
        # Inputs
        self.port = port
        self.baudrate = baudrate
        self.json_file = json_file
        self.print_flag = print_flag

        # Serial constants
        self.timeout = 1
        self._sleep_seconds = 0.1
        self.hdlc_handler = HDLCHandler()

        if self.json_file:
            self.json_writer = jsonlines.open(JSON_FILE, mode="a")

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
            if self.json_file:
                self.json_writer.close()

    def process_incoming_data(self, data):
        self.hdlc_handler.handle_byte(data)
        if self.hdlc_handler.state == HDLCState.READY:
            payload = self.hdlc_handler.payload
            if payload:
                length = int.from_bytes(payload[-2].to_bytes(1, "little"), "little", signed=False)
                rssi = int.from_bytes(payload[-1].to_bytes(1, "little"), "little", signed=True)
                message = payload[:length]

                # Store payload in a dictionary
                payload_data = {"message": message.decode(), "length": length, "rssi": rssi}

                # Print or/and store payload
                self.store_payload(payload_data)

    def store_payload(self, payload_data):
        if self.json_file:
            self.json_writer.write(payload_data)
        if self.print_flag:
            print(payload_data)

    def __del__(self):
        if self.json_file:
            if not self.json_writer.closed:
                self.json_writer.close()


if __name__ == "__main__":
    usb_reader = SerialReader(USB_PORT, BAUDRATE, None, print_flag=True)
    usb_reader.read_data()
