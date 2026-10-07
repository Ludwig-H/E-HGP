#!/bin/bash
# Determinisme multi-fils de la tour : dumps (noeuds, parents, niveaux exacts, verticales, attaches) a 1, 3 et 4 fils.
D=/workspaces/E-HGP/build/v10-g4-data-s1
O=/tmp/v11-audit/l06_code_tour/dumps
cd /tmp/v11-audit/l06_code_tour
: > determinism.txt
for frame in lidar00 lidar02; do
  for entry in core cover; do
    for t in 1 3 4; do
      ./build-release/mhgp10_tower $D/${frame}_full.u32le --k=5 --threads=$t --entry=$entry --dump=$O/${frame}_k5_${entry}_t$t.txt > $O/${frame}_k5_${entry}_t$t.json
      echo "$frame k5 $entry t$t exit=$? $(sha256sum $O/${frame}_k5_${entry}_t$t.txt | cut -d' ' -f1) $(stat -c %s $O/${frame}_k5_${entry}_t$t.txt)" >> determinism.txt
      if [ $t != 1 ]; then rm -f $O/${frame}_k5_${entry}_t$t.txt; fi
    done
  done
done
for t in 1 4; do
  ./build-release/mhgp10_tower $D/lidar01_full.u32le --k=10 --threads=$t --entry=core --dump=$O/lidar01_k10_core_t$t.txt > $O/lidar01_k10_core_t$t.json
  echo "lidar01 k10 core t$t exit=$? $(sha256sum $O/lidar01_k10_core_t$t.txt | cut -d' ' -f1) $(stat -c %s $O/lidar01_k10_core_t$t.txt)" >> determinism.txt
  rm -f $O/lidar01_k10_core_t$t.txt
done
touch determinism.done
