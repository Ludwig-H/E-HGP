#!/usr/bin/env bash
# Auditeur C — campagne des juges d'echantillon q2 et q3 (v5 : cle canonique comparee, sites longs choisis
# depuis les seules coordonnees, provenance bloquante). Reconstruit les bibliotheques v9 depuis ce depot,
# compile les deux juges avec une recette fixe, puis les execute (nice 19, 2 fils ; hote partage).
# Chaque cas a un code attendu (0, ou 1 pour les mutants) ; les cas « observes » n'ont pas d'attendu.
# Usage : run_judges_v5.sh <build_v9_release> <boost_include> <dossier_entrees> <sortie>
set -u -o pipefail
BUILD=$1; BOOST_INC=$2; D=$3; O=$4
HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "$HERE/../../.." && pwd)   # racine du depot
mkdir -p "$O" || exit 2
die() { echo "provenance: $*" >&2; echo "status=2 ($*)" > "$O/STATUS"; exit 2; }
# Lien bibliotheques <-> sources : le build pointe sur ce depot, src/ est propre, reconstruction imposee.
home=$(grep '^CMAKE_HOME_DIRECTORY:' "$BUILD/CMakeCache.txt" | cut -d= -f2) || die "CMakeCache illisible"
[ "$home" = "$SRC/morsehgp3D_v9" ] || die "build hors depot ($home)"
[ -z "$(git -C "$SRC" status --porcelain -- morsehgp3D_v9/src)" ] || die "morsehgp3D_v9/src modifie"
nice -n 19 cmake --build "$BUILD" --parallel 3 --target mhgp9_chain mhgp9_gen > "$O/build_libs.log" 2>&1 || die "build bibliotheques"
RECIPE="g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -I$SRC/morsehgp3D_v9 -I$SRC/morsehgp3D_v9/src/gen -isystem $BOOST_INC"
Q2="$O/q2_sample_judge"; Q3="$O/q3_sample_judge"
$RECIPE "$HERE/q2_sample_judge.cpp" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" -lpthread -o "$Q2" || die "compilation q2"
$RECIPE "$HERE/q3_sample_judge.cpp" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" -lpthread -o "$Q3" || die "compilation q3"
{
  echo "commit=$(git -C "$SRC" rev-parse HEAD)" || exit 1
  echo "src_commit=$(git -C "$SRC" log -1 --format=%H -- morsehgp3D_v9/src)" || exit 1
  echo "sources_juges_non_commitees:"; git -C "$HERE" status --porcelain -- q2_sample_judge.cpp q3_sample_judge.cpp run_judges_v5.sh || exit 1
  echo "recette=$RECIPE <source> $BUILD/libmhgp9_chain.a $BUILD/libmhgp9_gen.a -lpthread"
  echo "compilateur=$(g++ --version | head -1)"
  sha256sum "$HERE/run_judges_v5.sh" "$HERE/q2_sample_judge.cpp" "$HERE/q3_sample_judge.cpp" "$Q2" "$Q3" \
            "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" || exit 1
  sha256sum "$D"/s00_k5_s8_w8_r0_nested_8000.u32le "$D"/s01_k5_s8_w8_r0_nested_8000.u32le \
            "$D"/s02_k5_s8_w8_r0_nested_8000.u32le "$D"/scene_00_full.u32le || exit 1
} > "$O/PROVENANCE.txt" || die "bloc de provenance incomplet"
fail=0
run() {  # run <nom> <code attendu | obs> <commande...>
  local name=$1 want=$2; shift 2
  nice -n 19 "$@" > "$O/$name.txt" 2> "$O/$name.err"
  local c=$?
  echo "exit=$c expected=$want" >> "$O/$name.txt"
  if [ "$want" != obs ] && [ "$c" -ne "$want" ]; then fail=1; fi
}
L() { echo "$D/$1_k5_s8_w8_r0_nested_8000.u32le"; }
# Fixture d'egalite et mutants (un mutant doit rendre 1).
run q3_fixture_eq_compare 0 "$Q3" fixture-eq --compare
run q3_fixture_eq_overprune 1 "$Q3" fixture-eq --compare --inject=overprune
run q3_fixture_eq_level 1 "$Q3" fixture-eq --inject=level
run q3_fixture_eq_shell_dup 1 "$Q3" fixture-eq --inject=shell-dup
run q3_fixture_eq_key 1 "$Q3" fixture-eq --inject=key
run q2_lidar_s02_8000_k5_level 1 "$Q2" file "$(L s02)" 5 50 2 --inject=level
run q2_lidar_s02_8000_k5_shell_dup 1 "$Q2" file "$(L s02)" 5 50 2 --inject=shell-dup
run q2_lidar_s02_8000_k5_key 1 "$Q2" file "$(L s02)" 5 50 2 --inject=key
# Elagage contre force brute sur les 4 sites les plus isoles (choisis depuis les coordonnees), K10 ;
# le mutant de sur-elagage y est seulement observe (la fixture d'egalite est sa porte).
for s in s00 s02; do
  run "q3_lidar_${s}_8000_k10_compare_isolated" 0 "$Q3" file "$(L $s)" 10 0 2 --compare --long-sites=4
  run "q3_lidar_${s}_8000_k10_overprune_isolated" obs "$Q3" file "$(L $s)" 10 0 2 --compare --long-sites=4 --inject=overprune
done
# Campagnes.
for s in s00 s01 s02; do run "q3_lidar_${s}_8000_k10" 0 "$Q3" file "$(L $s)" 10 300 2 --min-top=100; done
run q3_lidar_s02_8000_k5 0 "$Q3" file "$(L s02)" 5 300 2 --min-top=100
run q3_uniform_8000_k10 0 "$Q3" family uniform 8000 10 100 2 --min-top=100
run q3_lidar_scene00_full_k10 0 "$Q3" file "$D/scene_00_full.u32le" 10 30 2 --min-top=10
for s in s00 s01 s02; do run "q2_lidar_${s}_8000_k10" 0 "$Q2" file "$(L $s)" 10 1000 2 --min-top=100; done
run q2_lidar_s02_8000_k5 0 "$Q2" file "$(L s02)" 5 1000 2 --min-top=100
run q2_uniform_8000_k10 0 "$Q2" family uniform 8000 10 500 2 --min-top=100
run q2_lidar_scene00_full_k10 0 "$Q2" file "$D/scene_00_full.u32le" 10 200 2 --min-top=10
echo "status=$fail" > "$O/STATUS"
exit $fail
