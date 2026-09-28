#!/bin/bash
# Audit L02 : campagne locale du grand-livre q2 (W2, nice 19), hote partage 8 coeurs.
set -u
D=/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L02_gen_q2
W=/workspaces/E-HGP/build/v9-open-worktree/build/v9-scaling/work
S=/workspaces/E-HGP/build/v9-q3-payload-scaling-inputs-20260926
OUT=$D/campaign.jsonl
: > $OUT
for sc in s00 s02; do for k in 5 10; do for n in 8000 16000 32000; do
  f=$W/${sc}_k5_s8_w8_r0_nested_${n}.u32le
  for c in prod w1 w4all nosib dfs nopool bare; do
    nice -n 19 $D/q2_ledger_probe $f $k $c 2 >> $OUT 2>>$D/campaign.err
  done
done; done; done
for fam in uniform clusters terrain; do for k in 5 10; do for n in 8000 16000 32000; do
  nice -n 19 $D/q2_ledger_probe $S/${fam}_${n}.u32le $k prod 2 >> $OUT 2>>$D/campaign.err
done; done; done
for sc in s00 s02; do for k in 5 10; do for n in 8000 16000 32000; do
  nice -n 19 $D/q2_ledger_probe $W/${sc}_k5_s8_w8_r0_nested_${n}.u32le $k prod 2 knn >> $D/knn.jsonl 2>>$D/campaign.err
done; done; done
echo CAMPAIGN_DONE >> $D/campaign.err
