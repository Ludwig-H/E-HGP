#!/bin/bash
# Non-vacuite de l'oracle : chaque pont compile contre un header MUTE doit produire des divergences.
S=/tmp/claude-1000/-workspaces-E-HGP/6300a9ba-5bd7-42ea-91b3-d2c07c163916/scratchpad/wf/oracle_differentiel
cd $S
for m in drop_continuation future_contribution final_root parents_zero open_as_closed birth_size_lax continuation_new_node dedup_across_roots; do
  rm -rf mut_$m; mkdir -p mut_$m
  PYTHONDONTWRITEBYTECODE=1 python3 -B run_oracle.py --bridge build/bridge_$m --out mut_$m --valid 60 --invalid 60 > mut_$m/report.json 2> mut_$m/stderr.txt
  echo "$m code=$?"
done
