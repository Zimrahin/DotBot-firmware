import jsonlines
import random

class JsonlMessageModifier:
    def __init__(self, input_file, output_file):
        self.input_file = input_file
        self.output_file = output_file

    def modify_message_and_crc(self):
        with jsonlines.open(self.input_file) as reader, jsonlines.open(self.output_file, mode="w") as writer:
            for obj in reader:
                original_message = obj['message'].encode('ascii')
                modified_message = bytearray(original_message)  # Create mutable byte array from message
                
                # Randomly modify bytes from index 32 to 95
                for i in range(32, 32 + 64):
                    modified_message[i] = random.randint(0, 127)

                # Update the message in the object and convert back to ASCII
                obj['message'] = modified_message.decode('ascii', errors='ignore')
                
                # Randomly set CRC to 0 with a 50% probability
                obj['crc'] = 0 if random.random() < 0.5 else obj['crc']
                
                # Write the modified object to the new JSONL file
                writer.write(obj)

        print(f"Modified messages and CRCs written to {self.output_file}")


# Usage example
if __name__ == "__main__":
    modifier = JsonlMessageModifier("experiment_data/BLE1MBit_IEEE802154250Kbit_0dBm0dBm_0MHz.jsonl", "modified_data.jsonl")
    modifier.modify_message_and_crc()
