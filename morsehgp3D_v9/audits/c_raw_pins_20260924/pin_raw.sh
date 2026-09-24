#!/usr/bin/env bash
# Epingles CPU des trames brutes (sol compris) b00/b01/b02 de R21 : bras moteur et bras par lots CPU (jumeaux hote
# des chemins GPU), K5 et K10, s8, W8. Base : origin/main courant. Sorties JSON de la sonde (condense du catalogue).
set -u
SP=/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad
W=/workspaces/E-HGP/build/v9-audit-c-publish
BASE=$(git -C $W rev-parse origin/main)
OUT=$SP/pinraw_out; mkdir -p $OUT; echo "base=$BASE" > $OUT/BASE.txt
mkdir -p $SP/pinraw_src && git -C $W archive $BASE morsehgp3D_v9 | tar -x -C $SP/pinraw_src
cmake -S $SP/pinraw_src/morsehgp3D_v9 -B $SP/pinraw_build -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr > $OUT/cfg.log 2>&1 || { echo cfg_failed; exit 1; }
cmake --build $SP/pinraw_build --parallel 8 --target mhgp9_tower_probe > $OUT/build.log 2>&1 || { echo build_failed; exit 1; }
sha256sum $SP/pinraw_build/mhgp9_tower_probe > $OUT/probe.sha256
R=$W/morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i
declare -A F=([b00]=scene_00_000000_grid [b01]=scene_01_000100_grid [b02]=scene_02_000200_grid)
BATCH="--lever=q34_batch_filter=1 --lever=q34_batch_certificates=1 --lever=q34_batch_q3=1 --lever=q34_batch_q4=1"
for arm in engine batch; do for s in b00 b01 b02; do for K in 5 10; do
  L=""; [ $arm = batch ] && L="$BATCH"
  /usr/bin/time -v $SP/pinraw_build/mhgp9_tower_probe $R/${F[$s]}/full.u32le $K 8 --catalogue-digest $L > $OUT/${s}_k${K}_${arm}.json 2> $OUT/${s}_k${K}_${arm}.time
  echo "$arm $s K$K rc=$?"
done; done; done
touch $OUT/DONE
