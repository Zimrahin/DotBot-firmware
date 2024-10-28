import jsonlines
import numpy as np
import matplotlib.pyplot as plt


class JsonlReader:
    def __init__(self, file_path, reference_path="reference_message.jsonl"):
        self.file_path = file_path

        # Load reference message
        with jsonlines.open(reference_path) as reader:
            self.reference_message = reader.read()["message"]

        self.bit_error_counts = np.zeros(len(self.reference_message) * 8)  # bit-level error count
        self.total_packets = 0
        self.crc_error_count = 0

    def read_data(self):
        # Read JSONL file and calculate bit errors
        with jsonlines.open(self.file_path) as reader:
            for line in reader:
                self.total_packets += 1
                message_bytes = line["message"]

                if line["crc"] == 0:
                    self.crc_error_count += 1

                # Calculate bit errors with respect to the reference message
                self._calculate_bit_errors(message_bytes)

    def _calculate_bit_errors(self, message_bytes):
        # Ensure messages are of equal length for comparison
        for byte_index, (ref_byte, msg_byte) in enumerate(zip(self.reference_message, message_bytes)):
            # Perform XOR operation between the reference byte and the message byte
            error_bits = ref_byte ^ msg_byte

            # Count bit errors by checking each bit position in the error_bits
            for bit_index in range(8):  # There are 8 bits in a byte
                # (1 << bit_index) is a mask. For example, 1 << 7 = 1000 0000
                if error_bits & (1 << bit_index):
                    # Increment error count for the specific bit position
                    self.bit_error_counts[byte_index * 8 + bit_index] += 1

    def compute_packet_error_rate(self):
        per = (self.crc_error_count / self.total_packets) * 100.0
        print(f"Packet Error Rate (PER): {per:.2f}%")
        return per

    def plot_bit_error_rate(self):
        # Calculate the percentage of error for each bit
        bit_error_rates = (self.bit_error_counts / self.total_packets) * 100.0
        # Plot error rate for each bit position
        plt.figure(figsize=(12, 6))
        plt.plot(range(len(bit_error_rates)), bit_error_rates, color="blue")
        plt.xlabel("Bit Position")
        plt.ylabel("Error Rate (%)")
        plt.title("Bit Error Rate per Bit Position")
        plt.show()


if __name__ == "__main__":
    jsonl_reader = JsonlReader("modified_data.jsonl")
    jsonl_reader.read_data()
    jsonl_reader.compute_packet_error_rate()
    jsonl_reader.plot_bit_error_rate()
