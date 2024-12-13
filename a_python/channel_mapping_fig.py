import numpy as np
import matplotlib.pyplot as plt

BLE_width = 1.2
IEEE_width = 2.0

def sinc_in_db(x, center, width):
    # Create a sinc-like curve in decibels, normalized and clipped to avoid side lobes
    sinc = np.sinc((x - center) / (width / 2))
    sinc_db = 20 * np.log10(np.abs(sinc) + 1e-6)  # Convert to dB scale
    sinc_db = np.clip(sinc_db, -20, 0)  # Clip below -50 dB and normalize
    # Clip values outside the center +- width range
    mask = np.abs(x - center) <= (width / 2)
    return np.where(mask, sinc_db, -20)  # Set values outside range to -50 dB

# BLE channel frequencies (in MHz)
ble_channels = {
    0: 2402, 1: 2404, 2: 2406, 3: 2408, 4: 2410, 5: 2412, 6: 2414, 7: 2416, 8: 2418,
    9: 2420, 10: 2422, 11: 2424, 12: 2426, 13: 2428, 14: 2430, 15: 2432, 16: 2434,
    17: 2436, 18: 2438, 19: 2440, 20: 2442, 21: 2444, 22: 2446, 23: 2448, 24: 2450,
    25: 2452, 26: 2454, 27: 2456, 28: 2458, 29: 2460, 30: 2462, 31: 2464, 32: 2466,
    33: 2468, 34: 2470, 35: 2472, 36: 2474, 37: 2476, 38: 2478, 39: 2480
}

# 802.15.4 channel frequencies (in MHz)
zigbee_channels = {
    11: 2405, 12: 2410, 13: 2415, 14: 2420, 15: 2425, 16: 2430, 17: 2435, 18: 2440,
    19: 2445, 20: 2450, 21: 2455, 22: 2460, 23: 2465, 24: 2470, 25: 2475, 26: 2480
}

# Plot configuration
freq_range = np.linspace(2400, 2500, 1000)
plt.figure(figsize=(12, 6))

# Plot BLE channels
for channel, freq in ble_channels.items():
    sinc_curve = sinc_in_db(freq_range, freq, BLE_width)
    plt.fill(freq_range, sinc_curve, label=f"BLE {channel}", color="blue", alpha=0.5)
    plt.text(freq, -15, str(freq), ha="center", va="top", color="blue", fontsize=8, rotation=45)

# Plot 802.15.4 channels
for channel, freq in zigbee_channels.items():
    sinc_curve = sinc_in_db(freq_range, freq, IEEE_width)
    plt.fill(freq_range, sinc_curve, label=f"802.15.4 {channel}", color="orange", alpha=0.5)
    plt.text(freq, 10, str(freq), ha="center", va="bottom", color="orange", fontsize=8, rotation=45)

# Final formatting
# plt.axhline(0, color="black", linewidth=0.8)
plt.xticks([])
plt.yticks([])
plt.grid(False)

# Add legend below the plot
plt.legend(["BLE Channels", "IEEE 802.15.4 Channels"], loc="lower center", bbox_to_anchor=(0.5, -0.15), ncol=2, frameon=False, markerscale=2)
plt.tight_layout(rect=[0, 0.2, 1, 1])
plt.show()
