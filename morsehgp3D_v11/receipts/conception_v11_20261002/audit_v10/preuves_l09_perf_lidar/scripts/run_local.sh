#!/usr/bin/env bash
# Mesures locales L09 : compteurs deterministes et repartition relative, 1/2/4 fils, trois trames sans sol.
# Lecture seule des entrees ; sorties sous /tmp/v11-audit/l09_perf_lidar/runs/.
set -u
B=/tmp/v11-audit/l09_perf_lidar/build-rel
D=/workspaces/E-HGP/build/v10-g4-data-s1
O=/tmp/v11-audit/l09_perf_lidar/runs
date -u +"debut %Y-%m-%dT%H:%M:%SZ" > "$O/journal.txt"
cat /proc/loadavg >> "$O/journal.txt"
run() {  # nom, commande...
  local name=$1; shift
  /usr/bin/time -v -o "$O/$name.time" "$@" > "$O/$name.json" 2> "$O/$name.err"
  echo "$name code=$? $(cat /proc/loadavg)" >> "$O/journal.txt"
}
for f in 00 01 02; do
  for k in 5 10; do
    for w in 1 4; do
      run "cat_l${f}_k${k}_w${w}" "$B/mhgp10_catalogue" "$D/lidar${f}_full.u32le" --k=$k --threads=$w
    done
    for w in 1 2 4; do
      run "tow_l${f}_k${k}_w${w}_r2" "$B/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=$k --threads=$w --no-points --repeat=2
    done
    run "tow1_l${f}_k${k}_w4" "$B/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=$k --threads=4 --no-points --repeat=1
    run "towcover_l${f}_k${k}_w4" "$B/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=$k --threads=4 --entry=cover --repeat=1
    run "towcore_l${f}_k${k}_w4" "$B/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=$k --threads=4 --entry=core --repeat=1
  done
done
date -u +"fin %Y-%m-%dT%H:%M:%SZ" >> "$O/journal.txt"
touch "$O/DONE"
