#!/bin/bash
# Invariants globaux aux tailles d'interet 8000 / 16000 / 32000 (entrees existantes v10-scale-inputs, regime spatial).
cd /tmp/v11-audit/l02_math_tour
IN=/workspaces/E-HGP/build/v10-scale-inputs
for fam in uniform clusters shells filaments terrain; do
  for x in 1 2 4; do
    f=syn_${fam}_space_x${x}
    ( /usr/bin/time -f "wall=%e s maxrss=%M KiB" nice -n 10 tools/l02_dump $IN/$f.u32le --k=5 --kcat=7 --threads=2 --no-points ) > scale/${f}_k5_kcat7.json 2> scale/${f}_k5_kcat7.time
    echo "rc=$?" >> scale/${f}_k5_kcat7.time
    L02_TMP=/tmp/v11-audit/l02_math_tour/run nice -n 10 python3 -B -W ignore tools/l02_emst_k1.py build_ref $IN/$f.u32le 2 > scale/${f}_k1_emst.json 2> scale/${f}_k1_emst.err
    echo "rc=$?" >> scale/${f}_k1_emst.err
  done
done
touch scale/scale.done
