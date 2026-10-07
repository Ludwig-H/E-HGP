#!/bin/bash
# L04 audit : mutant M18 (cmp_mag ignore le mot haut pour L >= 5) contre les portes d'oracle reduites.
SRC=/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10
W=/tmp/v11-audit/l04_code_fondations/mutants
rm -rf "$W/full_M18" "$W/build_M18"
mkdir -p "$W/full_M18"
cp -r "$SRC/src" "$SRC/cli" "$SRC/cmake" "$SRC/CMakeLists.txt" "$SRC/tests" "$SRC/reference" "$W/full_M18/"
cp "$W/m18/src/arith/wide.hpp" "$W/full_M18/src/arith/wide.hpp"
cmake -S "$W/full_M18" -B "$W/build_M18" -DCMAKE_BUILD_TYPE=Release > "$W/build_M18.log" 2>&1
cmake --build "$W/build_M18" -j3 --target mhgp10_catalogue mhgp10_tower >> "$W/build_M18.log" 2>&1 || { echo "M18 : ne compile pas"; exit 0; }
timeout 600 python3 -B "$W/full_M18/tests/oracle/test_catalogue_oracle.py" "$W/build_M18" 10 > "$W/oracle_cat_M18.out" 2>&1; rc1=$?
timeout 600 python3 -B "$W/full_M18/tests/oracle/test_tower_oracle.py" "$W/build_M18" 8 > "$W/oracle_tow_M18.out" 2>&1; rc2=$?
echo "M18_cmp_mag_mot_haut : oracle catalogue code=$rc1 ($(tail -1 "$W/oracle_cat_M18.out")) ; oracle tour code=$rc2 ($(tail -1 "$W/oracle_tow_M18.out"))"
# trame LiDAR : le catalogue mute differe-t-il du catalogue de base ? (K = 5, quart de trame)
Q=/workspaces/E-HGP/build/v10-g4-data-s1/lidar00_quarter_x_nonneg_y_nonneg.u32le
timeout 300 "$W/build_M18/mhgp10_tower" $Q --k=5 --threads=3 --dump="$W/dump_M18.txt" > "$W/dump_M18.json" 2>&1; rc3=$?
echo "M18 sur quart de trame (7069 points, K=5) : code=$rc3 sha256=$(sha256sum "$W/dump_M18.txt" 2>/dev/null | cut -c1-16) (base : b59443ab6f3bb2de)"
rm -rf "$W/full_M18" "$W/build_M18" "$W/dump_M18.txt"
echo FIN
