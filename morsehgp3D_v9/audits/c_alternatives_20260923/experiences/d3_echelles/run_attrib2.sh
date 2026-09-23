#!/bin/bash
# Suite reordonnee : K10 d'abord. Charge 1 min < 6, nice 19, un fil, budget cumule (140 s deja consommes).
D=/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/data
cd /tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/design/d3_echelles
used=140
for spec in "s02_k5_s8_w8_r0_nested_8000 10" "s00_k5_s8_w8_r0_nested_8000 10" "s01_k5_s8_w8_r0_nested_32000 5"; do
  set -- $spec
  f=$1; k=$2
  out=attrib_${f}_k${k}.json
  [ -s "$out" ] && continue
  if [ "$used" -gt 360 ]; then echo "budget atteint ($used s)" >> run.log; break; fi
  waited=0
  while :; do
    l=$(cut -d' ' -f1 /proc/loadavg)
    if awk "BEGIN{exit !($l < 6.0)}"; then break; fi
    sleep 20; waited=$((waited+20))
    if [ "$waited" -gt 3600 ]; then echo "charge trop haute depuis 1 h, arret" >> run.log; exit 0; fi
  done
  echo "$(date +%T) start $f K$k load=$l used=$used" >> run.log
  /usr/bin/time -f "%e s %U u %M kB" -o time_${f}_k${k}.txt nice -n 19 ./d3_attrib $D/$f.u32le $k 1 > $out 2> err_${f}_k${k}.txt
  u=$(awk '{print int($3+0.5)}' time_${f}_k${k}.txt)
  used=$((used+u))
  echo "$(date +%T) done $f K$k user=$u used=$used" >> run.log
done
echo "$(date +%T) fin2 used=$used" >> run.log
