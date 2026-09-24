import subprocess
import itertools
import multiprocessing
import os

benchmarks = ['blackscholes', 'fluidanimate', 'canneal', 'dedup']
l1d_sizes = ['16KiB', '32KiB', '64KiB', '128KiB']

def run_simulation(params):
    benchmark, size = params
    
    # 1. Nest the output folder inside the master directory
    out_dir = f"l1d_sweep_data/m5out_{benchmark}_{size}"
    
    command = [
        "./build/X86/gem5.opt",
        f"--outdir={out_dir}",
        "parsec_l1d_sweep.py",
        f"--benchmark={benchmark}",
        f"--l1d_size={size}"
    ]
    
    print(f"[*] Starting {benchmark} with {size} L1D...")
    subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return f"[+] Finished {benchmark} with {size} L1D!"

if __name__ == "__main__":
    # 2. Create the master folder if it doesn't exist yet
    os.makedirs("l1d_sweep_data", exist_ok=True)
    
    tasks = list(itertools.product(benchmarks, l1d_sizes))
    print(f"Total 16-core simulations to run: {len(tasks)}")
    
    with multiprocessing.Pool(processes=2) as pool:
        results = pool.map(run_simulation, tasks)
        
    for res in results:
        print(res)
        
    print("Sweep complete. Handing over to the plotter...")
