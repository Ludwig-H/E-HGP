#!/bin/bash
# Rejeu L01 : amas denses tres eloignes, feuille par defaut. Colonnes : c amas, m points par amas, cote a de l'amas,
# arete D du cube des coins ; compteurs deterministes du grand livre.
cd /tmp/v11-audit/l01_math_catalogue || exit 1
for spec in "2 20 16 262144" "2 50 16 262144" "2 200 32 262144" "2 1000 64 262144" "2 4000 128 262144" "8 1000 64 262144" "2 1000 64 4096" "2 1000 64 512"; do
  set -- $spec
  f=amas/c$1_m$2_a$3_D$4.u32le
  python3 amas/gen_amas.py $f $1 $2 $3 $4 11 > /dev/null
  for K in 5 10; do
    timeout 300 build/mhgp10_catalogue $f --k=$K --threads=3 > amas/out.json; code=$?
    python3 - "$spec" $K $code <<'PY'
import json, sys
spec, K, code = sys.argv[1], sys.argv[2], sys.argv[3]
d = json.load(open('amas/out.json'))
print('c m a D = %-20s K=%2s code=%s sites=%6d boules=%9d (%.1f/site) noeuds=%9d (%.1f/site) feuilles=%8d sum_m=%9d max_m=%3d bloquees=%d filter_tests=%.3g' % (spec, K, code, d['sites'], d['balls'], d['balls'] / d['sites'], d['nodes'], d['nodes'] / d['sites'], d['leaves'], d['sum_m'], d['max_m'], d['stalled_leaves'], d['filter_tests']))
PY
  done
done
