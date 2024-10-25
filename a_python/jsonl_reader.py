import jsonlines
import numpy as np
import matplotlib.pyplot as plt


class JsonlReader:
    def __init__(self, file_path):
        self.file_path = file_path
        self.reference_message = bytes(
            "AAAAAAAA--------AAAAAAAA--------AAAAAAAA--------AAAAAAAA--------AAAAAAAA--------AAAAAAAA--------AAAAAAAA--------AAAAAAAA12345",
            "ascii",
        )
        self.bit_error_counts = np.zeros(len(self.reference_message) * 8)  # bit-level error count
        self.total_packets = 0
        self.crc_error_count = 0

    def read_data(self):
        # Read JSONL file and calculate bit errors
        with jsonlines.open(self.file_path) as reader:
            for line in reader:
                self.total_packets += 1
                message_bytes = line["message"].encode("ascii")
                if line["crc"] == 0:
                    self.crc_error_count += 1

                # Calculate bit errors with respect to the reference message
                self._calculate_bit_errors(message_bytes)

    def _calculate_bit_errors(self, message_bytes):
        # Make messages are of equal length for comparison
        for byte_index, (ref_byte, msg_byte) in enumerate(zip(self.reference_message, message_bytes)):
            # Convert bytes to bits
            ref_bits = f"{ref_byte:08b}"
            msg_bits = f"{msg_byte:08b}"
            # Count bit errors
            for bit_index, (ref_bit, msg_bit) in enumerate(zip(ref_bits, msg_bits)):
                if ref_bit != msg_bit:
                    # Increment error count for the specific bit position
                    self.bit_error_counts[byte_index * 8 + bit_index] += 1

    def compute_packet_error_rate(self):
        per = (self.crc_error_count / self.total_packets) * 100.
        print(f"Packet Error Rate (PER): {per:.2f}%")
        return per

    def plot_bit_error_rate(self):
        # Calculate the percentage of error for each bit
        bit_error_rates = (self.bit_error_counts / self.total_packets) * 100.
        # Plot error rate for each bit position
        plt.figure(figsize=(12, 6))
        plt.plot(range(len(bit_error_rates)), bit_error_rates, color="blue")
        plt.xlabel("Bit Position")
        plt.ylabel("Error Rate (%)")
        plt.title("Bit Error Rate per Bit Position")
        plt.show()


if __name__ == "__main__":
    processer = JsonlReader("modified_data.jsonl")
    processer.read_data()
    processer.compute_packet_error_rate()
    processer.plot_bit_error_rate()
