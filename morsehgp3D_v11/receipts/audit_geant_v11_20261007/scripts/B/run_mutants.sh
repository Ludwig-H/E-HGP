#!/bin/bash
W=/tmp/claude-1000/-workspaces-E-HGP/6acaaf62-b44a-41e7-b5d6-81e4c7b6b7f1/scratchpad/agent_B
export TMPDIR=$W/tmp PYTHONDONTWRITEBYTECODE=1
cd /workspaces/E-HGP/morsehgp3D_v11/reference || exit 9
script=$1; mutant=$2
start=$(date +%s)
out=$(python3 -S -B $script --inject=$mutant 2>&1)
code=$?
last=$(printf '%s\n' "$out" | grep -E "mutant_(killed|survives)" | tail -1)
echo "$script $mutant code=$code sec=$(( $(date +%s)-start )) :: $last"
