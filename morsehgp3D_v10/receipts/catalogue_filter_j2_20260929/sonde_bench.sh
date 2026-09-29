#!/bin/bash
# usage : bench.sh INPUT K REPS variant1 variant2 ... (variantes = dossiers probe_*)
IN=/workspaces/E-HGP/build/v10-scale-inputs/$1.u32le; K=$2; N=$3; shift 3
cd /tmp/j2work
out=/tmp/j2work/bench_$$.txt; : > $out
for i in $(seq $N); do
  for v in "$@"; do
    r=$(./probe_$v/build/mhgp10_catalogue $IN --k=$K --threads=1 2>&1 >/tmp/j2work/pb_$$.json | awk '{print $2}')
    t=$(python3 -c "import json; d=json.load(open('/tmp/j2work/pb_$$.json')); print(d['catalogue_stages']['t_boxes'])")
    echo "$v $r $t" >> $out
  done
done
python3 - $out "$@" << 'PY'
import sys, statistics as st
rows = [l.split() for l in open(sys.argv[1])]
for v in sys.argv[2:]:
    f = sorted(float(r[1]) for r in rows if r[0] == v); b = sorted(float(r[2]) for r in rows if r[0] == v)
    print('%-8s filtre Gcyc min %.3f med %.3f | t_boxes min %.3f med %.3f' % (v, f[0], st.median(f), b[0], st.median(b)))
PY
rm -f $out /tmp/j2work/pb_$$.json
