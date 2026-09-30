#!/bin/bash
# Differentiel de sortie : base (56020cab6, sans correctif) a 4 fils contre final (pool_r2) a 1, 3 et 8 fils.
V=/tmp/mhgp10-r2/pool-verif
S=/workspaces/E-HGP/build/v10-scale-inputs
OUT=$V/diff/results.txt
: > $OUT
W=$V/diff/work; mkdir -p $W
run() {  # bin cli input k threads extra -> "code sha"
  local bin=$1 cli=$2 in=$3 k=$4 th=$5 extra=$6
  rm -f $W/d
  nice -n 5 $bin/$cli $S/$in --k=$k --threads=$th --dump=$W/d $extra > $W/stdout 2> $W/stderr
  local code=$?
  local sha=$(sha256sum < $W/d 2>/dev/null | cut -c1-24)
  local st=$(grep -o '"status":"[a-z_]*"' $W/stdout | head -1)
  rm -f $W/d
  echo "$code $sha $st"
}
for in in syn_filaments_space_x1.u32le syn_clusters_density_x2.u32le lidar02_quarter_x_neg_y_neg.u32le syn_shells_density_x4.u32le syn_filaments_space_x4.u32le lidar00_full.u32le; do
  for k in 5 10; do
    for cli in mhgp10_tower mhgp10_catalogue; do
      for extra in ""; do
        ref=$(run $V/build-base $cli $in $k 4 "$extra")
        line="$cli $in K=$k base4=[$ref]"
        allsame=1
        for th in 1 3 8; do
          r=$(run $V/build-final $cli $in $k $th "$extra")
          line="$line final$th=[$r]"
          [ "${r%% *}" = "${ref%% *}" ] && [ "$(echo $r | cut -d' ' -f2)" = "$(echo $ref | cut -d' ' -f2)" ] || allsame=0
        done
        echo "$line $([ $allsame = 1 ] && echo IDENTIQUE || echo ECART)" >> $OUT
      done
    done
  done
done
echo DONE >> $OUT
