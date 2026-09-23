#!/usr/bin/env bash
# Auditeur C — portes des juges q2/q3 (v8, apres la contrelecture B du juge v7) : les cas v7 inchanges, plus
# la garde d'index (PointId < n, bijection, positions) et ses trois mutants, refuses en code 2 avec leur
# marqueur INDEX_* et SANS ligne de synthese (refus avant l'echantillonnage) ; stderr de chaque cas, journaux
# de compilation des juges et sortie du --selftest du lanceur sont archives avec le recu.
# Rappel v7 : fixture d'egalite, mutants a code ET marqueur causal exiges, --compare sur les huit sites les
# plus isoles de s00, s01 et s02 avec plancher de la strate CRL (p = Kmax-2, coquille de 3 sites, q_min = 3,
# arete max >= 1600 unites) compte dans le parcours brut, mutant drop-crl tue dans cette strate.
# Provenance : chaque valeur affectee puis controlee avant ecriture ; dossier de sortie neuf ; chaque ecriture
# verifiee ; tout code >= 2 d'un juge est un echec, meme pour une observation.
# Usage : run_judges_v8_gates.sh <build_v9_release> <boost_include> <dossier_entrees> <sortie_neuve>
#         run_judges_v8_gates.sh --selftest      (le lanceur doit echouer sur provenance et sorties fautives)
# Entrees : regen_inputs.py les regenere depuis les trames versionnees du recu v8 (empreintes controlees).
set -u -o pipefail
die() { echo "infrastructure: $*" >&2; exit 2; }
provenance() {  # provenance <racine_depot> <fichier> ; aucune substitution non controlee
  local src=$1 out=$2 commit srccommit st
  commit=$(git -C "$src" rev-parse HEAD) || return 1
  [ -n "$commit" ] || return 1
  srccommit=$(git -C "$src" log -1 --format=%H -- morsehgp3D_v9/src) || return 1
  [ -n "$srccommit" ] || return 1
  st=$(git -C "$src" status --porcelain -- morsehgp3D_v9/src morsehgp3D_v9/tests/gen morsehgp3D_v9/CMakeLists.txt) || return 1
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
  # Un code >= 2 (refus avant calcul, chaine incomplete, infrastructure) n'est jamais une observation valide.
  if [ "$c" -ge 2 ]; then echo "$name : code $c (refus ou infrastructure)" >&2; fail=1; return; fi
  grep -Eq ' n=[0-9]+ kmax=' "$O/$name.txt" || { echo "$name : pas de ligne de synthese (infrastructure)" >&2; fail=1; return; }
  if [ "$want" = obs ]; then return; fi
  if [ "$c" -ne "$want" ]; then fail=1; return; fi
  if [ "$marker" != - ] && ! grep -q -- "$marker" "$O/$name.txt"; then
    echo "$name : code attendu sans marqueur causal $marker" >&2; fail=1
  fi
}
refusal() {  # refusal <nom> <marqueur INDEX_*> <commande...> : code 2 exact, marqueur, aucune synthese
  local name=$1 marker=$2; shift 2
  : > "$O/$name.txt" || die "sortie $name"
  nice -n 19 "$@" >> "$O/$name.txt" 2> "$O/$name.err"
  local c=$?
  printf 'exit=%s expected=2\n' "$c" >> "$O/$name.txt" || die "ecriture $name"
  if [ "$c" -ne 2 ]; then echo "$name : code $c au lieu du refus 2" >&2; fail=1; return; fi
  grep -q -- "$marker" "$O/$name.txt" || { echo "$name : refus sans marqueur $marker" >&2; fail=1; return; }
  if grep -Eq ' n=[0-9]+ kmax=' "$O/$name.txt"; then echo "$name : synthese emise, refus apres le jugement" >&2; fail=1; fi
}
if [ "${1:-}" = "--selftest" ]; then
  tmp=$(mktemp -d) || die "mktemp"
  if provenance "$tmp" "$tmp/p.txt" 2>/dev/null; then echo "selftest: provenance hors depot acceptee"; exit 1; fi
  # run() sur une sortie impossible : l'infrastructure doit echouer (code 2), jamais un mutant « tue ».
  ( O=/nonexistent_dir_mhgp9; run t 1 - true ) 2>/dev/null
  rc=$?
  [ "$rc" -eq 2 ] || { echo "selftest: run() sur sortie impossible a rendu $rc"; exit 1; }
  # Chaque cas porte une vraie ligne de synthese : seul le controle vise peut le refuser.
  SYN='x n=8 kmax=5 mode=compare'
  st_case() {  # st_case <nom> <attendu fail> <want> <marqueur> <sortie> <code>
    local name=$1 wantfail=$2 want=$3 marker=$4 out=$5 code=$6 rc2
    ( O=$tmp; fail=0; run "$name" "$want" "$marker" bash -c "echo '$out'; exit $code"; exit $fail ) 2>/dev/null
    rc2=$?
    [ "$rc2" -eq "$wantfail" ] || { echo "selftest: cas $name a rendu fail=$rc2 au lieu de $wantfail"; exit 1; }
  }
  st_case p0 0 0 - "$SYN" 0                 # temoin positif : sortie conforme acceptee
  st_case p1 1 1 CAUSE "$SYN" 1             # mutant sans marqueur causal refuse
  st_case p1b 0 1 CAUSE "$SYN CAUSE" 1      # mutant avec marqueur accepte
  st_case p2 1 obs - "$SYN" 2               # observation en code 2 refusee (controle >= 2)
  st_case p3 1 0 - "$SYN" 1                 # code different de l'attendu refuse
  st_case p4 1 obs - "x kmax=10" 0          # observation sans ligne de synthese refusee
  st_case p5 0 obs - "$SYN" 1               # observation valide en code 1 acceptee
  grep -q 'exit=2 expected=obs' "$tmp/p2.txt" || { echo "selftest: journal du refus p2 absent"; exit 1; }
  rf_case() {  # rf_case <nom> <attendu fail> <marqueur> <sortie> <code>
    local name=$1 wantfail=$2 marker=$3 out=$4 code=$5 rc2
    ( O=$tmp; fail=0; refusal "$name" "$marker" bash -c "echo '$out'; exit $code"; exit $fail ) 2>/dev/null
    rc2=$?
    [ "$rc2" -eq "$wantfail" ] || { echo "selftest: refus $name a rendu fail=$rc2 au lieu de $wantfail"; exit 1; }
  }
  rf_case r0 0 INDEX_X "x INDEX_X u=1" 2        # temoin : refus 2 avec marqueur et sans synthese accepte
  rf_case r1 1 INDEX_X "x autre" 2              # refus sans marqueur refuse
  rf_case r2 1 INDEX_X "x INDEX_X $SYN" 2       # refus apres synthese (jugement deja fait) refuse
  rf_case r3 1 INDEX_X "x INDEX_X" 1            # code 1 au lieu de 2 refuse
  rf_case r4 1 INDEX_X "x INDEX_X" 0            # code 0 refuse
  rf_case r5 1 INDEX_X "x INDEX_X" 3            # code 3 refuse
  rm -rf "$tmp"; echo "selftest ok"; exit 0
