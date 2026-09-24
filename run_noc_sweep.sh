#!/bin/bash
mkdir -p ~/gem5/noc_results_new
echo "Starting Garnet NoC Sweep..."

# The routing algorithm to test (XY is deterministic, Custom is adaptive)
ROUTING="XY" 
TRAFFIC="uniform_random"

# Sweep fine intervals from 0.01 to 0.019, then coarse intervals from 0.02 to 0.90
for RATE in $(seq -f "%.3f" 0.01 0.001 0.019) $(seq -f "%.2f" 0.02 0.02 0.70); do
    OUTDIR="noc_results_new/m5out_${TRAFFIC}_${ROUTING}_${RATE}"
    echo "Running: $TRAFFIC at $RATE inj_rate..."
    
    docker run --rm -v ~/gem5:/gem5 -w /gem5 gem5 \
      ./build/Garnet_standalone/gem5.debug --outdir=$OUTDIR \
      configs/example/garnet_synth_traffic.py \
      --network=garnet \
      --num-cpus=16 \
      --num-dirs=16 \
      --num-l2caches=16 \
      --sys-clock=2GHz \
      --num-l2caches=16 \
      --topology=Mesh_XY \
      --mesh-rows=4 \
      --sim-cycles=50000000 \
      --routing-algorithm=2 \
      --inj-vnet=0 \
      --synthetic=$TRAFFIC \
      --injectionrate=$RATE &
done

wait
echo "NoC Sweep Complete!"

