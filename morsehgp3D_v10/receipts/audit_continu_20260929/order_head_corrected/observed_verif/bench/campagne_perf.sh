#!/bin/bash
# Rejeu relatif (verificateur adverse) : processus neufs alternes, ordre des variantes tourne a chaque tour.
# usage : campagne_perf.sh SORTIE.tsv TOURS K FILS MODE(cat|cluster) VARIANTE=BIN ...
OUT=$1; ROUNDS=$2; K=$3; W=$4; MODE=$5; shift 5
IN=/workspaces/E-HGP/build/v10-scale-inputs/lidar02_full.u32le
T=$(mktemp -d /tmp/ov_perf.XXXXXX)
VARS=("$@"); NV=${#VARS[@]}
echo -e "tour\tvariante\tcharge1\tminflt\tmaxrss_kb\twall\tjson" > $OUT
for r in $(seq 0 $((ROUNDS-1))); do
  for i in $(seq 0 $((NV-1))); do
    vb=${VARS[$(( (i + r) % NV ))]}; v=${vb%%=*}; B=${vb#*=}
    load=$(cut -d' ' -f1 /proc/loadavg)
    if [ $MODE = cat ]; then
      /usr/bin/time -f "%R %M %e" -o $T/time $B $IN --k=$K --threads=$W > $T/json 2>/dev/null
    else
      /usr/bin/time -f "%R %M %e" -o $T/time $B $IN $T/lab --k=$K --mcs=200 --z=3 --selection=eom --entry=cover --threads=$W > $T/json 2>/dev/null
    fi
    read flt rss wall < $T/time
    echo -e "$r\t$v\t$load\t$flt\t$rss\t$wall\t$(tail -1 $T/json)" >> $OUT
  done
done
rm -rf $T
echo FIN > ${OUT%.tsv}.done
