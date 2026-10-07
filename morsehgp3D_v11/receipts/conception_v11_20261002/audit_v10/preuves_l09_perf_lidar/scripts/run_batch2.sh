#!/usr/bin/env bash
# Lot 2 : sondes instrumentees (hors depot) sur les trois trames, puis compteurs sur les scenes des demos
# (67k a 76k sites sans sol, 126k avec sol). Lecture seule des entrees ; aucune coordonnee n'est recopiee.
set -u
I=/tmp/v11-audit/l09_perf_lidar/instr
B=/tmp/v11-audit/l09_perf_lidar/build-rel
D=/workspaces/E-HGP/build/v10-g4-data-s1
S=/workspaces/E-HGP/build/v10-lidar-demos/_cache/scenes
O=/tmp/v11-audit/l09_perf_lidar/runs2
mkdir -p "$O"
date -u +"debut %Y-%m-%dT%H:%M:%SZ" > "$O/journal.txt"
run() {
  local name=$1; shift
  /usr/bin/time -v -o "$O/$name.time" "$@" > "$O/$name.json" 2> "$O/$name.err"
  echo "$name code=$? $(cat /proc/loadavg)" >> "$O/journal.txt"
}
# 1. repartition TSC du catalogue (1 fil) et compteurs de moments (2 fils), tour TSC (1 fil)
for f in 00 01 02; do
  for k in 5 10; do
    run "tsc_l${f}_k${k}_w1" "$I/build-tsc/mhgp10_catalogue" "$D/lidar${f}_full.u32le" --k=$k --threads=1
    run "mom_l${f}_k${k}_w3" "$I/build-mom/mhgp10_catalogue" "$D/lidar${f}_full.u32le" --k=$k --threads=3
    run "ttsc_l${f}_k${k}_w1" "$I/build-ttsc/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=$k --threads=1 --no-points
  done
done
# 2. scenes des demos : compteurs (binaire de reference), 4 fils
for s in 03_pieton_contre_facade 01_velos_en_rang 05_temoin_voitures_en_file 02_velos_contre_facade; do
  for k in 5 10; do
    run "demo_${s}_k${k}_cat_w4" "$B/mhgp10_catalogue" "$S/$s/sites.u32le" --k=$k --threads=4
    run "demo_${s}_k${k}_tow_w4" "$B/mhgp10_tower" "$S/$s/sites.u32le" --k=$k --threads=4 --no-points
  done
done
run "demo_04_avec_sol_k5_cat_w4" "$B/mhgp10_catalogue" "$S/04_velos_en_rang_avec_sol/sites.u32le" --k=5 --threads=4
run "demo_04_avec_sol_k5_tow_w4" "$B/mhgp10_tower" "$S/04_velos_en_rang_avec_sol/sites.u32le" --k=5 --threads=4 --no-points
run "demo_04_avec_sol_k10_cat_w3" "$B/mhgp10_catalogue" "$S/04_velos_en_rang_avec_sol/sites.u32le" --k=10 --threads=3
date -u +"fin %Y-%m-%dT%H:%M:%SZ" >> "$O/journal.txt"
touch "$O/DONE"
