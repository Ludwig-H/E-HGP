#!/bin/bash
# Mesure J2c : avant (b662673b2) contre apres (J2c), executions alternees, REPS repetitions, a 1 et 4 fils.
# Entrees : trames 02 et 00, quart 01 ; K = 5 et 10. Sortie TSV brute, puis medianes.
# usage : mesure_j2c.sh REPS OUT.tsv avant=BIN apres=BIN
REPS=$1; OUT=$2; shift 2
IN=/workspaces/E-HGP/build/v10-scale-inputs
T=$(mktemp -d /tmp/j2cmes.XXXXXX)
: > $OUT
for rep in $(seq $REPS); do
  for W in 1 4; do
    for f in lidar02_full lidar00_full lidar01_quarter_x_neg_y_neg; do
      for K in 5 10; do
        for nb in "$@"; do
          name=${nb%%=*}; bin=${nb#*=}
          $bin $IN/$f.u32le --k=$K --threads=$W > $T/r.json
          python3 - $T/r.json $name $f $K $W $rep >> $OUT << 'PY'
import json, sys
d = json.load(open(sys.argv[1])); s = d['catalogue_stages']
print('\t'.join(sys.argv[2:7] + ['%.4f' % d['catalogue_s'], '%.4f' % s['t_frontier'], '%.4f' % s['t_boxes'],
                                 '%.4f' % s['t_order'], '%.4f' % s['t_assemble'], str(d['balls']), str(d['nodes'])]))
PY
        done
      done
    done
  done
done
rm -rf $T
python3 - $OUT << 'PY'
import sys, statistics as st
rows = [l.rstrip('\n').split('\t') for l in open(sys.argv[1])]
print('entree K fils | t_boxes avant -> apres (gain) | catalogue_s avant -> apres (gain) | t_frontier avant -> apres')
for W in ('1', '4'):
    for f in ('lidar02_full', 'lidar00_full', 'lidar01_quarter_x_neg_y_neg'):
        for K in ('5', '10'):
            med = {}
            for n in ('avant', 'apres'):
                sel = [r for r in rows if r[0] == n and r[1] == f and r[2] == K and r[3] == W]
                med[n] = {c: st.median(float(r[i]) for r in sel) for c, i in (('cat', 5), ('front', 6), ('boxes', 7))}
                med[n]['rng'] = (min(float(r[7]) for r in sel), max(float(r[7]) for r in sel))
            a, b = med['avant'], med['apres']
            print('%s K=%s W=%s | %.3f [%.3f-%.3f] -> %.3f [%.3f-%.3f] (x%.2f) | %.3f -> %.3f (x%.2f) | %.4f -> %.4f' % (
                f, K, W, a['boxes'], a['rng'][0], a['rng'][1], b['boxes'], b['rng'][0], b['rng'][1], a['boxes'] / b['boxes'],
                a['cat'], b['cat'], a['cat'] / b['cat'], a['front'], b['front']))
PY
