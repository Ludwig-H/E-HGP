#!/bin/bash
# Mutant du pool : NOM.diff applique a une copie de src/sched/pool.cpp (etat r2), compile avec les options Release de
# CMake, membre pool.cpp.o remplace dans une copie de libmhgp10_core.a, mhgp10_unit et mhgp10_fault relies depuis les
# objets de test r2, puis chaque porte du pool jouee avec un delai (124 = delai atteint, >128 = signal).
# usage : run_pool_mutant.sh NOM
M=/tmp/mhgp10-r2/pool/mutants
S=/tmp/mhgp10-r2/pool/src/morsehgp3D_v10
B=/tmp/mhgp10-r2/pool/build
NAME=$1
D=$M/$NAME; rm -rf $D; mkdir -p $D; cd $D || exit 9
cp $S/src/sched/pool.cpp pool.cpp
patch -s pool.cpp < $M/$NAME.diff || { echo "$NAME patch_echoue" > codes.txt; exit 9; }
F="-O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror"
nice -n 5 c++ $F -I $S/src -c pool.cpp -o pool.cpp.o 2> compile.stderr || { echo "$NAME compilation_echouee" > codes.txt; exit 9; }
cp $B/libmhgp10_core.a libmhgp10_core.a && ar r libmhgp10_core.a pool.cpp.o
nice -n 5 c++ -O3 -DNDEBUG $B/CMakeFiles/mhgp10_unit.dir/tests/unit/unit_main.cpp.o libmhgp10_core.a -pthread -o mhgp10_unit
nice -n 5 c++ -O3 -DNDEBUG $B/CMakeFiles/mhgp10_fault.dir/tests/unit/fault_main.cpp.o libmhgp10_core.a -pthread -o mhgp10_fault
: > codes.txt
for g in pool_claim_wrap pool_short_jobs pool_caller_exception pool_worker_exception pool_flag_restored pool_cancel_and_reuse; do
  timeout 120 ./mhgp10_unit $g > unit_$g.stdout 2> unit_$g.stderr; echo "mhgp10_unit $g $?" >> codes.txt
done
for g in pool_construction entry_points aligned_forms; do
  timeout 300 ./mhgp10_fault $g > fault_$g.stdout 2> fault_$g.stderr; echo "mhgp10_fault $g $?" >> codes.txt
done
rm -f libmhgp10_core.a pool.cpp.o
