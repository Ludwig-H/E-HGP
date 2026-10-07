#!/usr/bin/env bash
# Vidages locaux (hors depot) : ng00 a ng02, K5/16, K5/24, K10/24. Un fil, sequentiel.
set -u
here="$(cd "$(dirname "$0")/.." && pwd)"
data="${MHGP12_DATA:-/workspaces/E-HGP/build/v11-full-data-20261002}"
tool="${MHGP12_DUMP_TOOL:-$here/build/mhgp12_leaf_dump}"
out="${MHGP12_DUMPS:-$here/dumps}"
mkdir -p "$out"
for frame in ng00 ng01 ng02; do
  for cfg in "5 16" "5 24" "10 24"; do
    set -- $cfg
    name="${frame}_k$1_l$2"
    [ -s "$out/$name.bin" ] && [ -s "$out/$name.json" ] && continue
    "$tool" "$data/lidar_$frame.u32le" "$data/lidar_$frame.ids.u32le" "$1" "$2" "$out/$name.bin" > "$out/$name.json.part"
    rc=$?
    echo "$name rc=$rc"
    if [ $rc -eq 0 ]; then mv "$out/$name.json.part" "$out/$name.json"; fi
  done
done
echo fini > "$out/.done"
