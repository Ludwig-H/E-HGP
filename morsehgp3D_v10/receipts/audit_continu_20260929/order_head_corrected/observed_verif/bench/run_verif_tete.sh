#!/bin/bash
# Campagne d'identite de la tete (verificateur adverse) : BIN = verif_tete_<variante>
V=/workspaces/E-HGP/build/v10-perf/ordre_tete-verif
BIN=$1; OUT=$2
L=/workspaces/E-HGP/build/v10-scale-inputs; I=$V/inputs; D=/workspaces/E-HGP/build/v10-perf/ordre_tete/out/degen
echo "debut $(date -Is) charge $(cut -d' ' -f1-3 /proc/loadavg) binaire $(sha256sum $BIN | cut -c1-16)" > $OUT
run() { r=$(timeout 3000 $BIN "$@" 2>&1); c=$?; echo "$(basename $1) K=$2 $3 exit=$c vrais=$(echo "$r" | grep -c 'identique=true') | $(echo "$r" | grep -v 'identique=true' | tr '\n' ' ' | cut -c1-400)" >> $OUT; }
for f in grid12s3 plan40 coquilles quasi_sphere; do for K in 1 3 5 10; do for e in cover core; do run $I/$f.u32le $K $e; done; done; done
for f in grid10 sphere101; do for K in 5 10; do for e in cover core; do run $D/$f.u32le $K $e; done; done; done
run $L/syn_shells_density_x1.u32le 5 cover; run $L/syn_shells_density_x1.u32le 8 core
run $L/syn_filaments_space_x1.u32le 10 cover; run $L/syn_clusters_density_x1.u32le 3 core
run $L/lidar01_quarter_x_neg_y_neg.u32le 10 core; run $L/lidar01_quarter_x_neg_y_neg.u32le 5 cover
run $L/lidar02_full.u32le 5 cover 1:3 89:3 200:3 1000:6 50:1
run $L/lidar00_full.u32le 5 core 200:3 2:1
run $L/lidar02_full.u32le 10 cover 200:3 89:3
echo "fin $(date -Is) charge $(cut -d' ' -f1-3 /proc/loadavg)" >> $OUT
echo FIN >> $OUT
