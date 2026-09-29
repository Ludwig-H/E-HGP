#!/bin/bash
# Mesure J2 : catalogue_stages.t_boxes a 1 fil, executions alternees (avant, apres, ...), REPS repetitions.
# usage : mesure_j2.sh REPS OUT.tsv nom1=BIN1 nom2=BIN2 ...
# Entrees : lidar02_full, lidar00_full, lidar01_quarter_x_neg_y_neg ; K = 5 et 10. Sortie TSV brute, puis medianes.
REPS=$1; OUT=$2; shift 2
IN=/workspaces/E-HGP/build/v10-scale-inputs
T=$(mktemp -d /tmp/j2mes.XXXXXX)
: > $OUT
for rep in $(seq $REPS); do
  for f in lidar02_full lidar00_full lidar01_quarter_x_neg_y_neg; do
    for K in 5 10; do
      for nb in "$@"; do
        name=${nb%%=*}; bin=${nb#*=}
        $bin $IN/$f.u32le --k=$K --threads=1 > $T/r.json
        python3 - $T/r.json $name $f $K $rep >> $OUT << 'PY'
import json, sys
d = json.load(open(sys.argv[1])); s = d['catalogue_stages']
print('\t'.join([sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5], '%.4f' % s['t_boxes'], '%.4f' % d['catalogue_s'],
                 '%.4f' % s['t_assemble'], str(d['balls'])]))
PY
      done
    done
  done
done
rm -rf $T
python3 - $OUT "$@" << 'PY'
import sys, statistics as st
rows = [l.rstrip('\n').split('\t') for l in open(sys.argv[1])]
names = [a.split('=', 1)[0] for a in sys.argv[2:]]
print('entree K ' + ' '.join('%s(t_boxes med [min-max])' % n for n in names) + ' rapports')
for f in ('lidar02_full', 'lidar00_full', 'lidar01_quarter_x_neg_y_neg'):
    for K in ('5', '10'):
        med = {}
        cells = []
        for n in names:
            v = sorted(float(r[4]) for r in rows if r[0] == n and r[1] == f and r[2] == K)
            med[n] = st.median(v)
            cells.append('%.3f [%.3f-%.3f]' % (med[n], v[0], v[-1]))
        ratios = ' '.join('%s/%s x%.2f' % (names[0], n, med[names[0]] / med[n]) for n in names[1:])
        print(f, K, ' | '.join(cells), '|', ratios)
PY
