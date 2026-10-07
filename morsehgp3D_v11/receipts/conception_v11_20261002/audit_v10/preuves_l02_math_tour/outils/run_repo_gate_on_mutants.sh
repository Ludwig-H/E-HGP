#!/bin/bash
# Porte enregistree du depot (tests/oracle/test_tower_oracle.py, 24 nuages par defaut) sur chaque mutant.
cd /tmp/v11-audit/l02_math_tour
run_one() {
  m=$1
  ( /usr/bin/time -f "wall=%e s" nice -n 10 python3 -B v10src/tests/oracle/test_tower_oracle.py mutants/$m/build ) > mutants/$m/repo_gate.log 2>&1
  echo "exit=$?" >> mutants/$m/repo_gate.log
}
export -f run_one
printf "%s\n" m_bin m_seq m_vopen m_onepiece m_strict m_vnoclimb | xargs -P 3 -I{} bash -c 'run_one {}'
touch mutants/repo_gate.done