fi
BUILD=$1; BOOST_INC=$2; D=$3; O=$4
HERE=$(cd "$(dirname "$0")" && pwd) || die "chemin du lanceur"
SRC=$(cd "$HERE/../../.." && pwd) || die "racine du depot"
[ ! -e "$O" ] || die "dossier de sortie deja present : $O"
mkdir -p "$O" || die "mkdir $O"
home=$(grep '^CMAKE_HOME_DIRECTORY:' "$BUILD/CMakeCache.txt") || die "CMakeCache illisible"
[ "${home#*=}" = "$SRC/morsehgp3D_v9" ] || die "build hors depot (${home#*=})"
btype=$(grep '^CMAKE_BUILD_TYPE:' "$BUILD/CMakeCache.txt") || die "CMAKE_BUILD_TYPE absent"
[ "${btype#*=}" = "Release" ] || die "build non Release (${btype#*=})"
"$HERE/run_judges_v8_gates.sh" --selftest > "$O/selftest.txt" 2>&1 || die "selftest du lanceur"
python3 "$HERE/regen_inputs.py" --check "$D" > "$O/inputs_check.txt" 2>&1 || die "entrees non conformes a regen_inputs.EXPECTED"
provenance "$SRC" "$O/PROVENANCE.txt" || die "provenance git"
# Depuis S2 (a6d81f9c), mhgp9_chain depend de mhgp9_gpu (lanceur CUDA ou stub sans CUDA).
GPU_LIB=""
if grep -q 'mhgp9_gpu' "$SRC/morsehgp3D_v9/CMakeLists.txt"; then
  nice -n 19 cmake --build "$BUILD" --parallel 3 --target mhgp9_chain mhgp9_gen mhgp9_gpu > "$O/build_libs.log" 2>&1 || die "build bibliotheques"
  GPU_LIB="$BUILD/libmhgp9_gpu.a"
else
  nice -n 19 cmake --build "$BUILD" --parallel 3 --target mhgp9_chain mhgp9_gen > "$O/build_libs.log" 2>&1 || die "build bibliotheques"
fi
RECIPE="g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -I$SRC/morsehgp3D_v9 -I$SRC/morsehgp3D_v9/src/gen -isystem $BOOST_INC"
Q2="$O/q2_sample_judge"; Q3="$O/q3_sample_judge"
$RECIPE "$HERE/q2_sample_judge.cpp" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" $GPU_LIB -lpthread -o "$Q2" \
  > "$O/compile_q2.log" 2>&1 || die "compilation q2"
$RECIPE "$HERE/q3_sample_judge.cpp" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" $GPU_LIB -lpthread -o "$Q3" \
  > "$O/compile_q3.log" 2>&1 || die "compilation q3"
