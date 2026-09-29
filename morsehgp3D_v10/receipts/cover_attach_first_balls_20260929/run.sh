#!/bin/bash
# Differentiel : attaches cover, binaire 25a6c8fc9 (une resolution par boule) contre le nouveau (premieres boules).
OLD=/workspaces/E-HGP/build/v10-bench-25a6c8fc9/mhgp10_tower
NEW=/workspaces/E-HGP/build/v10-wt/mhgp10_tower
cp $NEW ./mhgp10_tower.new
IN=/workspaces/E-HGP/build/v10-scale-inputs
for f in syn_filaments_space_x1 syn_clusters_density_x2 syn_shells_space_x1 lidar02_quarter_x_nonneg_y_nonneg lidar01_quarter_x_neg_y_neg; do
  for K in 5 10; do
    $OLD $IN/$f.u32le --k=$K --threads=2 --entry=cover --dump=old_${f}_k$K.dump > old_${f}_k$K.json; a=$?
    ./mhgp10_tower.new $IN/$f.u32le --k=$K --threads=2 --entry=cover --dump=new_${f}_k$K.dump > new_${f}_k$K.json; b=$?
    if cmp -s old_${f}_k$K.dump new_${f}_k$K.dump; then r=IDENTIQUE; else r=DIFFERENT; fi
    echo "$f K=$K rc=$a/$b $r $(sha256sum new_${f}_k$K.dump | cut -c1-16)"
    python3 -c "import json,sys; o=json.load(open('old_${f}_k$K.json')); n=json.load(open('new_${f}_k$K.json')); print('   old', {k:v for k,v in o.items() if k.endswith('_s') or k in ('t_attach','attach_s')}); print('   new', {k:v for k,v in n.items() if k.endswith('_s') or k in ('t_attach','attach_s')})" 2>/dev/null
    rm -f old_${f}_k$K.dump
  done
done
echo FIN
