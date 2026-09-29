#!/bin/bash
# Differentiel J1 : catalogue c764e121a (frontiere par compte) contre frontiere par charge + ordre LPT, a 1 et 3 fils.
OLD=/workspaces/E-HGP/build/v10-bench-c764e121a/build/mhgp10_catalogue
NEW=./mhgp10_catalogue.lpt
IN=/workspaces/E-HGP/build/v10-scale-inputs
for f in lidar02_full lidar00_full lidar01_quarter_x_neg_y_neg syn_shells_density_x2 syn_filaments_space_x4; do
  for K in 5 10; do
    $OLD $IN/$f.u32le --k=$K --threads=3 --dump=o.dump > o.json
    $NEW $IN/$f.u32le --k=$K --threads=3 --dump=n3.dump > n3.json
    $NEW $IN/$f.u32le --k=$K --threads=1 --dump=n1.dump > n1.json
    a=$(sha256sum o.dump | cut -c1-16); b=$(sha256sum n3.dump | cut -c1-16); c=$(sha256sum n1.dump | cut -c1-16)
    if [ "$a" = "$b" ] && [ "$b" = "$c" ]; then r=IDENTIQUES; else r=DIFFERENTS; fi
    led=$(python3 -c "
import json
o=json.load(open('o.json')); n=json.load(open('n3.json'))
keys=[k for k in o if k.endswith('tests') or k in ('nodes','leaves','judged','emitted','balls')]
print('grand-livre', 'identique' if all(o[k]==n[k] for k in keys) else 'DIFFERENT', 'taches', n['catalogue_stages']['tasks'], 'max_sites', n['catalogue_stages']['max_task_sites'])")
    echo "$f K=$K ancien=$a lpt3=$b lpt1=$c $r ; $led"
    rm -f o.dump n3.dump n1.dump
  done
done
echo FIN
