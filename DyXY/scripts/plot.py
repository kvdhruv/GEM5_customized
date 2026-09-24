import os
import re
import matplotlib.pyplot as plt

# Configuration mirroring the updated bash script
traffics = ["uniform_random", "transpose", "bit_complement", "shuffle"]
algo_names = ["XY", "ODD_EVEN", "DyXY"]
colors = {'XY': 'green', 'ODD_EVEN': 'blue', 'DyXY': 'red'}
markers = {'XY': '+', 'ODD_EVEN': 'x', 'DyXY': 'o'}

# Generate rates from 0.02 to 0.50 with a step of 0.02
rates = [f"{x/100:.2f}" for x in range(2, 52, 2)]

# Define paths
script_dir = os.path.dirname(os.path.abspath(__file__))
base_dir = os.path.abspath(os.path.join(script_dir, '..'))

results_dir = os.path.join(base_dir, 'results')
plots_dir = os.path.join(base_dir, 'plots')

print(f"Scanning for data in {results_dir}...")

# Create the 2x2 subplot matrix (since we now have exactly 4 traffics)
fig, axes = plt.subplots(2, 2, figsize=(14, 10))
fig.tight_layout(pad=6.0)

for idx, traffic in enumerate(traffics):
    # Adjust for 2x2 layout
    ax = axes[idx // 2, idx % 2]
    
    for algo in algo_names:
        latencies = []
        valid_rates = []
        
        for rate in rates:
            # Match the directory naming convention from the bash script
            stat_file = os.path.join(results_dir, f"m5out_{traffic}_{algo}_{rate}", "stats.txt")
            
            if os.path.exists(stat_file):
                with open(stat_file, 'r') as f:
                    content = f.read()
                    match = re.search(r'system\.ruby\.network\.average_packet_latency\s+([0-9\.]+)', content)
                    
                    if match:
                        cycles = float(match.group(1)) / 500.0
                        latencies.append(cycles)
                        valid_rates.append(float(rate))
                    else:
                        # Network is saturated; tag as infinity so it shoots up and off the graph
                        latencies.append(float('inf')) 
                        valid_rates.append(float(rate))
        
        # Plot the extracted data
        if valid_rates:
            ax.plot(valid_rates, latencies, marker=markers[algo], color=colors[algo], label=algo, linewidth=2)

    # Format each subplot to match the paper's style
    ax.set_title(f'{traffic.replace("_", " ")} traffic', fontsize=14, pad=10)
    ax.set_xlabel('injection rate [packets per cycle]', fontsize=12)
    ax.set_ylabel('avg. latency [cycles]', fontsize=12)
    ax.set_ylim(0, 40) # Hard cap at 120 cycles just like the paper (40 * 500 = 20000 -> visible scope)
    
    # Adjust X-axis limits based on traffic type to mimic the paper's zoom
    if traffic == "bit_complement":
        ax.set_xlim(0.1, 0.32)
    elif traffic == "transpose":
        ax.set_xlim(0.1, 0.5)
    elif traffic == "uniform_random":
        ax.set_xlim(0.1, 0.56)
    elif traffic == "shuffle":
        ax.set_xlim(0.1, 0.52)

    ax.grid(True, linestyle='-', alpha=0.3)
    
    # Only put the legend in the top-left to avoid clutter
    ax.legend(loc='upper left', fontsize=10)

# Create plots directory if it doesn't exist
os.makedirs(plots_dir, exist_ok=True)

# Save the master plot
plot_path = os.path.join(plots_dir, 'DyXY_Plots.png')
plt.savefig(plot_path, dpi=300, bbox_inches='tight', facecolor='white')
print(f"Plot successfully generated and saved to: {plot_path}")