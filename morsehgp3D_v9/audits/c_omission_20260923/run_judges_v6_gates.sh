#!/usr/bin/env bash
# Auditeur C — portes des juges q2/q3 (v6, apres la contrelecture B du lanceur v5) : fixture d'egalite,
# mutants a code ET marqueur causal exiges, --compare sur les sites isoles avec plancher d'incidences
# longues et mutant drop-long exige. Provenance : chaque valeur affectee puis controlee avant ecriture ;
# dossier de sortie neuf ; chaque ecriture verifiee ; echec d'infrastructure distinct du code du juge.
# Usage : run_judges_v6_gates.sh <build_v9_release> <boost_include> <dossier_entrees> <sortie_neuve>
#         run_judges_v6_gates.sh --selftest      (le lanceur doit echouer sur provenance et sortie fautives)
set -u -o pipefail
die() { echo "infrastructure: $*" >&2; exit 2; }
provenance() {  # provenance <racine_depot> <fichier> ; aucune substitution non controlee
  local src=$1 out=$2 commit srccommit st
  commit=$(git -C "$src" rev-parse HEAD) || return 1
  [ -n "$commit" ] || return 1
  srccommit=$(git -C "$src" log -1 --format=%H -- morsehgp3D_v9/src) || return 1
  [ -n "$srccommit" ] || return 1
  st=$(git -C "$src" status --porcelain -- morsehgp3D_v9/src) || return 1
  [ -z "$st" ] || return 1
  printf 'commit=%s\nsrc_commit=%s\n' "$commit" "$srccommit" > "$out" || return 1
}
fail=0
run() {  # run <nom> <attendu: 0|1|obs> <marqueur causal ou -> <commande...>
  local name=$1 want=$2 marker=$3; shift 3
  : > "$O/$name.txt" || die "sortie $name"
  nice -n 19 "$@" >> "$O/$name.txt" 2> "$O/$name.err"
  local c=$?
  printf 'exit=%s expected=%s\n' "$c" "$want" >> "$O/$name.txt" || die "ecriture $name"
  grep -q ' kmax=' "$O/$name.txt" || { echo "$name : pas de ligne de synthese (infrastructure)" >&2; fail=1; return; }
  if [ "$want" = obs ]; then return; fi
  if [ "$c" -ne "$want" ]; then fail=1; return; fi
  if [ "$marker" != - ] && ! grep -q -- "$marker" "$O/$name.txt"; then
    echo "$name : code attendu sans marqueur causal $marker" >&2; fail=1
  fi
}
if [ "${1:-}" = "--selftest" ]; then
  tmp=$(mktemp -d) || die "mktemp"
  if provenance "$tmp" "$tmp/p.txt" 2>/dev/null; then echo "selftest: provenance hors depot acceptee"; exit 1; fi
  # run() sur une sortie impossible : l'infrastructure doit echouer (code 2), jamais un mutant « tue ».
  ( O=/nonexistent_dir_mhgp9; run t 1 - true ) 2>/dev/null
  rc=$?
  [ "$rc" -eq 2 ] || { echo "selftest: run() sur sortie impossible a rendu $rc"; exit 1; }
  # Un mutant qui rend 1 sans marqueur causal doit faire echouer la porte.
  ( O=$tmp; fail=0; run t 1 CAUSE bash -c 'echo " kmax=5"; exit 1'; exit $fail ) 2>/dev/null
  rc=$?
  [ "$rc" -eq 1 ] || { echo "selftest: mutant sans marqueur accepte ($rc)"; exit 1; }
  rm -rf "$tmp"; echo "selftest ok"; exit 0
