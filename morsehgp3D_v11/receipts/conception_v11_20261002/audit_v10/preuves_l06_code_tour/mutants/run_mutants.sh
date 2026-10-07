#!/bin/bash
# Sensibilite de la porte mhgp10_tower_oracle (script du depot, copie sous /tmp) a six mutants causaux de tower.cpp.
# Code attendu d'une porte sensible : 1 (desaccord) ou 3 ; 0 = le mutant survit.
cd /tmp/v11-audit/l06_code_tour/mutants
T=/tmp/v11-audit/l06_code_tour/instr/tests/oracle/test_tower_oracle.py
run(){ local name=$1 build=$2; local t0=$(date +%s); PYTHONDONTWRITEBYTECODE=1 python3 $T $build 12 > oracle_$name.log 2>&1; local rc=$?; local t1=$(date +%s); echo "$name code=$rc duree=$((t1-t0))s : $(grep -E 'tower_oracle_checks' oracle_$name.log) ; premier ecart : $(grep -m1 ECART oracle_$name.log | cut -c1-110)" >> mutants_results.txt; }
: > mutants_results.txt
run TEMOIN /tmp/v11-audit/l06_code_tour/build-release &
run BINARIZE build_BINARIZE &
wait
run POINT_OPEN build_POINT_OPEN &
run VERT_OPEN build_VERT_OPEN &
wait
run DROP_LAST_REP build_DROP_LAST_REP &
run JUMP_ANY build_JUMP_ANY &
wait
run WINDOW_LO build_WINDOW_LO
touch mutants.done
