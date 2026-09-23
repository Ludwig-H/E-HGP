#!/usr/bin/env bash
# Auditeur C — campagne du juge d'echantillon q2 (nice 19, 2 fils ; hote partage).
# Usage : run_q2_judge.sh <binaire> <dossier_coupes_lidar> <sortie>
set -u
X=$1; D=$2; O=$3
mkdir -p "$O"
run() { local name=$1; shift; nice -n 19 "$X" "$@" > "$O/$name.txt" 2> "$O/$name.err"; echo "exit=$?" >> "$O/$name.txt"; }
for s in s00 s01 s02; do run "q2_lidar_${s}_8000_k10" file "$D/${s}_k5_s8_w8_r0_nested_8000.u32le" 10 1000 2; done
run q2_lidar_s02_8000_k5 file "$D/s02_k5_s8_w8_r0_nested_8000.u32le" 5 1000 2
run q2_uniform_8000_k10 family uniform 8000 10 500 2
run q2_lidar_scene00_full_k10 file "$D/scene_00_full.u32le" 10 200 2
touch "$O/DONE_Q2"
