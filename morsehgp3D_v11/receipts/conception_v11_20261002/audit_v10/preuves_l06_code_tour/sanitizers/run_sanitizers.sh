#!/bin/bash
# ASan + UBSan puis TSan du HEAD (build hors source) sur de petites entrees : tour K = 5 et 10, entrees core et cover.
cd /tmp/v11-audit/l06_code_tour
S=/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10
: > sanitizers.txt
cmake -S $S -B build-asan -DCMAKE_BUILD_TYPE=Release -DMHGP10_SANITIZE=ON > cfg_asan.log 2>&1
cmake --build build-asan --parallel 3 --target mhgp10_tower > bld_asan.log 2>&1
echo "build asan+ubsan rc=$?" >> sanitizers.txt
for f in mutants/rand1500 edge/grid6 edge/sphere12 edge/plane_grid12 edge/line20 edge/n1 edge/n2 edge/n3 edge/circle65; do
  for args in "--k=5 --threads=4" "--k=5 --threads=4 --entry=cover" "--k=10 --threads=2 --no-points"; do
    ./build-asan/mhgp10_tower $f.u32le $args > san_out.json 2> san_err.txt
    rc=$?
    echo "asan+ubsan $f $args : code $rc ; lignes stderr $(wc -l < san_err.txt) ; $(cut -c1-60 san_out.json)" >> sanitizers.txt
    if [ -s san_err.txt ]; then head -5 san_err.txt >> sanitizers.txt; fi
  done
done
cmake -S $S -B build-tsan -DCMAKE_BUILD_TYPE=Release -DMHGP10_TSAN=ON > cfg_tsan.log 2>&1
cmake --build build-tsan --parallel 3 --target mhgp10_tower > bld_tsan.log 2>&1
echo "build tsan rc=$?" >> sanitizers.txt
for f in mutants/rand1500 edge/grid6 edge/plane_grid12; do
  for args in "--k=5 --threads=4" "--k=5 --threads=4 --entry=cover" "--k=10 --threads=4"; do
    setarch $(uname -m) -R ./build-tsan/mhgp10_tower $f.u32le $args > san_out.json 2> san_err.txt
    rc=$?
    echo "tsan $f $args : code $rc ; rapports $(grep -c 'WARNING: ThreadSanitizer' san_err.txt) ; $(cut -c1-60 san_out.json)" >> sanitizers.txt
    if [ -s san_err.txt ]; then head -8 san_err.txt >> sanitizers.txt; fi
  done
done
touch sanitizers.done
