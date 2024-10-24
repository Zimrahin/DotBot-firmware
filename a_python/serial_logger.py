import serial
import time
from hdlc import HDLCHandler, HDLCState

USB_PORT = "/dev/ttyACM0"
BAUDRATE = 1000000


# Class to handle serial communication between the gateway and the computer
class SerialReader:
    def __init__(self, port, baudrate, timeout=1):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.hdlc_handler = HDLCHandler()
        self._sleep_seconds = 0.1

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

    def process_incoming_data(self, data):
        self.hdlc_handler.handle_byte(data)
        if self.hdlc_handler.state == HDLCState.READY:
            payload = self.hdlc_handler.payload
            if payload:
                length = int.from_bytes(payload[-2].to_bytes(1, "little"), "little", signed=False)
                rssi = int.from_bytes(payload[-1].to_bytes(1, "little"), "little", signed=True)
                message = payload[:length]
                self.print_payload_info(message, length, rssi)

    @staticmethod
    def print_payload_info(message, length, rssi):
        print(f"Message: {message}")
        print(f"Length: {length}")
        print(f"RSSI: {rssi}")


if __name__ == "__main__":
    usb_reader = SerialReader(USB_PORT, BAUDRATE)
    usb_reader.read_data()
