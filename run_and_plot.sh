#!/bin/bash

DATA_FILE="my_gem5_data.txt"
echo "rate latency" > $DATA_FILE

echo "Extracting data from stats.txt files..."
# Loop through your fine and coarse injection rates
for rate in $(seq -f "%.3f" 0.01 0.001 0.019) $(seq -f "%.2f" 0.02 0.02 0.90); do
    stat_file="noc_results_new/m5out_uniform_random_XY_${rate}/stats.txt"
    
    if [ -f "$stat_file" ]; then
        # Grep the ticks and convert to cycles
        ticks=$(grep -w "system.ruby.network.average_packet_latency" "$stat_file" | awk '{print $2}')
        if [ ! -z "$ticks" ]; then
            cycles=$(echo "scale=3; $ticks / 1000" | bc)
            echo "$rate $cycles" >> $DATA_FILE
        fi
    fi
done

echo "Generating graph with gnuplot..."

# Feed instructions directly into gnuplot
gnuplot <<- EOF
    set terminal pngcairo size 1000,600 enhanced font 'Arial,12'
    set output 'noc_gnuplot_curve.png'
    
    set title '4x4 Mesh NoC Latency vs. Injection Rate'
    set xlabel 'Injection Rate (flits/node/cycle)'
    set ylabel 'Average Packet Latency (cycles)'
    
    set grid
    
    # Cap the axes to reveal the baseline and saturation knee
    set xrange [0:0.9]
    set yrange [0:20]
    
    # Plot the data, skipping the first header line
    plot '$DATA_FILE' skip 1 using 1:2 with linespoints \
         linewidth 2 pointtype 7 pointsize 1.5 linecolor rgb 'blue' \
         title 'XY Routing'
EOF

echo "Done! Graph instantly saved as noc_gnuplot_curve.png"
