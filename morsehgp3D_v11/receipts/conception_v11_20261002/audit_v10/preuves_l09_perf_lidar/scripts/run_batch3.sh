#!/usr/bin/env bash
# Lot 3 : controle independant de la composition « vague 1 » du juge (binaires hors depot, lecture seule) contre la
# base (construction neuve du HEAD, sha256 du catalogue identique a la base du juge), sur le chemin FULL sans attaches.
set -u
B=/tmp/v11-audit/l09_perf_lidar/build-rel
C=/workspaces/E-HGP/build/v10-perf/juge/composition/bin
D=/workspaces/E-HGP/build/v10-g4-data-s1
O=/tmp/v11-audit/l09_perf_lidar/runs3
mkdir -p "$O"
date -u +"debut %Y-%m-%dT%H:%M:%SZ" > "$O/journal.txt"
run() {
  local name=$1; shift
  /usr/bin/time -v -o "$O/$name.time" "$@" > "$O/$name.json" 2> "$O/$name.err"
  echo "$name code=$? $(cat /proc/loadavg)" >> "$O/journal.txt"
}
# 1. identite de la tour FULL (dump complet) : trame 01, K = 5 et K = 10 ; trame 02, K = 5
for spec in "01 5" "02 5" "01 10"; do
  set -- $spec; f=$1; k=$2
  "$B/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=$k --threads=4 --no-points --dump="$O/dump_base_l${f}_k${k}.txt" > "$O/dumprun_base_l${f}_k${k}.json" 2>&1
  "$C/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=$k --threads=4 --no-points --dump="$O/dump_comp_l${f}_k${k}.txt" > "$O/dumprun_comp_l${f}_k${k}.json" 2>&1
  sha256sum "$O/dump_base_l${f}_k${k}.txt" "$O/dump_comp_l${f}_k${k}.txt" >> "$O/dumps.sha256"
  wc -l "$O/dump_base_l${f}_k${k}.txt" >> "$O/dumps.sha256"
  rm -f "$O/dump_base_l${f}_k${k}.txt" "$O/dump_comp_l${f}_k${k}.txt"
  echo "dump l$f k$k $(cat /proc/loadavg)" >> "$O/journal.txt"
done
# 2. temps CPU en alternance ABBA, 2 fils, FULL sans attaches, une passe
for f in 00 01 02; do
  for r in 1 2; do
    run "A_l${f}_k5_r${r}" "$B/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=5 --threads=2 --no-points
    run "B_l${f}_k5_r${r}" "$C/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=5 --threads=2 --no-points
    run "B2_l${f}_k5_r${r}" "$C/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=5 --threads=2 --no-points
    run "A2_l${f}_k5_r${r}" "$B/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=5 --threads=2 --no-points
  done
done
for f in 01 02; do
  run "A_l${f}_k10_r1" "$B/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=10 --threads=3 --no-points
  run "B_l${f}_k10_r1" "$C/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=10 --threads=3 --no-points
  run "B2_l${f}_k10_r1" "$C/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=10 --threads=3 --no-points
  run "A2_l${f}_k10_r1" "$B/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=10 --threads=3 --no-points
done
# 3. levier ISA : construction x86-64-v3 du HEAD contre la construction par defaut, 1 fil, alternance ABBA
V=/tmp/v11-audit/l09_perf_lidar/build-v3
for spec in "01 5" "02 5" "01 10"; do
  set -- $spec; f=$1; k=$2
  run "isaA_l${f}_k${k}" "$B/mhgp10_catalogue" "$D/lidar${f}_full.u32le" --k=$k --threads=1
  run "isaB_l${f}_k${k}" "$V/mhgp10_catalogue" "$D/lidar${f}_full.u32le" --k=$k --threads=1
  run "isaB2_l${f}_k${k}" "$V/mhgp10_catalogue" "$D/lidar${f}_full.u32le" --k=$k --threads=1
  run "isaA2_l${f}_k${k}" "$B/mhgp10_catalogue" "$D/lidar${f}_full.u32le" --k=$k --threads=1
done
date -u +"fin %Y-%m-%dT%H:%M:%SZ" >> "$O/journal.txt"
touch "$O/DONE"
