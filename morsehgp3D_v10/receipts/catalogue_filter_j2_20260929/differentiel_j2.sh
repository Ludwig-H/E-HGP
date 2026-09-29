#!/bin/bash
# Differentiel J2 : catalogue 568d45297 (reference) contre le filtre J2, sur les 10 entrees du differentiel J1.
# Reference a 3 fils ; J2 a 1 et 4 fils. Dumps canoniques complets dans /tmp, sha256 puis suppression.
# Grand livre : compteurs inchanges compares a la reference ; nouveaux compteurs compares entre 1 et 4 fils.
# usage : differentiel_j2.sh NEW_BIN [REF_BIN]
NEW=${1:?binaire J2}
REF=${2:-/workspaces/E-HGP/build/v10-wt/mhgp10_catalogue}
IN=/workspaces/E-HGP/build/v10-scale-inputs
T=$(mktemp -d /tmp/j2diff.XXXXXX)
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
    led=$(python3 - $T/o.json $T/n1.json $T/n4.json << 'PY'
import json, sys
o, n1, n4 = (json.load(open(p)) for p in sys.argv[1:4])
same = ['balls', 'levels', 'by_q_p', 'nodes', 'leaves', 'sum_m', 'max_m', 'leaf_dominance_tests', 'pair_tests',
        'triple_tests', 'line_hits', 'quad_tests', 'judged', 'extended', 'weighted', 'max_shell', 'stalled_leaves']
new = ['skipped_bbox', 'preskipped_bbox', 'filter_tests']
ok_ref = all(o[k] == n1[k] == n4[k] for k in same)
ok_thr = all(n1[k] == n4[k] for k in new)
# arbre complet : chaque noeud interne a 8 enfants, donc ignores = noeuds - feuilles - (noeuds - 1) / 8
skipped_ref = o['nodes'] - o['leaves'] - (o['nodes'] - 1) // 8
ok_skip = n1['skipped_bbox'] == skipped_ref
print('grand-livre', 'identique' if ok_ref else 'DIFFERENT', '| nouveaux compteurs 1=4 fils',
      'oui' if ok_thr else 'NON', '| skipped_bbox', n1['skipped_bbox'], 'attendu', skipped_ref,
      'ok' if ok_skip else 'FAUX', '| preskipped', n1['preskipped_bbox'], '| filter_tests', n1['filter_tests'],
      '| ref guard+dominance', o['guard_tests'] + o['dominance_tests'])
sys.exit(0 if ok_ref and ok_thr and ok_skip else 1)
PY
)
    [ $? -eq 0 ] || fail=1
    echo "$f K=$K ref3=${a:0:16} j2_1=${b:0:16} j2_4=${c:0:16} $r ; $led"
  done
done
rm -rf $T
echo "FIN echec=$fail"
exit $fail
