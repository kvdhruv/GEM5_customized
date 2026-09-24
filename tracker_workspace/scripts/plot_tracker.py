import os
import re
import matplotlib.pyplot as plt

# Dynamic Workspace Pathing (Safe inside and outside Docker)
script_dir = os.path.dirname(os.path.abspath(__file__))
base_dir = os.path.abspath(os.path.join(script_dir, '..'))

results_dir = os.path.join(base_dir, 'results')
plots_dir = os.path.join(base_dir, 'plots')
os.makedirs(plots_dir, exist_ok=True)

# Configuration mapping all 6 traffic patterns and 5 comparative algorithms
traffics = ["uniform_random", "transpose", "bit_complement", "bit_reverse"]
algo_names = ["FVC", "NOP", "FON", "BOF", "TRACKER"]

colors = {'FVC': 'green', 'NOP': 'blue', 'FON': 'darkorange', 'BOF': 'black', 'TRACKER': 'red'}
markers = {'FVC': '+', 'NOP': 'x', 'FON': 's', 'BOF': '^', 'TRACKER': 'o'}
labels = {'FVC': 'FVC', 'NOP': 'NOP', 'FON': 'FON', 'BOF': 'BOF', 'TRACKER': 'TRACKER (CFC)'}

# Match the 0.10 step increments present in your results folder (0.10 to 0.70)
rates = [f"{r/100:.2f}" for r in range(2, 42, 2)]

print(f"Scanning for data in {results_dir} and creating master plot...")

# Create the 2x3 subplot matrix to match your layout style
fig, axes = plt.subplots(2, 3, figsize=(18, 11))
fig.tight_layout(pad=6.0)

for idx, traffic in enumerate(traffics):
    ax = axes[idx // 3, idx % 3]
    any_data_for_subplot = False
    
    for algo in algo_names:
        latencies = []
        valid_rates = []
        
        for rate in rates:
            folder_name = f"m5out_{traffic}_{algo}_{rate}"
            stat_file = os.path.join(results_dir, folder_name, "stats.txt")
            
            if os.path.exists(stat_file):
                with open(stat_file, 'r') as f:
                    content = f.read()
                    # Using regex lookup logic to scan the files
                    match = re.search(r'system\.ruby\.network\.average_packet_latency\s+([0-9\.]+)', content)
                    
                    if match:
                        # Convert raw simulation ticks to cycles (1 cycle = 500 ticks at 2GHz)
                        cycles = float(match.group(1)) / 500.0
                        latencies.append(cycles)
                        valid_rates.append(float(rate))
                    else:
                        # Network is saturated or simulation was aborted; push line vertically off-grid
                        latencies.append(float('inf')) 
                        valid_rates.append(float(rate))
            else:
                # Folder or stat file missing implies saturation point was already exceeded
                latencies.append(float('inf'))
                valid_rates.append(float(rate))
        
        # Plot the extracted line data
        if any(l != float('inf') for l in latencies):
            # Sort vectors hand-in-hand to ensure clean line segments
            sorted_pairs = sorted(zip(valid_rates, latencies))
            plot_x, plot_y = zip(*sorted_pairs)
            
            ax.plot(plot_x, plot_y, 
                    marker=markers[algo], 
                    color=colors[algo], 
                    label=labels[algo], 
                    linewidth=2, 
                    markersize=6,
                    markeredgewidth=1.5)
            any_data_for_subplot = True

    # Format each subplot window to clean up text and axes
    clean_title = traffic.replace("_", " ").title()
    ax.set_title(f'{clean_title} Traffic', fontsize=13, fontweight='bold', pad=10)
    ax.set_xlabel('injection rate [packets per cycle]', fontsize=11)
    ax.set_ylabel('avg. latency [cycles]', fontsize=11)
    
    # Standard 0-40 cycle constraint used in the paper plots
    ax.set_ylim(0, 120) 
    
    # Keep X-limits structured cleanly to mirror your custom zoom configurations
    if traffic == "bit_complement":
        ax.set_xlim(0.02, 0.2)
    elif traffic == "transpose":
        ax.set_xlim(0.02, 0.4)
    elif traffic == "uniform_random":
        ax.set_xlim(0.02, 0.4)
    elif traffic == "shuffle":
        ax.set_xlim(0.02, 0.4)
    else: # tornado and bit_reverse
        ax.set_xlim(0.02, 0.4)

    ax.grid(True, which='both', linestyle=':', linewidth=0.75, color='gray', alpha=0.5)
    ax.set_axisbelow(True)
    ax.tick_params(axis='both', which='major', labelsize=10)
    
    # Place a clean legend on each plot if data exists to verify tracking order
    if any_data_for_subplot:
        ax.legend(loc='upper left', fontsize=9, frameon=True, facecolor='white', edgecolor='gainsboro')

# Save the finalized, comprehensive 2x3 matrix image
plot_path = os.path.join(plots_dir, 'TRACKER_Evaluation_Master_Plot.png')
plt.savefig(plot_path, dpi=300, bbox_inches='tight', facecolor='white')
plt.close()

print(f"\nSuccess! Master matrix plot successfully saved to: {plot_path}")