#!/bin/bash
echo "Starting Parallel Garnet NoC Sweep (Max 20 Concurrent Jobs)..."

# Configuration
MAX_JOBS=20
TRAFFICS=("uniform_random" "transpose" "bit_complement" "shuffle")
# "uniform_random" "transpose" "tornado" "bit_complement" "shuffle" "bursty"
ALGOS=(2 3 4 5)
ALGO_NAMES=("BOF" "FVC" "FON" "NOP")
RATES=$(seq -f "%.2f" 0.10 0.02 0.70)

# Ensure the results directory exists
mkdir -p ~/gem5/bofar_workspace/results
cd ~/gem5

for TRAFFIC in "${TRAFFICS[@]}"; do
    for i in "${!ALGOS[@]}"; do
        ALGO=${ALGOS[$i]}
        ALGO_NAME=${ALGO_NAMES[$i]}
        
        for RATE in $RATES; do
            OUTDIR="bofar_workspace/results/m5out_${TRAFFIC}_${ALGO_NAME}_${RATE}"
            
            echo "Queueing: $TRAFFIC | $ALGO_NAME | Rate: $RATE"
            
            # Run the specific standalone debug command in the background
            docker run --rm -v ~/gem5:/gem5 -w /gem5 gem5 \
              ./build/Garnet_standalone/gem5.debug --outdir=$OUTDIR \
              configs/example/garnet_synth_traffic.py \
              --network=garnet \
              --num-cpus=16 \
              --num-dirs=16 \
              --num-l2caches=16 \
              --sys-clock=2GHz \
              --topology=Mesh_XY \
              --mesh-rows=4 \
              --sim-cycles=50000000 \
              --routing-algorithm=$ALGO \
              --inj-vnet=0 \
              --vcs-per-vnet=4 \
              --buffers-per-data-vc=1 \
              --buffers-per-ctrl-vc=1 \
              --synthetic=$TRAFFIC \
              --injectionrate=$RATE > /dev/null 2>&1 &

            # --- THE BATCH LIMITER ---
            # Count active background jobs. If we hit the max, pause and wait for one to finish.
            while [ $(jobs -p | wc -l) -ge $MAX_JOBS ]; do
                sleep 2
            done
            
        done
    done
done

# Wait for the final batch of stragglers to finish
echo "All jobs queued. Waiting for the final batch to complete..."
wait

echo "Evaluation Complete! Data is ready in bofar_workspace/results/"
