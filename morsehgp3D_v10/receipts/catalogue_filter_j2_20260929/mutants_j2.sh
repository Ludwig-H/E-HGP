#!/bin/bash
# Mutants J2 (hors patch) : chaque mutant est construit dans /tmp/j2work/mutN, puis juge par
#   (1) le differentiel de dump et de grand livre contre la reference 568d45297 (quart 01 et synthetiques, K = 5 et 10),
#   (2) l'oracle T2 (tests/oracle/test_catalogue_oracle.py, code 0 conforme, 1 desaccord).
# usage : mutants_j2.sh N...   (sources deja preparees dans /tmp/j2work/mutN/morsehgp3D_v10)
export PYTHONDONTWRITEBYTECODE=1
REF=/workspaces/E-HGP/build/v10-wt/mhgp10_catalogue
IN=/workspaces/E-HGP/build/v10-scale-inputs
ORACLE=/workspaces/E-HGP/build/v10-j2/morsehgp3D_v10/tests/oracle/test_catalogue_oracle.py
T=$(mktemp -d /tmp/j2mut.XXXXXX)
for M in "$@"; do
  S=/tmp/j2work/mut$M/morsehgp3D_v10; B=/tmp/j2work/mut$M/build
  cmake -S $S -B $B -DCMAKE_BUILD_TYPE=Release > /dev/null && cmake --build $B --parallel 4 --target mhgp10_catalogue 2>&1 | grep -E "error" ;
  echo "== mutant $M"
  for f in lidar01_quarter_x_neg_y_neg syn_shells_density_x2; do
    for K in 5 10; do
      $REF $IN/$f.u32le --k=$K --threads=3 --dump=$T/o.dump > $T/o.json
      a=$(sha256sum $T/o.dump | cut -c1-16); rm -f $T/o.dump
      $B/mhgp10_catalogue $IN/$f.u32le --k=$K --threads=3 --dump=$T/m.dump > $T/m.json
      b=$(sha256sum $T/m.dump | cut -c1-16); rm -f $T/m.dump
      led=$(python3 - $T/o.json $T/m.json << 'PY'
import json, sys
o, m = (json.load(open(p)) for p in sys.argv[1:3])
keys = ['balls', 'nodes', 'leaves', 'sum_m', 'max_m', 'pair_tests', 'triple_tests', 'quad_tests', 'judged']
diff = [k for k in keys if o[k] != m[k]]
print('grand-livre', 'identique' if not diff else 'DIFFERENT ' + ' '.join('%s:%s->%s' % (k, o[k], m[k]) for k in diff))
PY
)
      [ "$a" = "$b" ] && r=dump_identique || r=DUMP_DIFFERENT
      echo "  $f K=$K ref=$a mut=$b $r ; $led"
    done
  done
  python3 $ORACLE $B > $T/oracle.txt 2>&1; rc=$?
  echo "  oracle T2 : code $rc ; $(tail -1 $T/oracle.txt)"
done
rm -rf $T
