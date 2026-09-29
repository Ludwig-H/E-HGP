#!/bin/bash
# Calibration de M(K) pour J2c : t_boxes a 1 fil selon --leaf=M, valeurs de M alternees dans chaque repetition.
# usage : calibre_m.sh BIN REPS OUT.tsv K "M1 M2 ..." "entree1 entree2 ..."
BIN=$1; REPS=$2; OUT=$3; K=$4; MS=$5; INS=$6
IN=/workspaces/E-HGP/build/v10-scale-inputs
T=$(mktemp -d /tmp/j2cal.XXXXXX)
for rep in $(seq $REPS); do
  for f in $INS; do
    for M in $MS; do
      $BIN $IN/$f.u32le --k=$K --threads=1 --leaf=$M > $T/r.json
      python3 - $T/r.json $f $K $M $rep >> $OUT << 'PY'
import json, sys
d = json.load(open(sys.argv[1])); s = d['catalogue_stages']
print('\t'.join(sys.argv[2:6] + ['%.4f' % s['t_boxes'], '%.4f' % d['catalogue_s'], str(d['nodes']), str(d['leaves']),
                                 str(d['sum_m']), str(d['balls'])]))
PY
    done
  done
done
rm -rf $T