fi
BUILD=$1; BOOST_INC=$2; D=$3; O=$4
HERE=$(cd "$(dirname "$0")" && pwd) || die "chemin du lanceur"
SRC=$(cd "$HERE/../../.." && pwd) || die "racine du depot"
[ ! -e "$O" ] || die "dossier de sortie deja present : $O"
mkdir -p "$O" || die "mkdir $O"
home=$(grep '^CMAKE_HOME_DIRECTORY:' "$BUILD/CMakeCache.txt") || die "CMakeCache illisible"
[ "${home#*=}" = "$SRC/morsehgp3D_v9" ] || die "build hors depot (${home#*=})"
provenance "$SRC" "$O/PROVENANCE.txt" || die "provenance git"
nice -n 19 cmake --build "$BUILD" --parallel 3 --target mhgp9_chain mhgp9_gen > "$O/build_libs.log" 2>&1 || die "build bibliotheques"
RECIPE="g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -I$SRC/morsehgp3D_v9 -I$SRC/morsehgp3D_v9/src/gen -isystem $BOOST_INC"
Q2="$O/q2_sample_judge"; Q3="$O/q3_sample_judge"
$RECIPE "$HERE/q2_sample_judge.cpp" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" -lpthread -o "$Q2" || die "compilation q2"
$RECIPE "$HERE/q3_sample_judge.cpp" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" -lpthread -o "$Q3" || die "compilation q3"
st=$(git -C "$HERE" status --porcelain -- q2_sample_judge.cpp q3_sample_judge.cpp run_judges_v6_gates.sh) || die "git status juges"
cc=$(g++ --version | head -1) || die "g++ --version"
printf 'sources_juges_non_commitees=%s\nrecette=%s <source> %s %s -lpthread\ncompilateur=%s\n' \
  "${st:-aucune}" "$RECIPE" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" "$cc" >> "$O/PROVENANCE.txt" || die "ecriture provenance"
sha256sum "$HERE/run_judges_v6_gates.sh" "$HERE/q2_sample_judge.cpp" "$HERE/q3_sample_judge.cpp" "$Q2" "$Q3" \
  "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" "$D"/s00_k5_s8_w8_r0_nested_8000.u32le \
  "$D"/s02_k5_s8_w8_r0_nested_8000.u32le >> "$O/PROVENANCE.txt" || die "empreintes"
L() { echo "$D/$1_k5_s8_w8_r0_nested_8000.u32le"; }
run q3_fixture_eq_compare 0 - "$Q3" fixture-eq --compare
run q3_fixture_eq_overprune 1 PRUNE_DISAGREES "$Q3" fixture-eq --compare --inject=overprune
run q3_fixture_eq_level 1 "MISSING a=" "$Q3" fixture-eq --inject=level
run q3_fixture_eq_shell_dup 1 "MISSING a=" "$Q3" fixture-eq --inject=shell-dup
run q3_fixture_eq_key 1 "MISSING a=" "$Q3" fixture-eq --inject=key
run q2_lidar_s02_8000_k5_level 1 "MISSING a=" "$Q2" file "$(L s02)" 5 50 2 --inject=level
run q2_lidar_s02_8000_k5_shell_dup 1 "MISSING a=" "$Q2" file "$(L s02)" 5 50 2 --inject=shell-dup
run q2_lidar_s02_8000_k5_key 1 "MISSING a=" "$Q2" file "$(L s02)" 5 50 2 --inject=key
for s in s00 s02; do
  run "q3_lidar_${s}_8000_k10_compare_isolated" 0 - "$Q3" file "$(L $s)" 10 0 2 --compare --long-sites=4 --min-long=50
  run "q3_lidar_${s}_8000_k10_drop_long_isolated" 1 PRUNE_DISAGREES "$Q3" file "$(L $s)" 10 0 2 --compare --long-sites=4 --min-long=50 --inject=drop-long
  run "q3_lidar_${s}_8000_k10_overprune_isolated" obs - "$Q3" file "$(L $s)" 10 0 2 --compare --long-sites=4 --inject=overprune
done
printf 'status=%d\n' "$fail" > "$O/STATUS" || die "ecriture STATUS"
exit $fail
