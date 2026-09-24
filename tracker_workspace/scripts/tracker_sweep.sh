#!/bin/bash
echo "Starting TRACKER Sweep..."

MAX_JOBS=10
TRAFFICS=("uniform_random"  "bit_complement" "transpose" "bit_reverse")
# "tornado"
ALGOS=( 6) 
ALGO_NAMES=( "TRACKER")
# "BOF" "FVC" "FON" "NOP"
RATES=$(seq -f "%.2f" 0.02 0.02 0.4)

cd ~/gem5

for TRAFFIC in "${TRAFFICS[@]}"; do
    for i in "${!ALGOS[@]}"; do
        ALGO=${ALGOS[$i]}
        ALGO_NAME=${ALGO_NAMES[$i]}
        
        for RATE in $RATES; do
            OUTDIR="/gem5/tracker_workspace/results/m5out_${TRAFFIC}_${ALGO_NAME}_${RATE}"
            
            echo "Queueing: $TRAFFIC | $ALGO_NAME | Rate: $RATE"
            
            docker run --rm -v ~/gem5:/gem5 -w /gem5 gem5 \
              ./build/Garnet_standalone/gem5.debug --outdir=$OUTDIR \
              configs/example/garnet_synth_traffic.py \
              --network=garnet \
              --num-cpus=16 \
              --num-dirs=16 \
              --sys-clock=2GHz \
              --topology=Mesh_XY \
              --mesh-rows=4 \
              --sim-cycles=5000000 \
              --routing-algorithm=$ALGO \
              --synthetic=$TRAFFIC \
              --injectionrate=$RATE \
              --vcs-per-vnet=4 > /dev/null 2>&1 &

            while [ $(jobs -p | wc -l) -ge $MAX_JOBS ]; do
                sleep 2
            done
        done
    done
done

wait
echo "Evaluation Complete! Check tracker_workspace/results/"