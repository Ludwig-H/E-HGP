#!/bin/bash
# Stress du cas de 18 points coplanaires a K = 1 : BIN N_RUNS THREADS -> nombre de refus (sortie autre que "ok").
# Lance en 6 copies paralleles pour charger la machine : for j in 1 2 3 4 5 6; do ./stress18.sh BIN 400 2 & done; wait
BIN=$1; N=$2; T=$3; F=$(dirname "$0")/cas18_coplanaire_k1.u32le
bad=0
for i in $(seq $N); do
  out=$($BIN $F --k=1 --threads=$T 2>&1)
  case "$out" in *'"status":"ok"'*) ;; *) bad=$((bad+1)); echo "$out" | head -c 150; echo;; esac
done
echo "threads=$T runs=$N refus=$bad"
