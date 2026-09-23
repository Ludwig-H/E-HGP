#!/bin/bash
# Refutation D3 : attribution sur la trame entiere s02 K5 (45 845 sites), un fil, nice 19,
# seulement si la charge 1 min < 6 ; abandon apres 25 min d'attente.
D=/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad
B=$D/design/d3_echelles/d3_attrib
cd $D/design/d3_refut
waited=0
while :; do
  l=$(cut -d' ' -f1 /proc/loadavg)
  if awk "BEGIN{exit !($l < 6.0)}"; then break; fi
  sleep 20; waited=$((waited+20))
  if [ "$waited" -gt 1500 ]; then echo "$(date +%T) charge trop haute ($l), abandon" >> run.log; touch done.flag; exit 0; fi
done
echo "$(date +%T) start scene_02_full K5 load=$l" >> run.log
/usr/bin/time -f "%e s %U u %M kB" -o time_scene_02_full_k5.txt timeout 420 nice -n 19 $B $D/data/scene_02_full.u32le 5 1 > attrib_scene_02_full_k5.json 2> err_scene_02_full_k5.txt
echo "$(date +%T) done rc=$? $(cat time_scene_02_full_k5.txt)" >> run.log
touch done.flag
