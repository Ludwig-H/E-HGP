#!/bin/bash
# L04 audit : les mutants de geometrie qui survivent a mhgp10_unit sont-ils tues par les portes d'oracle T2 ?
# Chaque mutant : copie de l'arbre sous /tmp, une ligne modifiee, build complet (-j3), oracles reduits.
SRC=/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10
W=/tmp/v11-audit/l04_code_fondations/mutants
apply() {  # nom fichier ancien nouveau
  rm -rf "$W/full_$1" "$W/build_$1"
  mkdir -p "$W/full_$1"
  cp -r "$SRC/src" "$SRC/cli" "$SRC/cmake" "$SRC/CMakeLists.txt" "$SRC/tests" "$SRC/reference" "$W/full_$1/"
  python3 - "$W/full_$1/$2" "$3" "$4" <<'PY'
import sys
p, old, new = sys.argv[1:4]
s = open(p).read()
if old not in s:
    print('MUTANT INAPPLICABLE'); sys.exit(2)
open(p, 'w').write(s.replace(old, new))
PY
  cmake -S "$W/full_$1" -B "$W/build_$1" -DCMAKE_BUILD_TYPE=Release > "$W/build_$1.log" 2>&1
  cmake --build "$W/build_$1" -j3 --target mhgp10_catalogue mhgp10_tower >> "$W/build_$1.log" 2>&1 || { echo "$1 : ne compile pas"; return; }
  timeout 600 python3 -B "$W/full_$1/tests/oracle/test_catalogue_oracle.py" "$W/build_$1" 10 > "$W/oracle_cat_$1.out" 2>&1; rc1=$?
  timeout 600 python3 -B "$W/full_$1/tests/oracle/test_tower_oracle.py" "$W/build_$1" 8 > "$W/oracle_tow_$1.out" 2>&1; rc2=$?
  echo "$1 : oracle catalogue code=$rc1 ($(tail -1 "$W/oracle_cat_$1.out")) ; oracle tour code=$rc2 ($(tail -1 "$W/oracle_tow_$1.out"))"
  rm -rf "$W/full_$1" "$W/build_$1"
}
apply M0_temoin src/arith/geometry.cpp 'return acc.sign();' 'return acc.sign();'
apply M2_center3_D src/arith/geometry.hpp 'out.D = 2 * (i128(w.x) * w.x + i128(w.y) * w.y + i128(w.z) * w.z);' 'out.D = 4 * (i128(w.x) * w.x + i128(w.y) * w.y + i128(w.z) * w.z);'
apply M3_center4_signe src/arith/geometry.hpp 'i128 N0 = uu * vsx + vv * sux + ss * uvx,' 'i128 N0 = uu * vsx - vv * sux + ss * uvx,'
apply M4_level3_den src/arith/geometry.cpp 'l.den = arith::I128w::from_u128(4 * ww);' 'l.den = arith::I128w::from_u128(2 * ww);'
apply M5_orient_center_wide src/arith/geometry.cpp 'return acc.sign();' 'return -acc.sign();'
apply M6_compare src/arith/geometry.cpp 'return arith::cmp(arith::mul(a.num, b.den), arith::mul(b.num, a.den));' 'return arith::cmp(arith::mul(a.num, a.den), arith::mul(b.num, b.den));'
apply M14_morton_axes src/cloud/cloud.cpp 'return spread21(x) | (spread21(y) << 1) | (spread21(z) << 2);' 'return spread21(x) | (spread21(y) << 2) | (spread21(z) << 1);'
echo FIN
