#!/usr/bin/env bash
# Auditeur C — campagne des juges d'echantillon q2 et q3 (v4 : coquille exacte, famille haute d'arite
# fixee, ciblage des ancres longues). Compile les deux juges avec une recette fixe, puis les execute
# (nice 19, 2 fils ; hote partage). Chaque cas a un code attendu (0, ou 1 pour les mutants).
# Usage : run_judges_v4.sh <build_v9_release> <boost_include> <dossier_entrees> <sortie>
set -u -o pipefail
BUILD=$1; BOOST_INC=$2; D=$3; O=$4
HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "$HERE/../../.." && pwd)   # racine du depot
mkdir -p "$O"
RECIPE="g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -I$SRC/morsehgp3D_v9 -I$SRC/morsehgp3D_v9/src/gen -isystem $BOOST_INC"
Q2="$O/q2_sample_judge"; Q3="$O/q3_sample_judge"
$RECIPE "$HERE/q2_sample_judge.cpp" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" -lpthread -o "$Q2" || exit 2
$RECIPE "$HERE/q3_sample_judge.cpp" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" -lpthread -o "$Q3" || exit 2
{
  echo "commit=$(git -C "$HERE" rev-parse HEAD)"
  echo "sources_modifiees_non_commitees:"
  git -C "$HERE" status --porcelain -- q2_sample_judge.cpp q3_sample_judge.cpp run_judges_v4.sh
  echo "recette=$RECIPE <source> $BUILD/libmhgp9_chain.a $BUILD/libmhgp9_gen.a -lpthread"
  echo "compilateur=$(g++ --version | head -1)"
  echo "bibliotheques_construites_depuis=$(git -C "$SRC" log -1 --format=%h -- morsehgp3D_v9/src)"
  sha256sum "$HERE/run_judges_v4.sh" "$HERE/q2_sample_judge.cpp" "$HERE/q3_sample_judge.cpp" "$Q2" "$Q3" \
            "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a"
  sha256sum "$D"/s00_k5_s8_w8_r0_nested_8000.u32le "$D"/s01_k5_s8_w8_r0_nested_8000.u32le \
            "$D"/s02_k5_s8_w8_r0_nested_8000.u32le "$D"/scene_00_full.u32le
} > "$O/PROVENANCE.txt"
fail=0
run() {  # run <nom> <code attendu> <commande...>
  local name=$1 want=$2; shift 2
  nice -n 19 "$@" > "$O/$name.txt" 2> "$O/$name.err"
  local c=$?
  echo "exit=$c expected=$want" >> "$O/$name.txt"
  [ "$c" -eq "$want" ] || fail=1
}
L() { echo "$D/$1_k5_s8_w8_r0_nested_8000.u32le"; }
# Fixture d'egalite et mutants (un mutant doit rendre 1).
run q3_fixture_eq_compare 0 "$Q3" fixture-eq --compare
run q3_fixture_eq_overprune 1 "$Q3" fixture-eq --compare --inject=overprune
run q3_fixture_eq_level 1 "$Q3" fixture-eq --inject=level
run q3_fixture_eq_shell_dup 1 "$Q3" fixture-eq --inject=shell-dup
run q2_lidar_s02_8000_k5_level 1 "$Q2" file "$(L s02)" 5 50 2 --inject=level
run q2_lidar_s02_8000_k5_shell_dup 1 "$Q2" file "$(L s02)" 5 50 2 --inject=shell-dup
# Campagne q3, puis --compare cible sur les sites porteurs d'ancres longues trouves a s00/K10.
for s in s00 s01 s02; do run "q3_lidar_${s}_8000_k10" 0 "$Q3" file "$(L $s)" 10 300 2 --min-top=100; done
LONG=$(grep -o 'LONG_SITE a=[0-9]*' "$O/q3_lidar_s00_8000_k10.txt" | head -4 | sed 's/LONG_SITE a=//' | paste -sd, -)
if [ -n "$LONG" ]; then
  run q3_lidar_s00_8000_k10_compare_long 0 "$Q3" file "$(L s00)" 10 0 2 --compare --sites="$LONG"
else
  echo "aucun site long a s00/K10" > "$O/q3_lidar_s00_8000_k10_compare_long.txt"; fail=1
fi
run q3_lidar_s02_8000_k5 0 "$Q3" file "$(L s02)" 5 300 2 --min-top=100
run q3_uniform_8000_k10 0 "$Q3" family uniform 8000 10 100 2 --min-top=100
run q3_lidar_scene00_full_k10 0 "$Q3" file "$D/scene_00_full.u32le" 10 30 2 --min-top=10
# Campagne q2.
for s in s00 s01 s02; do run "q2_lidar_${s}_8000_k10" 0 "$Q2" file "$(L $s)" 10 1000 2 --min-top=100; done
run q2_lidar_s02_8000_k5 0 "$Q2" file "$(L s02)" 5 1000 2 --min-top=100
run q2_uniform_8000_k10 0 "$Q2" family uniform 8000 10 500 2 --min-top=100
run q2_lidar_scene00_full_k10 0 "$Q2" file "$D/scene_00_full.u32le" 10 200 2 --min-top=10
echo "status=$fail" > "$O/STATUS"
exit $fail
