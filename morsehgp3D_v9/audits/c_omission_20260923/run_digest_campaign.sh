#!/usr/bin/env bash
# Auditeur C — retraits acceptes : la tour mutee differe-t-elle de la tour saine ?
# (condense FNV de tower_digest ; nice 19, 2 fils). Usage : <binaire> <coupes> <sortie>
set -u
X=$1; D=$2; O=$3
mkdir -p "$O"
run() { local name=$1; shift; nice -n 19 "$X" "$@" > "$O/$name.txt" 2> "$O/$name.err"; echo "exit=$?" >> "$O/$name.txt"; }
run uniform_8000_k5 family uniform 8000 5 20 2
for s in s00 s02; do run "lidar_${s}_8000_k5" file "$D/${s}_k5_s8_w8_r0_nested_8000.u32le" 5 20 2; done
run lidar_s02_8000_k10 file "$D/s02_k5_s8_w8_r0_nested_8000.u32le" 10 8 2
touch "$O/DONE"
