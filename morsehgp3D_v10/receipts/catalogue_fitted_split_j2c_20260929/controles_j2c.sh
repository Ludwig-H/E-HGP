#!/bin/bash
# Chaine des controles J2c sur le code final (b662673b2 + J2c), en sequence, au plus 4 fils a la fois.
export PYTHONDONTWRITEBYTECODE=1
J=/workspaces/E-HGP/build/v10-j2/j2c
V=/workspaces/E-HGP/build/v10-j2
IN=/workspaces/E-HGP/build/v10-scale-inputs
REF=$V/build-base/mhgp10_catalogue
W=/tmp/j2work
cd $J
log() { echo "$(date +%H:%M:%S) $*" >> $J/controles_j2c.log; }
: > $J/controles_j2c.log
log "debut $(uptime)"
# 1. differentiel, build normal
./differentiel_j2c.sh $J/mhgp10_catalogue.j2c $REF $J/ledger_j2c.jsonl > differentiel_j2c.txt 2>&1; log "differentiel normal : $(tail -1 differentiel_j2c.txt)"
# 2. build empoisonne et differentiel
rm -rf $W/j2c_poison && cmake -S morsehgp3D_v10 -B $W/j2c_poison -DCMAKE_BUILD_TYPE=Release -DMHGP10_POISON=ON > /dev/null && cmake --build $W/j2c_poison --parallel 4 --target mhgp10_catalogue mhgp10_core > /dev/null 2>&1
cp $W/j2c_poison/mhgp10_catalogue $J/mhgp10_catalogue.j2c_poison
./differentiel_j2c.sh $J/mhgp10_catalogue.j2c_poison $REF > differentiel_j2c_poison.txt 2>&1; log "differentiel empoisonne : $(tail -1 differentiel_j2c_poison.txt)"
# 3. niveaux exacts
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Imorsehgp3D_v10/src $V/levelhash.cpp build/libmhgp10_core.a -lpthread -o $W/levelhash_j2c
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -DMHGP10_POISON -Imorsehgp3D_v10/src $V/levelhash.cpp $W/j2c_poison/libmhgp10_core.a -lpthread -o $W/levelhash_j2c_poison
for f in lidar02_full lidar00_full lidar01_quarter_x_neg_y_neg syn_shells_density_x2 syn_filaments_space_x4; do
  for K in 5 10; do
    a=$($W/levelhash_base $IN/$f.u32le $K 4); b=$($W/levelhash_j2c $IN/$f.u32le $K 4); c=$($W/levelhash_j2c $IN/$f.u32le $K 1)
    d=$($W/levelhash_j2c_poison $IN/$f.u32le $K 4)
    [ "$a" = "$b" ] && [ "$b" = "$c" ] && [ "$c" = "$d" ] && r=IDENTIQUES || r=DIFFERENTS
    echo "$f K=$K base4=[$a] j2c4=[$b] j2c1=[$c] j2c_poison4=[$d] $r"
  done
done > niveaux_j2c.txt 2>&1; log "niveaux : $(grep -c IDENTIQUES niveaux_j2c.txt) identiques sur 10"
# 4. entrees degenerees (normal et empoisonne)
./degen_j2c.sh $J/mhgp10_catalogue.j2c $REF 4 > degen_j2c.txt 2>&1; log "degenerees : $(tail -1 degen_j2c.txt)"
./degen_j2c.sh $J/mhgp10_catalogue.j2c_poison $REF 4 > degen_j2c_poison.txt 2>&1; log "degenerees empoisonne : $(tail -1 degen_j2c_poison.txt)"
# 5. ASan et UBSan
rm -rf $W/j2c_asan && cmake -S morsehgp3D_v10 -B $W/j2c_asan -DCMAKE_BUILD_TYPE=Release -DMHGP10_SANITIZE=ON > /dev/null && cmake --build $W/j2c_asan --parallel 4 --target mhgp10_catalogue > /dev/null 2>&1
( for f in $IN/lidar02_quarter_x_nonneg_y_nonneg $IN/syn_shells_density_x1 $W/degen/sphere101 $W/degen/grid10; do
    for K in 5 10; do
      $REF $f.u32le --k=$K --threads=2 --dump=$W/ra.dump > /dev/null; a=$(sha256sum $W/ra.dump | cut -c1-16)
      $W/j2c_asan/mhgp10_catalogue $f.u32le --k=$K --threads=2 --dump=$W/sa.dump > /dev/null 2> $W/sa.err; rc=$?
      b=$(sha256sum $W/sa.dump | cut -c1-16)
      echo "$(basename $f) K=$K ref=$a asan=$b code=$rc stderr_lignes=$(wc -l < $W/sa.err) $([ "$a" = "$b" ] && echo IDENTIQUES || echo DIFFERENTS)"
    done
  done ) > sanitizers_j2c.txt 2>&1; rm -f $W/ra.dump $W/sa.dump; log "asan : $(grep -c 'code=0 stderr_lignes=0 IDENTIQUES' sanitizers_j2c.txt) propres sur 8"
# 6. ThreadSanitizer (setarch -R)
rm -rf $W/j2c_tsan && cmake -S morsehgp3D_v10 -B $W/j2c_tsan -DCMAKE_BUILD_TYPE=Release -DMHGP10_TSAN=ON > /dev/null && cmake --build $W/j2c_tsan --parallel 4 --target mhgp10_catalogue > /dev/null 2>&1
( for K in 5 10; do
    $REF $IN/lidar01_quarter_x_neg_y_neg.u32le --k=$K --threads=3 --dump=$W/rt.dump > /dev/null; a=$(sha256sum $W/rt.dump | cut -c1-16)
    setarch $(uname -m) -R $W/j2c_tsan/mhgp10_catalogue $IN/lidar01_quarter_x_neg_y_neg.u32le --k=$K --threads=4 --dump=$W/st.dump > /dev/null 2> $W/st.err; rc=$?
    b=$(sha256sum $W/st.dump | cut -c1-16)
    echo "quart01 K=$K 4 fils ref=$a tsan=$b code=$rc avertissements_tsan=$(grep -c 'WARNING: ThreadSanitizer' $W/st.err) $([ "$a" = "$b" ] && echo IDENTIQUES || echo DIFFERENTS)"
  done
  setarch $(uname -m) -R python3 $J/morsehgp3D_v10/tests/oracle/test_catalogue_oracle.py $W/j2c_tsan 12 > $W/tsan_oracle.txt 2>&1; rc=$?
  echo "oracle T2 sous TSan (12 nuages, 2 fils par appel) : code $rc ; $(tail -1 $W/tsan_oracle.txt)" ) > tsan_j2c.txt 2>&1
rm -f $W/rt.dump $W/st.dump; log "tsan : $(cat tsan_j2c.txt | tr '\n' ' ' | cut -c1-300)"
# 7. portes
ctest --test-dir build -L gate > ctest_gate_j2c.txt 2>&1
grep -E "catalogue_oracle_checks|tower_oracle_checks" build/Testing/Temporary/LastTest.log >> ctest_gate_j2c.txt
log "portes : $(grep -E 'tests passed|tests failed' ctest_gate_j2c.txt)"
# 8. mutants regeneres depuis le code final
python3 mutants_j2c.py $J/morsehgp3D_v10 > /dev/null && ./mutants_j2c.sh $J/mhgp10_catalogue.j2c m1 m2 m3 > mutants_j2c.txt 2>&1
log "mutants : fini"
log "fin $(uptime)"
echo FINI > $J/controles_done.flag
