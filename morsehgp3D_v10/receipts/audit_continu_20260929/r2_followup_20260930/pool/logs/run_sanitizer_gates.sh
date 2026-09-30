#!/bin/bash
# usage : run_sanitizer_gates.sh asan|tsan
# Portes du pool sur le build sanitizer : mhgp10_unit (tout puis chaque groupe du pool), mhgp10_fault (chaque groupe),
# porte des executables (asan seulement : la bibliotheque LD_PRELOAD n'est pas prevue sous TSan). Codes et sorties
# dans preuves/<tag>/ ; les rapports des sanitizers sont cherches dans stderr.
TAG=$1
B=/tmp/mhgp10-r2/pool/build-$TAG
O=/tmp/mhgp10-r2/pool/preuves/$TAG; mkdir -p $O; cd $O
S=/tmp/mhgp10-r2/pool/src/morsehgp3D_v10
export TMPDIR=/tmp/mhgp10-r2/pool/tmp
rm -f $O/done; : > $O/codes.txt
if [ "$TAG" = tsan ]; then RUN="setarch $(uname -m) -R"; else RUN=""; fi
run() {  # run NOM commande...
  local name=$1; shift
  local s=$(date +%s)
  nice -n 5 timeout 1800 $RUN "$@" > $name.stdout 2> $name.stderr; local c=$?
  local e=$(date +%s)
  local rep=$(grep -cE 'ERROR: AddressSanitizer|ERROR: LeakSanitizer|runtime error|WARNING: ThreadSanitizer|SUMMARY: (Address|Undefined|Thread|Leak)Sanitizer' $name.stderr)
  echo "$name code=$c duree=$((e-s))s rapports_sanitizer=$rep" >> $O/codes.txt
}
run unit_tout $B/mhgp10_unit
for g in pool_caller_exception pool_worker_exception pool_flag_restored pool_cancel_and_reuse pool_claim_wrap pool_short_jobs; do
  run unit_$g $B/mhgp10_unit $g
done
for g in pool_construction entry_points aligned_forms; do run fault_$g $B/mhgp10_fault $g; done
if [ "$TAG" = asan ]; then run worker_bad_alloc python3 $S/tests/regression/test_worker_bad_alloc.py $B; fi
touch $O/done
