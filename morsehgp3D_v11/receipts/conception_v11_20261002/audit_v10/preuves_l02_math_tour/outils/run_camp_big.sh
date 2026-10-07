#!/bin/bash
# Campagne du juge L02 sur le build de reference : n 10-12, K = 8 (80 %) ou 10 (20 %), huit familles, 2 x 200 nuages.
cd /tmp/v11-audit/l02_math_tour
run_one() {
  s=$1
  ( /usr/bin/time -f "wall=%e s" env L02_TMP=/tmp/v11-audit/l02_math_tour/run nice -n 10 python3 -B tools/l02_judge.py tools/l02_dump $s 200 generic,grid,plane,clusters,sphere,circle,line,cube 10-12 8 ) > camp/big_seed$s.log 2>&1
  echo "exit=$?" >> camp/big_seed$s.log
}
export -f run_one
printf "%s\n" 21 22 | xargs -P 2 -I{} bash -c 'run_one {}'
touch camp/big.done
