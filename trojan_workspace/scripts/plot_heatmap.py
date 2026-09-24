import re
import os
import math
import argparse
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns


parser = argparse.ArgumentParser(description="Generate Dynamic NoC Congestion Heatmap")
parser.add_argument('--rows', type=int, default=0, help="Force specific number of rows")
parser.add_argument('--cols', type=int, default=0, help="Force specific number of columns")
args = parser.parse_args()


script_dir = os.path.dirname(os.path.abspath(__file__))
base_dir = os.path.abspath(os.path.join(script_dir, '..'))

STATS_FILE = os.path.join(base_dir, 'results', 'trojan_attack', 'stats.txt')
PLOT_OUTPUT = os.path.join(base_dir, 'plots', 'trojan_heatmap.png')

os.makedirs(os.path.dirname(PLOT_OUTPUT), exist_ok=True)


buffer_data = {}

print(f"[*] Parsing {STATS_FILE} for router buffer reads...")

try:
    with open(STATS_FILE, 'r') as file:
        for line in file:
            # Match exactly: system.ruby.network.routersXX.buffer_reads   YYYY
            match = re.search(r'system\.ruby\.network\.routers(\d+)\.buffer_reads\s+(\d+)', line)
            if match:
                router_id = int(match.group(1))
                buffer_reads = int(match.group(2))
                buffer_data[router_id] = buffer_reads
except FileNotFoundError:
    print(f"\n[!] ERROR: Could not find stats.txt at {STATS_FILE}")
    exit(1)

if not buffer_data:
    print("\n[!] ERROR: No router buffer data found in stats.txt.")
    exit(1)

# Auto-detect total routers (highest ID + 1)
total_routers = max(buffer_data.keys()) + 1

# Auto-detect mesh dimensions
if args.rows > 0 and args.cols > 0:
    mesh_rows = args.rows
    mesh_cols = args.cols
else:
    # Assume a square mesh (e.g., 16 -> 4x4, 64 -> 8x8)
    mesh_rows = int(math.sqrt(total_routers))
    mesh_cols = total_routers // mesh_rows

print(f"[*] Detected {total_routers} routers. Generating a {mesh_cols}x{mesh_rows} Heatmap.")

if mesh_rows * mesh_cols != total_routers:
    print(f"[!] WARNING: Grid size ({mesh_cols}x{mesh_rows}) does not match total routers ({total_routers}).")

# Populate the dynamic grid
congestion_grid = np.zeros(mesh_rows * mesh_cols)
for rid, reads in buffer_data.items():
    if rid < len(congestion_grid):
        congestion_grid[rid] = reads


# Reshape the 1D array into a 2D matrix
heatmap_data = congestion_grid.reshape((mesh_rows, mesh_cols))

# Auto-detect the Trojan Node (the node with the absolute highest congestion)
trojan_node_id = np.argmax(congestion_grid)

plt.figure(figsize=(10, 8))
# Capture the axis object returned by seaborn
ax = sns.heatmap(heatmap_data, annot=True, fmt=".0f", cmap="YlOrRd", 
            linewidths=1, linecolor='black', square=True,
            cbar_kws={'label': 'Total Buffer Reads (Congestion)'})

# --- INVERT THE Y-AXIS TO MATCH CARTESIAN/HARDWARE LAYOUT ---
ax.invert_yaxis()


plt.title(f"NoC Congestion Heatmap: {mesh_cols}x{mesh_rows} Mesh\n(Estimated Attack Origin: Node {trojan_node_id})", fontsize=14, pad=15)
plt.xlabel("Mesh X-Coordinate", fontsize=12)
plt.ylabel("Mesh Y-Coordinate", fontsize=12)

# Save the plot
plt.savefig(PLOT_OUTPUT, dpi=300, bbox_inches='tight')
print(f"[*] Success! Dynamic Heatmap saved to {PLOT_OUTPUT}")