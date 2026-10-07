#!/bin/bash
# Toutes les autres portes CTest du depot (hors les deux oracles) contre les mutants BINARIZE et VERT_OPEN.
cd /tmp/v11-audit/l06_code_tour/mutants
: > gates_mutants.txt
for m in BINARIZE VERT_OPEN; do
  cmake --build build_$m --parallel 3 > bld_all_$m.log 2>&1
  echo "$m build complet rc=$?" >> gates_mutants.txt
  PYTHONDONTWRITEBYTECODE=1 ctest --test-dir build_$m -E 'mhgp10_catalogue_oracle|mhgp10_tower_oracle' -j 2 > ctest_$m.log 2>&1
  echo "$m ctest rc=$?" >> gates_mutants.txt
  grep -E "Test +#|tests passed|tests failed" ctest_$m.log >> gates_mutants.txt
done
touch gates_mutants.done
