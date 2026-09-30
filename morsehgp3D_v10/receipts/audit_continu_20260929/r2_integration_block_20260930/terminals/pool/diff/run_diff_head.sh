#!/bin/bash
# Differentiel tete : mhgp10_cluster (etiquettes + arbre) et mhgp10_mreach_cluster (etiquettes), base a 4 fils contre
# final a 1, 3 et 8 fils, n = 8000, 16000, 32000.
V=/tmp/mhgp10-r2/pool-verif
S=/workspaces/E-HGP/build/v10-scale-inputs
OUT=$V/diff/results_head.txt
: > $OUT
W=$V/diff/work_head; mkdir -p $W
runc() {  # bin in k th -> "code sha_labels sha_tree"
  rm -f $W/l $W/t
  nice -n 5 $1/mhgp10_cluster $S/$2 $W/l --k=$3 --mcs=20 --threads=$4 --tree=$W/t > $W/so 2>&1
  local c=$?
  echo "$c $(sha256sum < $W/l 2>/dev/null | cut -c1-16) $(sha256sum < $W/t 2>/dev/null | cut -c1-16)"
}
runm() {
  rm -f $W/l
  nice -n 5 $1/mhgp10_mreach_cluster $S/$2 $W/l --k=$3 --mcs=20 --threads=$4 > $W/so 2>&1
  local c=$?
  echo "$c $(sha256sum < $W/l 2>/dev/null | cut -c1-16) $(sha256sum < $W/so | cut -c1-16)"
}
for in in syn_filaments_space_x1.u32le syn_clusters_density_x2.u32le syn_shells_density_x4.u32le lidar02_quarter_x_neg_y_neg.u32le; do
  for k in 2 5 10; do
    for f in runc runm; do
      ref=$($f $V/build-base $in $k 4)
      line="$f $in K=$k base4=[$ref]"; same=1
      for th in 1 3 8; do r=$($f $V/build-final $in $k $th); line="$line final$th=[$r]"; [ "$r" = "$ref" ] || same=0; done
      echo "$line $([ $same = 1 ] && echo IDENTIQUE || echo ECART)" >> $OUT
    done
  done
done
echo DONE >> $OUT
