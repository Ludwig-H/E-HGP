#!/usr/bin/env bash
# Mesure locale de la passe 1 de la validation FULL : variantes compilees (complete, sans puissances, sans support,
# sans les deux), 08/000000 K5 et K10, W8, deux repetitions entrelacees. Sorties JSON de la sonde.
set -u
SP=/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad
SRC=$SP/seal_src/morsehgp3D_v9
IN=/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le
OUT=$SP/seal_out; mkdir -p $OUT
declare -A FLAGS=([full]="" [nopow]="-DMHGP9_PASS1_SKIP_POWERS" [nosup]="-DMHGP9_PASS1_SKIP_SUPPORT" [none]="-DMHGP9_PASS1_SKIP_POWERS -DMHGP9_PASS1_SKIP_SUPPORT")
for v in full nopow nosup none; do
  cmake -S $SRC -B $SP/seal_build_$v -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr "-DCMAKE_CXX_FLAGS=${FLAGS[$v]}" > $OUT/cfg_$v.log 2>&1 || { echo "cfg $v failed"; exit 1; }
  cmake --build $SP/seal_build_$v --parallel 8 --target mhgp9_tower_probe > $OUT/build_$v.log 2>&1 || { echo "build $v failed"; exit 1; }
done
for r in 1 2; do for K in 5 10; do for v in full nopow nosup none; do
  $SP/seal_build_$v/mhgp9_tower_probe $IN $K 8 --catalogue-digest > $OUT/${v}_k${K}_r${r}.json 2> $OUT/${v}_k${K}_r${r}.err
  echo "$v K$K r$r rc=$?"
done; done; done
touch $OUT/DONE
