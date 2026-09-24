#!/bin/bash
echo "Initiating Bursty Hardware Trojan Attack on Node 5..."

# Define where the stats.txt file will be saved
OUTDIR="trojan_workspace/results/trojan_attack"
mkdir -p $OUTDIR

# Run the simulation
docker run --rm --user $(id -u):$(id -g) -v ~/gem5:/gem5 -w /gem5 gem5 \
  ./build/Garnet_standalone/gem5.debug --outdir=$OUTDIR \
  configs/example/garnet_synth_traffic.py \
  --network=garnet \
  --num-cpus=16 \
  --num-dirs=16 \
  --topology=Mesh_XY \
  --mesh-rows=4 \
  --sim-cycles=5000000 \
  --routing-algorithm=6 \
  --synthetic=uniform_random \
  --injectionrate=0.02 \
  --vcs-per-vnet=4 \
  --trojan-id=5  
  
echo "Attack Complete. Data logged to $OUTDIR/stats.txt"
