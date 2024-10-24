import serial
import time
from hdlc import *

USB_PORT = '/dev/ttyACM0'
BAUDRATE = 1000000

def read_usb():
    try:
        # Open the serial connection
        with serial.Serial(USB_PORT, BAUDRATE, timeout=1) as ser:
            hdlc_handler = HDLCHandler()
            while True:
                incoming_data = ser.read()
                if incoming_data:
                    hdlc_handler.handle_byte(incoming_data)
                    if hdlc_handler.state == HDLCState.READY:
                        payload = hdlc_handler.payload
                        if payload:
                            length = int.from_bytes(
                                payload[-2].to_bytes(1, "little"), "little", signed=False
                            )
                            rssi = int.from_bytes(
                                payload[-1].to_bytes(1, "little"), "little", signed=True
                            )
                            message = payload[:length]
                            print(message)
                            print(length)
                            print(rssi)
                else:
                    # If no data is available, wait a little before the next check
                    time.sleep(0.1)
    except serial.SerialException as e:
        print(f"Error opening or reading from {USB_PORT}: {e}")

if __name__ == "__main__":
    read_usb()
