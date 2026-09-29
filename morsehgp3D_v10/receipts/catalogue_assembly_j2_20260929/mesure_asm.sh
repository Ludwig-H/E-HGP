#!/bin/bash
# Mesure de l'assemblage : J2 contre J2 + assemblage, executions alternees, REPS repetitions.
# Cas : trames 02 et 00 a 4 fils, trame 02 a 1 fil ; K = 5 et 10. Sortie TSV brute puis medianes.
# usage : mesure_asm.sh REPS OUT.tsv nom1=BIN1 nom2=BIN2
REPS=$1; OUT=$2; shift 2
IN=/workspaces/E-HGP/build/v10-scale-inputs
T=$(mktemp -d /tmp/j2asm.XXXXXX)
: > $OUT
CASES="lidar02_full:4 lidar00_full:4 lidar02_full:1"
for rep in $(seq $REPS); do
  for c in $CASES; do
    f=${c%%:*}; W=${c#*:}
    for K in 5 10; do
      for nb in "$@"; do
        name=${nb%%=*}; bin=${nb#*=}
        $bin $IN/$f.u32le --k=$K --threads=$W > $T/r.json
        python3 - $T/r.json $name $f $K $W $rep >> $OUT << 'PY'
import json, sys
d = json.load(open(sys.argv[1])); s = d['catalogue_stages']
keys = ('t_boxes', 't_order', 't_collect', 't_sort', 't_bands', 't_assemble', 't_compare', 't_ranks', 't_copy')
print('\t'.join(sys.argv[2:7] + ['%.4f' % d['catalogue_s']] + ['%.4f' % s[k] for k in keys] + [str(d['balls'])]))
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
cols = ['catalogue_s', 't_boxes', 't_order', 't_collect', 't_sort', 't_bands', 't_assemble', 't_compare', 't_ranks', 't_copy']
print('entree K fils : ' + ' | '.join('%s : %s' % (n, ' '.join(c for c in ('t_order', 't_assemble', 'order+assemble'))) for n in names))
for f, W in (('lidar02_full', '4'), ('lidar00_full', '4'), ('lidar02_full', '1')):
    for K in ('5', '10'):
        out = []
        for n in names:
            sel = [r for r in rows if r[0] == n and r[1] == f and r[2] == K and r[3] == W]
            med = {c: st.median(float(r[5 + i]) for r in sel) for i, c in enumerate(cols)}
            oa = st.median(float(r[7]) + float(r[11]) for r in sel)
            out.append('%s : order %.3f assemble %.3f (compare %.3f ranks %.3f copy %.3f) somme %.3f catalogue %.3f' % (
                n, med['t_order'], med['t_assemble'], med['t_compare'], med['t_ranks'], med['t_copy'], oa, med['catalogue_s']))
        print(f, 'K=%s' % K, 'W=%s' % W, ' | '.join(out))
PY
