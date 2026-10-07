#!/bin/bash
# Echantillon de boules du dump canonique (trame 02, K = 10) : tirage de Bernoulli 0,0004 a graine fixe, en flux.
cd /tmp/v11-audit/l01_math_catalogue || exit 1
rm -f j1/fifo; mkfifo j1/fifo
awk 'BEGIN{srand(20261002)} rand() < 0.0004' < j1/fifo > j1/lidar02_k10_echantillon.txt &
nice -n 5 build/mhgp10_catalogue /workspaces/E-HGP/build/v10-g4-data-s1/lidar02_full.u32le --k=10 --threads=3 --dump=j1/fifo > j1/lidar02_k10.json
wait
rm -f j1/fifo
wc -l j1/lidar02_k10_echantillon.txt
