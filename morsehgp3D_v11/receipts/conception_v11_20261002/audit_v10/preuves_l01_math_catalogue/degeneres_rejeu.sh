#!/bin/bash
# Rejeu L01 des entrees degenerees : catalogue (K = 5 et 10), puis tour (K = 5) sur trois d'entre elles.
cd /tmp/v11-audit/l01_math_catalogue || exit 1
for f in sph_89 sph_101 sph_314 plan_40 grille_12 droite_2000 cercle_108; do for K in 5 10; do
 out=$(timeout 300 build/mhgp10_catalogue degen/$f.u32le --k=$K --threads=2); code=$?
 echo "catalogue $f K=$K code=$code $(echo "$out" | python3 -c "
import json,sys
d=json.loads(sys.stdin.read())
if d['status']!='ok': print(d)
else: print('sites',d['sites'],'boules',d['balls'],'etendues',d['extended'],'coquille_max',d['max_shell'],'noeuds',d['nodes'],'feuilles',d['leaves'],'max_m',d['max_m'],'bloquees',d['stalled_leaves'],'t_catalogue=%.2fs (local, indicatif)'%d['catalogue_s'])
")"
done; done
for f in sph_89 cercle_108 grille_12; do
 out=$(timeout 300 build/mhgp10_tower degen/$f.u32le --k=5 --threads=2); code=$?
 echo "tour $f K=5 code=$code $(echo "$out" | cut -c1-110)"
done
