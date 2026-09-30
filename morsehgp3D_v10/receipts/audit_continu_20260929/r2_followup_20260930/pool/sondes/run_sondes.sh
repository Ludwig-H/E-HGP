#!/bin/bash
# Sondes des auditeurs et du verificateur rejouees sur le pool r2 (src/sched/pool.cpp de l'etat r2), en Release,
# ASan/UBSan et TSan (setarch -R). Compilation : sonde + pool.cpp seul. Codes dans codes.txt.
# usage : run_sondes.sh SRC_DIR ETIQUETTE
SRC=${1:-/tmp/mhgp10-r2/pool/src/morsehgp3D_v10/src}; TAG=${2:-r2}
D=/tmp/mhgp10-r2/pool/sondes; O=$D/$TAG; mkdir -p $O; cd $O || exit 9
rm -f done; : > codes.txt
W="-std=c++20 -Wall -Wextra -Wpedantic"
declare -A MODE=( [release]="-O2" [asan]="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer" [tsan]="-O1 -g -fsanitize=thread" )
for m in release asan tsan; do
  for p in adv_pool wrap_grain thread_create_failure contre_pool pool_failure_probe pool_throw_borne; do
    nice -n 5 g++ $W ${MODE[$m]} -I$SRC $D/$p.cpp $SRC/sched/pool.cpp -pthread -ldl -o ${p}_$m 2> ${p}_$m.compile.stderr \
      || echo "$p $m compilation_echouee" >> codes.txt
  done
done
run() {  # run MODE NOM args...
  local m=$1 name=$2; shift 2
  local pre=""; [ $m = tsan ] && pre="setarch $(uname -m) -R"
  nice -n 5 timeout 900 $pre ./${name}_$m "$@" > ${name}_${m}${*:+_$(echo "$*" | tr ' ' '_')}.stdout 2> ${name}_${m}${*:+_$(echo "$*" | tr ' ' '_')}.stderr
  local c=$?
  local rep=$(grep -chE 'ERROR: AddressSanitizer|ERROR: LeakSanitizer|runtime error|WARNING: ThreadSanitizer' ${name}_${m}${*:+_$(echo "$*" | tr ' ' '_')}.stderr)
  echo "$m $name $* code=$c rapports_sanitizer=$rep $(tail -1 ${name}_${m}${*:+_$(echo "$*" | tr ' ' '_')}.stdout)" >> codes.txt
}
for m in release asan tsan; do
  run $m adv_pool
  run $m adv_pool g8
  run $m wrap_grain
  for k in 1 2 3; do run $m thread_create_failure $k; done
  run $m contre_pool
  run $m pool_failure_probe
  run $m pool_throw_borne
done
touch done
