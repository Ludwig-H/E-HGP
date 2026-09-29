#!/bin/bash
# Entrees degenerees (grilles entieres, spheres de points entiers) : J2c contre la reference 568d45297.
# Dump identique, ou meme refus (statut et raison). Compteurs de l'arbre et feuilles bloquees releves.
# usage : degen_j2c.sh NEW_BIN [REF_BIN] [FILS]
NEW=${1:?binaire}
REF=${2:-/workspaces/E-HGP/build/v10-j2/build-base/mhgp10_catalogue}
W=${3:-4}
D=/tmp/j2work/degen
T=$(mktemp -d /tmp/j2deg.XXXXXX)
fail=0
for f in grid10 grid10s7 grid16 sphere101 sphere314; do
  for K in 5 10; do
    $REF $D/$f.u32le --k=$K --threads=$W --dump=$T/o.dump > $T/o.json; ro=$?
    $NEW $D/$f.u32le --k=$K --threads=$W --dump=$T/n.dump > $T/n.json; rn=$?
    if [ $ro -eq 0 ] && [ $rn -eq 0 ]; then
      a=$(sha256sum $T/o.dump | cut -c1-16); b=$(sha256sum $T/n.dump | cut -c1-16)
      [ "$a" = "$b" ] && r=IDENTIQUES || { r=DIFFERENTS; fail=1; }
      info=$(python3 -c "
import json; o=json.load(open('$T/o.json')); n=json.load(open('$T/n.json'))
print('boules', o['balls'], 'ref: noeuds', o['nodes'], 'feuilles', o['leaves'], 'bloquees', o['stalled_leaves'], 'm_max', o['max_m'],
      '| j2c: noeuds', n['nodes'], 'feuilles', n['leaves'], 'bloquees', n['stalled_leaves'], 'm_max', n['max_m'],
      'identite', n['nodes'] - n['leaves'] - (n['nodes'] - 1) // 2 == n['skipped_bbox'])")
      echo "$f K=$K ref=$a j2c=$b $r ; $info"
    else
      so=$(cat $T/o.json); sn=$(cat $T/n.json)
      [ "$ro" = "$rn" ] && [ "$so" = "$sn" ] && r=MEME_REFUS || { r=REFUS_DIFFERENTS; fail=1; }
      echo "$f K=$K ref=[code $ro $so] j2c=[code $rn $sn] $r"
    fi
    rm -f $T/o.dump $T/n.dump
  done
done
rm -rf $T
echo "FIN echec=$fail"
exit $fail