provenance "$SRC" "$O/PROVENANCE.after_build.txt" || die "provenance git apres construction"
cmp -s "$O/PROVENANCE.txt" "$O/PROVENANCE.after_build.txt" || die "depot modifie pendant la construction"
st=$(git -C "$HERE" status --porcelain -- q2_sample_judge.cpp q3_sample_judge.cpp run_judges_v8_gates.sh regen_inputs.py) || die "git status juges"
cc=$(g++ --version | head -1) || die "g++ --version"
printf 'sources_juges_non_commitees=%s\nrecette=%s <source> %s %s -lpthread\ncompilateur=%s\nbuild_type=Release\nentrees=conformes_a_regen_inputs.EXPECTED\n' \
  "${st:-aucune}" "$RECIPE" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" "$cc" >> "$O/PROVENANCE.txt" || die "ecriture provenance"
sha256sum "$HERE/run_judges_v8_gates.sh" "$HERE/regen_inputs.py" "$HERE/q2_sample_judge.cpp" "$HERE/q3_sample_judge.cpp" "$Q2" "$Q3" \
  "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" "$D"/s00_k5_s8_w8_r0_nested_8000.u32le \
  "$D"/s01_k5_s8_w8_r0_nested_8000.u32le "$D"/s02_k5_s8_w8_r0_nested_8000.u32le >> "$O/PROVENANCE.txt" || die "empreintes"
L() { echo "$D/$1_k5_s8_w8_r0_nested_8000.u32le"; }
# Garde d'index (contrelecture B du juge v7) : trois mutants par juge, refus 2 avant l'echantillonnage.
for m in out-of-range:INDEX_ID_OUT_OF_RANGE duplicate:INDEX_ID_DUPLICATE missing:INDEX_ID_MISSING; do
  inj=${m%%:*}; mk=${m##*:}
  refusal "q2_lidar_s02_8000_k5_index_${inj}" "$mk" "$Q2" file "$(L s02)" 5 50 2 "--inject=index-$inj"
  refusal "q3_fixture_eq_index_${inj}" "$mk" "$Q3" fixture-eq --compare "--inject=index-$inj"
done
run q3_fixture_eq_compare 0 - "$Q3" fixture-eq --compare
run q3_fixture_eq_overprune 1 PRUNE_DISAGREES "$Q3" fixture-eq --compare --inject=overprune
run q3_fixture_eq_level 1 "MISSING a=" "$Q3" fixture-eq --inject=level
run q3_fixture_eq_shell_dup 1 "MISSING a=" "$Q3" fixture-eq --inject=shell-dup
run q3_fixture_eq_key 1 "MISSING a=" "$Q3" fixture-eq --inject=key
# Fixture CRL minimale (triangle de l'audit A) : la seule ancre a (--sites=0) distingue l'arete max (v7) des
# partenaires (v6) ; les deux mutants d'enumeration doivent etre tues DANS la strate CRL.
run q3_fixture_crl_compare 0 - "$Q3" fixture-crl --compare --sites=0 --min-crl=1
run q3_fixture_crl_drop_long 1 PRUNE_DISAGREES_CRL "$Q3" fixture-crl --compare --sites=0 --min-crl=1 --inject=drop-long
run q3_fixture_crl_drop_crl 1 PRUNE_DISAGREES_CRL "$Q3" fixture-crl --compare --sites=0 --min-crl=1 --inject=drop-crl
run q2_lidar_s02_8000_k5_level 1 "MISSING a=" "$Q2" file "$(L s02)" 5 50 2 --inject=level
run q2_lidar_s02_8000_k5_shell_dup 1 "MISSING a=" "$Q2" file "$(L s02)" 5 50 2 --inject=shell-dup
run q2_lidar_s02_8000_k5_key 1 "MISSING a=" "$Q2" file "$(L s02)" 5 50 2 --inject=key
# Sites isoles (choisis depuis les seules coordonnees) : --compare exhaustif, plancher CRL mesure dans le
# parcours brut (triangles CRL distincts observes : 92, 179 et 17 a s00, s01, s02 ; la cle CRL retiree du
# catalogue doit etre declaree manquante), mutant drop-crl tue dans la strate, drop-long mutant de longueur.
for pair in s00:80 s01:150 s02:15; do
  s=${pair%%:*}; m=${pair##*:}
  run "q3_lidar_${s}_8000_k10_compare_isolated" 0 - "$Q3" file "$(L $s)" 10 0 2 --compare --long-sites=8 --min-crl=$m
  run "q3_lidar_${s}_8000_k10_drop_crl_isolated" 1 PRUNE_DISAGREES_CRL "$Q3" file "$(L $s)" 10 0 2 --compare --long-sites=8 --min-crl=$m --inject=drop-crl
  run "q3_lidar_${s}_8000_k10_drop_long_isolated" 1 PRUNE_DISAGREES "$Q3" file "$(L $s)" 10 0 2 --compare --long-sites=8 --min-crl=$m --inject=drop-long
  run "q3_lidar_${s}_8000_k10_overprune_isolated" obs - "$Q3" file "$(L $s)" 10 0 2 --compare --long-sites=8 --inject=overprune
done
printf 'status=%d\n' "$fail" > "$O/STATUS" || die "ecriture STATUS"
exit $fail
