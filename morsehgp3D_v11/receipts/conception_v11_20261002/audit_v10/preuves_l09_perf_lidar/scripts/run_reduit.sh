#!/usr/bin/env bash
# Plan reduit (charge de la machine > 40) : scenes des demos, composition du juge, ISA, complements TSC.
set -u
I=/tmp/v11-audit/l09_perf_lidar/instr
B=/tmp/v11-audit/l09_perf_lidar/build-rel
V=/tmp/v11-audit/l09_perf_lidar/build-v3
C=/workspaces/E-HGP/build/v10-perf/juge/composition/bin
D=/workspaces/E-HGP/build/v10-g4-data-s1
S=/workspaces/E-HGP/build/v10-lidar-demos/_cache/scenes
O2=/tmp/v11-audit/l09_perf_lidar/runs2
O3=/tmp/v11-audit/l09_perf_lidar/runs3
mkdir -p "$O2" "$O3"
J=/tmp/v11-audit/l09_perf_lidar/runs3/journal.txt
date -u +"debut %Y-%m-%dT%H:%M:%SZ" > "$J"
run() {  # dossier, nom, commande...
  local dir=$1 name=$2; shift 2
  /usr/bin/time -v -o "$dir/$name.time" "$@" > "$dir/$name.json" 2> "$dir/$name.err"
  echo "$name code=$? $(cat /proc/loadavg)" >> "$J"
}
# A1. scenes des demos, K = 5 (catalogue seul, 3 fils)
for s in 03_pieton_contre_facade 01_velos_en_rang 05_temoin_voitures_en_file 02_velos_contre_facade; do
  run "$O2" "demo_${s}_k5_cat_w4" "$B/mhgp10_catalogue" "$S/$s/sites.u32le" --k=5 --threads=3
done
run "$O2" "demo_04_avec_sol_k5_cat_w4" "$B/mhgp10_catalogue" "$S/04_velos_en_rang_avec_sol/sites.u32le" --k=5 --threads=3
run "$O2" "demo_02_velos_contre_facade_k5_tow_w4" "$B/mhgp10_tower" "$S/02_velos_contre_facade/sites.u32le" --k=5 --threads=3 --no-points
# B. composition du juge : identite du dump FULL (trame 01, K = 5), puis temps CPU ABBA
"$B/mhgp10_tower" "$D/lidar01_full.u32le" --k=5 --threads=3 --no-points --dump="$O3/dump_base_l01_k5.txt" > "$O3/dumprun_base_l01_k5.json" 2>&1
"$C/mhgp10_tower" "$D/lidar01_full.u32le" --k=5 --threads=3 --no-points --dump="$O3/dump_comp_l01_k5.txt" > "$O3/dumprun_comp_l01_k5.json" 2>&1
sha256sum "$O3/dump_base_l01_k5.txt" "$O3/dump_comp_l01_k5.txt" > "$O3/dumps.sha256"
wc -l "$O3/dump_base_l01_k5.txt" >> "$O3/dumps.sha256"
rm -f "$O3/dump_base_l01_k5.txt" "$O3/dump_comp_l01_k5.txt"
echo "dumps $(cat /proc/loadavg)" >> "$J"
for f in 01 02; do
  for r in 1 2; do
    run "$O3" "A_l${f}_k5_r${r}" "$B/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=5 --threads=2 --no-points
    run "$O3" "B_l${f}_k5_r${r}" "$C/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=5 --threads=2 --no-points
    run "$O3" "B2_l${f}_k5_r${r}" "$C/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=5 --threads=2 --no-points
    run "$O3" "A2_l${f}_k5_r${r}" "$B/mhgp10_tower" "$D/lidar${f}_full.u32le" --k=5 --threads=2 --no-points
  done
done
run "$O3" "A_l01_k10_r1" "$B/mhgp10_tower" "$D/lidar01_full.u32le" --k=10 --threads=3 --no-points
run "$O3" "B_l01_k10_r1" "$C/mhgp10_tower" "$D/lidar01_full.u32le" --k=10 --threads=3 --no-points
run "$O3" "B2_l01_k10_r1" "$C/mhgp10_tower" "$D/lidar01_full.u32le" --k=10 --threads=3 --no-points
run "$O3" "A2_l01_k10_r1" "$B/mhgp10_tower" "$D/lidar01_full.u32le" --k=10 --threads=3 --no-points
# C. ISA x86-64-v3, 1 fil
run "$O3" "isaA_l01_k5" "$B/mhgp10_catalogue" "$D/lidar01_full.u32le" --k=5 --threads=1
run "$O3" "isaB_l01_k5" "$V/mhgp10_catalogue" "$D/lidar01_full.u32le" --k=5 --threads=1
run "$O3" "isaB2_l01_k5" "$V/mhgp10_catalogue" "$D/lidar01_full.u32le" --k=5 --threads=1
run "$O3" "isaA2_l01_k5" "$B/mhgp10_catalogue" "$D/lidar01_full.u32le" --k=5 --threads=1
run "$O3" "isaA_l01_k10" "$B/mhgp10_catalogue" "$D/lidar01_full.u32le" --k=10 --threads=1
run "$O3" "isaB_l01_k10" "$V/mhgp10_catalogue" "$D/lidar01_full.u32le" --k=10 --threads=1
# D. complements TSC (trame 02)
run "$O2" "tsc_l02_k5_w1" "$I/build-tsc/mhgp10_catalogue" "$D/lidar02_full.u32le" --k=5 --threads=1
run "$O2" "tsc_l02_k10_w1" "$I/build-tsc/mhgp10_catalogue" "$D/lidar02_full.u32le" --k=10 --threads=1
run "$O2" "ttsc_l02_k10_w1" "$I/build-ttsc/mhgp10_tower" "$D/lidar02_full.u32le" --k=10 --threads=1 --no-points
run "$O2" "mom_l02_k5_w3" "$I/build-mom/mhgp10_catalogue" "$D/lidar02_full.u32le" --k=5 --threads=3
# A2. scenes des demos, K = 10, puis trame avec sol
run "$O2" "demo_02_velos_contre_facade_k10_cat_w4" "$B/mhgp10_catalogue" "$S/02_velos_contre_facade/sites.u32le" --k=10 --threads=3
run "$O2" "demo_01_velos_en_rang_k10_cat_w4" "$B/mhgp10_catalogue" "$S/01_velos_en_rang/sites.u32le" --k=10 --threads=3
run "$O2" "demo_04_avec_sol_k10_cat_w3" "$B/mhgp10_catalogue" "$S/04_velos_en_rang_avec_sol/sites.u32le" --k=10 --threads=3
date -u +"fin %Y-%m-%dT%H:%M:%SZ" >> "$J"
touch "$O3/DONE"
