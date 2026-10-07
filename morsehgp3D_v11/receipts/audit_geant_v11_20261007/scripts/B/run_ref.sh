#!/bin/bash
W=/tmp/claude-1000/-workspaces-E-HGP/6acaaf62-b44a-41e7-b5d6-81e4c7b6b7f1/scratchpad/agent_B
export TMPDIR=$W/tmp PYTHONDONTWRITEBYTECODE=1
cd /workspaces/E-HGP/morsehgp3D_v11/reference || exit 9
name=$1; shift
start=$(date +%s)
python3 -S -B "$@" > $W/$name.out 2>&1
code=$?
end=$(date +%s)
echo "code=$code seconds=$((end-start))" >> $W/$name.out
