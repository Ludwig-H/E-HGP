#!/bin/bash
# Catalogue v10 (HEAD afb081774, copie /tmp) sur les trois trames du contrat ; sortie JSON par (trame, K).
D=/workspaces/E-HGP/build/v10-g4-data-s1
B=/tmp/v11-audit/l01_math_catalogue/build/mhgp10_catalogue
O=/tmp/v11-audit/l01_math_catalogue/runs
for f in lidar02_full lidar00_full lidar01_full; do
  for K in 2 3 5 7 10 12; do
    nice -n 5 $B $D/$f.u32le --k=$K --threads=3 > $O/${f}_k$K.json 2> $O/${f}_k$K.err
    echo "$f K=$K code=$?" >> $O/done.log
  done
done
echo FIN >> $O/done.log
