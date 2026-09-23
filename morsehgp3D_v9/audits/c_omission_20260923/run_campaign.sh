#!/usr/bin/env bash
# Auditeur C — campagne de la sonde d'omission (nice 19, 2 fils ; hote partage).
# Usage : run_campaign.sh <binaire> <dossier_coupes_lidar_8k> <sortie>
set -u
X=$1; D=$2; O=$3
mkdir -p "$O"
run() { local name=$1; shift; nice -n 19 "$X" "$@" > "$O/$name.txt" 2> "$O/$name.err"; echo "exit=$?" >> "$O/$name.txt"; }
for f in uniform terrain clusters; do run "${f}_8000_k5" family "$f" 8000 5 20 2; done
for s in s00 s01 s02; do run "lidar_${s}_8000_k5" file "$D/${s}_k5_s8_w8_r0_nested_8000.u32le" 5 20 2; done
run uniform_8000_k7 family uniform 8000 7 10 2
run lidar_s02_8000_k7 file "$D/s02_k5_s8_w8_r0_nested_8000.u32le" 7 10 2
run uniform_8000_k10 family uniform 8000 10 8 2
run lidar_s02_8000_k10 file "$D/s02_k5_s8_w8_r0_nested_8000.u32le" 10 8 2
touch "$O/DONE"
