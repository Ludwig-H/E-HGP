#!/bin/bash
# Rejeu L01 : 14 points (deux amas de 7 points aux coins opposes du domaine u18), K = 10, un fil.
# Feuille par defaut (M = 24), puis --leaf=13 (= K + 3), 12, 11 ; delai de 60 s par appel.
cd /tmp/v11-audit/l01_math_catalogue || exit 1
for leaf in 0 13 12 11; do
  opt=""; [ $leaf != 0 ] && opt="--leaf=$leaf"
  s=$(date +%s.%N)
  timeout 60 build/mhgp10_catalogue petite_feuille/amas_coins_14.u32le --k=10 --threads=1 $opt > petite_feuille/out_leaf$leaf.json; code=$?
  e=$(date +%s.%N)
  python3 - $leaf $code $s $e <<'PY'
import json, sys
leaf, code, s, e = sys.argv[1], sys.argv[2], float(sys.argv[3]), float(sys.argv[4])
try:
    d = json.load(open('petite_feuille/out_leaf%s.json' % leaf))
    print('leaf=%s code=%s duree=%.2fs boules=%d noeuds=%d feuilles=%d bloquees=%d max_m=%d' % (leaf, code, e - s, d['balls'], d['nodes'], d['leaves'], d['stalled_leaves'], d['max_m']))
except Exception:
    print('leaf=%s code=%s duree=%.2fs pas de sortie (delai de 60 s atteint)' % (leaf, code, e - s))
PY
done
