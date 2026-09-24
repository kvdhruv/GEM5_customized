import os
import re
import matplotlib.pyplot as plt

benchmarks = ['blackscholes', 'fluidanimate', 'canneal', 'bodytrack']
sizes_str = ['16KiB', '32KiB', '64KiB', '128KiB']
x_values = [16, 32, 64, 128] 

plt.figure(figsize=(10, 6))

for bench in benchmarks:
    miss_rates = []
    
    for size in sizes_str:
        # 1. Update the path to look inside the master folder
        stat_file = f"l1d_sweep_data/m5out_{bench}_{size}/stats.txt"
        total_accesses = 0
        total_misses = 0
        
        if os.path.exists(stat_file):
            with open(stat_file, 'r') as f:
                content = f.read()
                
                # Captures overallAccesses across all l1dcaches0 to l1dcaches15
                accesses = re.findall(r'l1dcaches\d+\.overallAccesses::total\s+(\d+)', content)
                
                # Captures overallMisses across all l1dcaches0 to l1dcaches15
                misses = re.findall(r'l1dcaches\d+\.overallMisses::total\s+(\d+)', content)
                
                total_accesses = sum(int(m) for m in accesses)
                total_misses = sum(int(m) for m in misses)
                
        if total_accesses > 0:
            miss_rate = (total_misses / total_accesses) * 100.0
        else:
            miss_rate = 0.0
            
        miss_rates.append(miss_rate)
        
    plt.plot(x_values, miss_rates, marker='o', linewidth=2, label=bench)

plt.title('16-Core L1 Data Cache Miss Rate vs. Size (PARSEC)')
plt.xlabel('L1 Data Cache Size (KiB)')
plt.ylabel('Global L1D Miss Rate (%)')
plt.xticks(x_values, sizes_str)
plt.legend()
plt.grid(True, linestyle='--', alpha=0.7)

# 2. Save the final graph inside the master folder
plt.savefig('l1d_sweep_data/l1d_miss_rate.png', dpi=300, bbox_inches='tight')

print("Graph successfully saved inside l1d_sweep_data/")
