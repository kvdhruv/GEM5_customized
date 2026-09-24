import os
import re
import matplotlib.pyplot as plt

traffic = "uniform_random"
routing = "XY"

# 1. Generate the exact float strings your sweep used
rates_fine = [f"{i / 1000:.3f}" for i in range(10, 20)] # Generates 0.010 to 0.019
rates_coarse = [f"{i * 0.02:.2f}" for i in range(1, 46)] # Generates 0.02 to 0.90
rates = rates_fine + rates_coarse

valid_rates = []
latencies = []

print("Scanning stats.txt files and extracting data...")

# 2. Safely extract data directly from the files
for rate in rates:
    stat_file = f"noc_results_new/m5out_{traffic}_{routing}_{rate}/stats.txt"
    
    if os.path.exists(stat_file):
        with open(stat_file, 'r') as f:
            content = f.read()
            
            # Robust regex to catch the latency (handles both integers and floats)
            match = re.search(r'system\.ruby\.network\.average_packet_latency\s+([0-9\.]+)', content)
            
            if match:
                # Convert raw Ticks to Cycles
                cycles = float(match.group(1)) / 1000.0
                valid_rates.append(float(rate))
                latencies.append(cycles)

# 3. Plot the data
plt.figure(figsize=(10, 6))
plt.plot(valid_rates, latencies, marker='o', linewidth=2, color='blue', label=f'{routing} Routing')

# 4. Cap the Y-axis so the baseline curve isn't squashed
plt.ylim(0, 20) 
plt.xlim(0, 0.7)

plt.title(f'4x4 Mesh NoC Latency vs. Injection Rate ({traffic.replace("_", " ").title()})')
plt.xlabel('Injection Rate (flits/node/cycle)')
plt.ylabel('Average Packet Latency (cycles)')
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend()

# 5. Your exact save and print commands
plt.savefig('noc_results_new/noc_latency_curve.png', dpi=300, bbox_inches='tight')
print("Graph successfully saved inside noc_results_new/noc_latency_curve.png")