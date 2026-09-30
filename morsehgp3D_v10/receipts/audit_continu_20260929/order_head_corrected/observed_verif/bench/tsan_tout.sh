#!/bin/bash
# TSan sur la combinaison tout (v2 + v3c + THP + v4) : catalogue (dump) et chaine (etiquettes + arbre), 4 fils,
# compares aux binaires de base (Release). Avertissements TSan comptes dans stderr.
V=/workspaces/E-HGP/build/v10-perf/ordre_tete-verif
T=/tmp/ov_tsan_run; mkdir -p $T
TS=/tmp/ov_tsan_tout; B=$V/build/base
L=/workspaces/E-HGP/build/v10-scale-inputs; D=/workspaces/E-HGP/build/v10-perf/ordre_tete/out/degen
OUT=$V/out/tsan_tout.txt
echo "debut $(date -Is) charge $(cut -d' ' -f1-3 /proc/loadavg)" > $OUT
for c in $L/lidar01_quarter_x_neg_y_neg.u32le:5 $L/lidar01_quarter_x_neg_y_neg.u32le:10 $D/grid16.u32le:10 $D/grid10.u32le:10; do
  f=${c%:*}; K=${c##*:}
  $B/mhgp10_catalogue $f --k=$K --threads=4 --dump=$T/ref.dump > /dev/null 2>&1
  setarch $(uname -m) -R $TS/mhgp10_catalogue $f --k=$K --threads=4 --dump=$T/ts.dump > /dev/null 2> $T/ts.err; code=$?
  w=$(grep -c "WARNING: ThreadSanitizer" $T/ts.err)
  cmp -s $T/ref.dump $T/ts.dump && r=IDENTIQUES || r=DIFFERENTS
  echo "catalogue $(basename $f) K=$K code=$code avertissements_tsan=$w $r" >> $OUT
done
for c in $L/lidar01_quarter_x_neg_y_neg.u32le:5:cover $L/lidar01_quarter_x_neg_y_neg.u32le:10:cover $L/lidar01_quarter_x_neg_y_neg.u32le:5:core $D/grid10.u32le:10:core; do
  IFS=: read f K e <<< "$c"
  $B/mhgp10_cluster $f $T/ref.lab --k=$K --mcs=20 --z=3 --entry=$e --threads=4 --tree=$T/ref.tree > /dev/null 2>&1
  setarch $(uname -m) -R $TS/mhgp10_cluster $f $T/ts.lab --k=$K --mcs=20 --z=3 --entry=$e --threads=4 --tree=$T/ts.tree > /dev/null 2> $T/ts.err; code=$?
  w=$(grep -c "WARNING: ThreadSanitizer" $T/ts.err)
  cmp -s $T/ref.lab $T/ts.lab && cmp -s $T/ref.tree $T/ts.tree && r=IDENTIQUES || r=DIFFERENTS
  echo "cluster $(basename $f) K=$K $e code=$code avertissements_tsan=$w $r" >> $OUT
done
echo "fin $(date -Is)" >> $OUT
rm -rf $T
