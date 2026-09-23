#!/bin/bash
cd /workspaces/E-HGP/build/v9-open-worktree
for K in 5 10; do for s in 00 01 02; do
  /usr/bin/time -v build/v9-dev/mhgp9_tower_probe /workspaces/E-HGP/morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_${s}_grid/full.u32le $K 8 --grid=1mm > build/v9-runs/dead_20260923/s${s}_k${K}.json 2> build/v9-runs/dead_20260923/s${s}_k${K}.time
  echo "s$s K$K rc=$?"
done; done
echo ALLDONE
