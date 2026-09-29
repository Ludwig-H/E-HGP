#!/bin/bash
# Juge des mutants J2c (hors patch). Pour chaque mutant de /tmp/j2work/j2cmut/mN :
#   (1) dumps contre la reference 568d45297 (quart 01, syn_shells_x2, grid10 ; K = 5 et 10) ;
#   (2) grand livre de l'arbre contre J2c correct (memes entrees) ;
#   (3) oracle T2 (tests/oracle/test_catalogue_oracle.py : 0 conforme, 1 desaccord), rejoue une fois s'il echoue,
#       pour ecarter la course du pool (antérieure a J2, signalee par le coordinateur).
# usage : mutants_j2c.sh J2C_BIN m1 m2 ...
export PYTHONDONTWRITEBYTECODE=1
GOOD=${1:?binaire J2c correct}; shift
REF=/workspaces/E-HGP/build/v10-j2/build-base/mhgp10_catalogue
IN=/workspaces/E-HGP/build/v10-scale-inputs
ORACLE=/workspaces/E-HGP/build/v10-j2/j2c/morsehgp3D_v10/tests/oracle/test_catalogue_oracle.py
T=$(mktemp -d /tmp/j2cmut.XXXXXX)
for M in "$@"; do
  S=/tmp/j2work/j2cmut/$M/morsehgp3D_v10; B=/tmp/j2work/j2cmut/$M/build
  cmake -S $S -B $B -DCMAKE_BUILD_TYPE=Release > /dev/null && cmake --build $B --parallel 4 --target mhgp10_catalogue 2>&1 | grep -E "error"
  echo "== mutant $M"
  for f in $IN/lidar01_quarter_x_neg_y_neg $IN/syn_shells_density_x2 /tmp/j2work/degen/grid10; do
    for K in 5 10; do
      $REF $f.u32le --k=$K --threads=3 --dump=$T/o.dump > /dev/null; a=$(sha256sum $T/o.dump | cut -c1-16); rm -f $T/o.dump
      $GOOD $f.u32le --k=$K --threads=3 > $T/g.json
      $B/mhgp10_catalogue $f.u32le --k=$K --threads=3 --dump=$T/m.dump > $T/m.json; rc=$?
      b=$( [ $rc -eq 0 ] && sha256sum $T/m.dump | cut -c1-16 || echo "refus:$(cat $T/m.json)"); rm -f $T/m.dump
      led=$(python3 - $T/g.json $T/m.json << 'PY'
import json, sys
try:
    g, m = (json.load(open(p)) for p in sys.argv[1:3])
except Exception:
    print('grand-livre illisible'); sys.exit(0)
if m.get('status') != 'ok':
    print('grand-livre sans objet (refus)'); sys.exit(0)
keys = ['balls', 'nodes', 'leaves', 'sum_m', 'filter_tests', 'pair_tests', 'quad_tests', 'judged']
diff = [k for k in keys if g[k] != m[k]]
print('grand-livre', 'identique a J2c' if not diff else 'DIFFERENT de J2c ' + ' '.join('%s:%s->%s' % (k, g[k], m[k]) for k in diff))
PY
)
      [ "$a" = "$b" ] && r=dump_identique || r=DUMP_DIFFERENT
      echo "  $(basename $f) K=$K ref=$a mut=$b $r ; $led"
    done
  done
  for essai in 1 2; do
    python3 $ORACLE $B > $T/oracle.txt 2>&1; rc=$?
    echo "  oracle T2 (essai $essai) : code $rc ; $(tail -1 $T/oracle.txt) ; premier ecart : $(grep -m1 ECART $T/oracle.txt | cut -c1-160)"
    [ $rc -eq 0 ] && break
  done
done
rm -rf $T
