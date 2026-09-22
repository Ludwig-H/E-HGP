#!/usr/bin/env bash
# Campagne de la premiere tour v9 : trois trames sans sol a 1 mm, K5 puis K10,
# huit fils. Lance depuis la racine du depot ; ecrit dans raw/ a cote du script.
set -u
here="$(cd "$(dirname "$0")" && pwd)"
probe="${PROBE:-build/v9/mhgp9_tower_probe}"
base=morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6
{
  echo "commit=$(git rev-parse HEAD)"
  echo "probe_sha256=$(sha256sum "$probe" | cut -d' ' -f1)"
  echo "host_cpu=$(grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 | xargs)"
  echo "nproc=$(nproc)"
  for s in 00 01 02; do echo "input_$s=$(sha256sum $base/scene_${s}_grid/full.u32le | cut -d' ' -f1)"; done
} > "$here/raw/context.txt"
for k in 5 10; do
  for s in 00 01 02; do
    tag="s${s}_k${k}_w8"
    uptime > "$here/raw/$tag.load_before"
    /usr/bin/time -v "$probe" "$base/scene_${s}_grid/full.u32le" "$k" 8 --grid=1mm \
      > "$here/raw/$tag.json" 2> "$here/raw/$tag.time"
    echo "rc=$?" >> "$here/raw/$tag.time"
    uptime > "$here/raw/$tag.load_after"
  done
done
touch "$here/raw/campaign.done"
