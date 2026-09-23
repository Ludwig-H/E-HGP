#!/usr/bin/env bash
# Auditeur C — campagne des juges d'echantillon q2 et q3 durcis (nice 19, 2 fils ; hote partage).
# Usage : run_judges_v3.sh <q2_bin> <q3_bin> <dossier_entrees> <sortie>
# Chaque cas a un code attendu (0, ou 1 pour les mutants) ; tout ecart rend STATUS non nul.
# Provenance : commit, sources non commitees, SHA-256 des sources, des binaires et des entrees.
set -u -o pipefail
Q2=$1; Q3=$2; D=$3; O=$4
HERE=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$O"
{
  echo "commit=$(git -C "$HERE" rev-parse HEAD)"
  echo "sources_modifiees_non_commitees:"; git -C "$HERE" status --porcelain -- q2_sample_judge.cpp q3_sample_judge.cpp
  sha256sum "$HERE/q2_sample_judge.cpp" "$HERE/q3_sample_judge.cpp" "$Q2" "$Q3"
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
# Fixture d'egalite et mutants (le mutant doit rendre 1).
run q3_fixture_eq_compare 0 "$Q3" fixture-eq --compare
run q3_fixture_eq_overprune 1 "$Q3" fixture-eq --compare --inject=overprune
run q3_fixture_eq_level 1 "$Q3" fixture-eq --inject=level
run q2_lidar_s02_8000_k5_level 1 "$Q2" file "$(L s02)" 5 50 2 --inject=level
# Elagage compare a la force brute sur donnees LiDAR.
run q3_lidar_s02_8000_k10_compare 0 "$Q3" file "$(L s02)" 10 3 2 --compare
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
