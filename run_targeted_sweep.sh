#!/bin/bash
mkdir -p ~/gem5/l1d_sweep_data
echo "Starting PARALLEL Targeted PARSEC Runs..."

# 1. Launch Canneal at 32KiB in the background
echo "Launching: canneal at 32KiB..."
docker run --rm -v ~/gem5:/gem5 -v ~/gem5full:/gem5_fs -w /gem5 gem5 \
  ./build/X86/gem5.opt --outdir=l1d_sweep_data/m5out_canneal_32KiB parsec_l1d_sweep.py --benchmark=canneal --l1d_size=32KiB &

# 2. Launch Canneal at 128KiB in the background (The missing run!)
echo "Launching: canneal at 128KiB..."
docker run --rm -v ~/gem5:/gem5 -v ~/gem5full:/gem5_fs -w /gem5 gem5 \
  ./build/X86/gem5.opt --outdir=l1d_sweep_data/m5out_canneal_128KiB parsec_l1d_sweep.py --benchmark=canneal --l1d_size=128KiB &

# 3. Launch all Bodytrack sizes in the background simultaneously
for SIZE in "16KiB" "32KiB" "64KiB" "128KiB"; do
    echo "Launching: bodytrack at $SIZE..."
    docker run --rm -v ~/gem5:/gem5 -v ~/gem5full:/gem5_fs -w /gem5 gem5 \
      ./build/X86/gem5.opt --outdir=l1d_sweep_data/m5out_bodytrack_${SIZE} parsec_l1d_sweep.py --benchmark=bodytrack --l1d_size=$SIZE &
done

echo "All 6 simulations have been launched in parallel!"
echo "Waiting for all of them to finish..."

# The wait command pauses the script here until all background jobs (&) complete
wait 

echo "Targeted parallel runs complete!"
