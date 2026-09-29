#!/bin/bash
# Matrice d'identite du verificateur : base (1 fil) contre variantes (plusieurs fils), empreintes de verif_digest.
# usage : matrix.sh SORTIE "VARIANTES" "FILS" ENTREE:CONFIG...
# CONFIG = nom d'une configuration ci-dessous. Resultat : une ligne par (entree, config).
V=/workspaces/E-HGP/build/v10-perf/tour-verif
OUT=$1; shift
VARS=$1; shift
WL=$1; shift
declare -A C
C[A]="--k=5 --no-points"
C[B]="--k=5 --only-order=5 --entry=cover"
C[C]="--k=6 --entry=cover --ball-nodes"
C[D]="--k=4 --entry=core"
C[E]="--k=5 --only-order=5 --entry=cover --cover-extra=1"
C[F]="--k=10 --only-order=10 --entry=cover"
C[G]="--k=10 --no-points"
C[H]="--k=5 --no-verticals --entry=core"
C[I]="--k=2 --no-points"
C[J]="--k=3 --only-order=1 --entry=core"
C[K]="--k=7 --entry=cover"
C[L]="--k=7 --only-order=7 --entry=cover"
digs() { python3 -c "
import json,sys
s=set()
for l in sys.stdin:
    l=l.strip()
    if not l: continue
    d=json.loads(l)
    s.add(d.get('digest') or ('ECHEC:'+d.get('status','?')+':'+str(d.get('reason'))+':'+str(d.get('order'))))
print(' '.join(sorted(s)) if s else 'VIDE')
"; }
for item in "$@"; do
  in=${item%%:*}; cfg=${item#*:}
  opts=${C[$cfg]}
  ref=$(timeout 1200 $V/build/base/verif_digest $in $opts --threads=1 --cat-threads=2 | digs)
  line="$(basename $in .u32le) $cfg base=$ref"
  verdict=OK
  for v in $VARS; do
    got=$(timeout 1200 $V/build/$v/verif_digest $in $opts --threads=$WL --repeat=1 --cat-threads=2 | digs)
    line="$line $v=$got"
    [ "$got" = "$ref" ] || verdict=ECART
  done
  echo "$line $verdict" >> $OUT
done
echo "FIN $(date -Is) $(uptime)" >> $OUT
