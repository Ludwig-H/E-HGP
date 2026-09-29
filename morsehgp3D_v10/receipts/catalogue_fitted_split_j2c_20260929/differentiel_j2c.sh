#!/bin/bash
# Differentiel J2c : catalogue 568d45297 (reference, 3 fils) contre J2c a 1 et 4 fils, sur les 10 entrees.
# Dumps canoniques complets dans /tmp, sha256 puis suppression.
# Compteurs du catalogue (boules, niveaux, by_q_p, extended, weighted, max_shell) : egaux a la reference.
# Compteurs de l'arbre (J2c) : egaux entre 1 et 4 fils ; identite de l'arbre binaire
# skipped = nodes - leaves - (nodes - 1) / 2 ; valeurs ecrites dans LEDGER (une ligne JSON par cas) si donne.
# usage : differentiel_j2c.sh NEW_BIN [REF_BIN] [LEDGER_OUT]
NEW=${1:?binaire J2c}
REF=${2:-/workspaces/E-HGP/build/v10-j2/build-base/mhgp10_catalogue}
LEDGER=${3:-}
IN=/workspaces/E-HGP/build/v10-scale-inputs
T=$(mktemp -d /tmp/j2cdiff.XXXXXX)
[ -n "$LEDGER" ] && : > $LEDGER
fail=0
for f in lidar02_full lidar00_full lidar01_quarter_x_neg_y_neg syn_shells_density_x2 syn_filaments_space_x4; do
  for K in 5 10; do
    $REF $IN/$f.u32le --k=$K --threads=3 --dump=$T/o.dump > $T/o.json
    a=$(sha256sum $T/o.dump | cut -d' ' -f1); rm -f $T/o.dump
    $NEW $IN/$f.u32le --k=$K --threads=1 --dump=$T/n1.dump > $T/n1.json
    b=$(sha256sum $T/n1.dump | cut -d' ' -f1); rm -f $T/n1.dump
    $NEW $IN/$f.u32le --k=$K --threads=4 --dump=$T/n4.dump > $T/n4.json
    c=$(sha256sum $T/n4.dump | cut -d' ' -f1); rm -f $T/n4.dump
    if [ -n "$a" ] && [ "$a" = "$b" ] && [ "$b" = "$c" ]; then r=IDENTIQUES; else r=DIFFERENTS; fail=1; fi
    led=$(python3 - $T/o.json $T/n1.json $T/n4.json "$f" "$K" "$LEDGER" << 'PY'
import json, sys
o, n1, n4 = (json.load(open(p)) for p in sys.argv[1:4])
f, K, ledger = sys.argv[4], sys.argv[5], sys.argv[6]
cat = ['balls', 'levels', 'by_q_p', 'extended', 'weighted', 'max_shell']
tree = ['nodes', 'leaves', 'skipped_bbox', 'sum_m', 'max_m', 'stalled_leaves', 'filter_tests', 'leaf_dominance_tests',
        'pair_tests', 'triple_tests', 'line_hits', 'quad_tests', 'judged']
ok_cat = all(o[k] == n1[k] == n4[k] for k in cat)
ok_thr = all(n1[k] == n4[k] for k in tree)
ok_id = n1['skipped_bbox'] == n1['nodes'] - n1['leaves'] - (n1['nodes'] - 1) // 2
if ledger:
    with open(ledger, 'a') as out:
        out.write(json.dumps(dict(entree=f, K=int(K), **{k: n1[k] for k in tree})) + '\n')
print('catalogue', 'identique' if ok_cat else 'DIFFERENT', '| arbre 1=4 fils', 'oui' if ok_thr else 'NON',
      '| identite binaire', 'ok' if ok_id else 'FAUSSE', '| noeuds', o['nodes'], '->', n1['nodes'], '| feuilles',
      o['leaves'], '->', n1['leaves'], '| filter_tests', o.get('filter_tests', o.get('guard_tests', 0) + o.get('dominance_tests', 0)),
      '->', n1['filter_tests'], '| bloquees', n1['stalled_leaves'])
sys.exit(0 if ok_cat and ok_thr and ok_id else 1)
PY
)
    [ $? -eq 0 ] || fail=1
    echo "$f K=$K ref3=${a:0:16} j2c_1=${b:0:16} j2c_4=${c:0:16} $r ; $led"
  done
done
rm -rf $T
echo "FIN echec=$fail"
exit $fail
