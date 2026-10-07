#!/bin/bash
# J-KM2 (restriction) : cat(K) == restrict(cat(K+2), p + q <= K + 1), enregistrement par enregistrement, dans l'ordre
# canonique, rang exclu (les rangs different). Entree non ponderee. Empreintes sha256 des deux flux.
# Usage : jkm2.sh BIN ENTREE K
BIN=$1; IN=$2; K=$3; K2=$((K+2)); T=$(mktemp -d /tmp/v11-audit/l01_math_catalogue/jkm2/tmp.XXXXXX)
mkfifo $T/a $T/b
( awk -F'|' '{split($1,h," "); print h[2],h[3],h[4],h[5] "|" $2 "|" $3 "|" $4}' < $T/a | tee >(wc -l > $T/na) | sha256sum > $T/ha ) &
$BIN $IN --k=$K --threads=3 --dump=$T/a > $T/ja.json
wait
( awk -F'|' -v K=$K '{split($1,h," "); if (h[3] + h[2] <= K + 1) print h[2],h[3],h[4],h[5] "|" $2 "|" $3 "|" $4}' < $T/b | tee >(wc -l > $T/nb) | sha256sum > $T/hb ) &
$BIN $IN --k=$K2 --threads=3 --dump=$T/b > $T/jb.json
wait
sleep 0.5
echo "$(basename $IN) K=$K : cat(K) $(cat $T/na) lignes sha256=$(cut -c1-16 $T/ha) | restrict(cat(K+2)) $(cat $T/nb) lignes sha256=$(cut -c1-16 $T/hb) | $( [ "$(cat $T/ha)" = "$(cat $T/hb)" ] && echo EGAL || echo DIFFERENT )"
rm -rf $T
