#!/bin/bash
W=/tmp/claude-1000/-workspaces-E-HGP/6acaaf62-b44a-41e7-b5d6-81e4c7b6b7f1/scratchpad/agent_B
export TMPDIR=$W/tmp PYTHONDONTWRITEBYTECODE=1
cd /workspaces/E-HGP/morsehgp3D_v11/reference || exit 9
( python3 -S -B -O test_ref.py --suite=fast > $W/ref_fast_O.out 2>&1; echo "code=$?" >> $W/ref_fast_O.out ) &
( python3 -S -B -O test_supports.py > $W/supports_O.out 2>&1; echo "code=$?" >> $W/supports_O.out ) &
( python3 -S -B -O test_projection_contracts.py > $W/proj_O.out 2>&1; echo "code=$?" >> $W/proj_O.out ) &
( python3 -S -B test_ref.py --suite=fast --jobs=3 > $W/ref_fast_split.out 2>&1; echo "code=$?" >> $W/ref_fast_split.out ) &
wait
