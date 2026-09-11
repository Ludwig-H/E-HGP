#!/usr/bin/env bash
# Rejeu des octets ad7ffd28 depuis un export PINNÉ (`git archive ad7ffd28`),
# isolé du worktree partagé qui transitionnait en même temps vers ce842a3f.
# Chaque #include résout vers les octets ad7ffd28. record.py code attendu exact.
set -u
R=/workspaces/E-HGP
A=$R/morsehgp3D_v7/audits/receipts_cache_commit_20260911
PIN=$R/build/ad7ffd28_v7_pin/morsehgp3D_v7
W=$R/build/ad7ffd28_replay
BOOST=$R/build/v7_boost_gate/extracted/usr/include
REC="python3 $A/record.py"
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -isystem $BOOST"
SAN="-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie"
SANENV="ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1"
OLD="$A/../receipts_raccord_ancres_20260910/suite_cache_20260910"
CA="$A/../receipts_raccord_ancres_20260910/corpus_aleatoire"
rm -rf "$A/portes_commit" "$A/empreintes_commit" "$A/mutants_groupes" "$A/corpus_commit" "$A/front_commit" "$W"
mkdir -p "$W/gates" "$A/portes_commit" "$A/empreintes_commit" "$A/mutants_groupes" "$A/corpus_commit" "$A/front_commit"
cd "$R"
git rev-parse HEAD > "$A/HEAD.txt"; echo "ad7ffd28b35e153a20bd8cf42534d1cd29160bcd" > "$A/REVIEWED_COMMIT.txt"
g++ --version | head -1 > "$A/compiler_version.txt"

echo "== portes O2 / ASan-UBSan (arbre pinné ad7ffd28)"
: > "$A/portes_commit/identite_o2_san.txt"
for g in facet_resolver_cache_gate full_ball_tower_gate full_ball_work_gate witness_front_gate; do
  $REC "$A/portes_commit" "compile_${g}_o2" 0 -- g++ $FLAGS -O2 "$PIN/tests/$g.cpp" -o "$W/gates/${g}_o2"
  $REC "$A/portes_commit" "${g}_o2_selftest" 0 -- "$W/gates/${g}_o2" --selftest
  $REC "$A/portes_commit" "${g}_o2_bad_argument" 2 -- "$W/gates/${g}_o2" --unknown
  $REC "$A/portes_commit" "compile_${g}_san" 0 -- g++ $FLAGS $SAN "$PIN/tests/$g.cpp" -o "$W/gates/${g}_san"
  env $SANENV $REC "$A/portes_commit" "${g}_san_selftest" 0 -- "$W/gates/${g}_san" --selftest
  if cmp -s "$A/portes_commit/${g}_o2_selftest.stdout" "$A/portes_commit/${g}_san_selftest.stdout"; then
    echo "$g stdout O2 == stdout ASan/UBSan" >> "$A/portes_commit/identite_o2_san.txt"
  else echo "$g stdout O2 != stdout ASan/UBSan" >> "$A/portes_commit/identite_o2_san.txt"; fi
done

echo "== empreintes par famille (arbre pinné)"
cp "$OLD/empreintes/full_ball_tower_probe_family.cpp" "$PIN/bench/"
$REC "$A/empreintes_commit" compile_probe_family_o2 0 -- g++ $FLAGS -O2 "$PIN/bench/full_ball_tower_probe_family.cpp" -o "$W/gates/probe_family_o2"
for cfg in "uniform 400" "uniform 2000" "scanline_overlap_multiecho 2000" "scanline_single_pass 2000" "terrain 2000"; do
  set -- $cfg
  env MHGP7_AUDIT_FAMILY="$1" $REC "$A/empreintes_commit" "commit_$1_n$2" 0 -- "$W/gates/probe_family_o2" --n="$2" --s=8 --kmax=10 --threads=2
done
python3 -B - "$A/empreintes_commit" "$OLD/empreintes" > "$A/empreintes_commit/comparaison_avec_recu_20260910.txt" <<'PYEOF'
import json, pathlib, sys
new_dir, old_dir = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
for fam, n in [("uniform",400),("uniform",2000),("scanline_overlap_multiecho",2000),("scanline_single_pass",2000),("terrain",2000)]:
    try:
        new=json.loads((new_dir/f"commit_{fam}_n{n}.stdout").read_text()); old=json.loads((old_dir/f"wip4_{fam}_n{n}.stdout").read_text())
    except (OSError,ValueError) as e: print(fam,n,"ILLISIBLE",e); continue
    keys=["payload_digest","nodes","extra_records","balls","contributions","vertical_refs"]
    print(fam,n,"identique" if all(new.get(k)==old.get(k) for k in keys) else "DIFFERENT",{k:(old.get(k),new.get(k)) for k in keys})
PYEOF

echo "== mutants groupés (copies du pinné)"
for kind in nominal growth_grouped inert_grouped; do
  T="$W/mutants/$kind"; rm -rf "$T"; mkdir -p "$T"
  cp -r "$PIN/src" "$PIN/oracle" "$PIN/bench" "$PIN/tests" "$T/"
  $REC "$A/mutants_groupes" "mutate_$kind" 0 -- python3 -B "$A/mutants_groupes_mutate.py" "$kind" "$T/src/forest/full_ball_tower.hpp"
  diff -u "$PIN/src/forest/full_ball_tower.hpp" "$T/src/forest/full_ball_tower.hpp" > "$A/mutants_groupes/$kind.diff"
  $REC "$A/mutants_groupes" "compile_${kind}_o2" 0 -- g++ $FLAGS -O2 "$T/tests/full_ball_tower_gate.cpp" -o "$W/gates/${kind}_gate_o2"
  if [ "$kind" = nominal ]; then exp=0; else exp=1; fi
  $REC "$A/mutants_groupes" "${kind}_selftest" "$exp" -- "$W/gates/${kind}_gate_o2" --selftest
done

echo "== corpus aléatoire (pont pinné)"
$REC "$A/corpus_commit" compile_tower_bridge_o2 0 -- g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -I "$PIN" "$CA/tower_bridge.cpp" -o "$W/gates/tower_bridge"
cd "$CA"
env PYTHONDONTWRITEBYTECODE=1 $REC "$A/corpus_commit" principal_normal 0 -- python3 -B tower_corpus.py --bridge "$W/gates/tower_bridge" --clouds 200 --seed 20260910
env PYTHONDONTWRITEBYTECODE=1 $REC "$A/corpus_commit" principal_optimized 0 -- python3 -B -O tower_corpus.py --bridge "$W/gates/tower_bridge" --clouds 200 --seed 20260910
env PYTHONDONTWRITEBYTECODE=1 $REC "$A/corpus_commit" cocircular_normal 0 -- python3 -B tower_corpus.py --bridge "$W/gates/tower_bridge" --clouds 300 --seed 20260912 --no-fixtures --family cocircular
env PYTHONDONTWRITEBYTECODE=1 $REC "$A/corpus_commit" extended_normal 0 -- python3 -B tower_corpus.py --bridge "$W/gates/tower_bridge" --clouds 2000 --seed 20260911 --no-fixtures
env PYTHONDONTWRITEBYTECODE=1 $REC "$A/corpus_commit" mutant_drop_extra_ball 1 -- python3 -B tower_corpus.py --bridge "$W/gates/tower_bridge" --clouds 200 --seed 20260910 --mutant drop_extra_ball
cd "$R"
: > "$A/corpus_commit/comparaison_avec_recu_20260910.txt"
for pair in "principal_normal corpus_normal" "cocircular_normal corpus_cocircular_normal" "extended_normal corpus_ext_normal"; do
  set -- $pair
  if cmp -s "$A/corpus_commit/$1.stdout" "$CA/$2.json"; then echo "$1 == reçu du 10 sept. ($2.json), octets identiques"; else echo "$1 != $2.json"; fi >> "$A/corpus_commit/comparaison_avec_recu_20260910.txt"
done
if cmp -s "$A/corpus_commit/principal_normal.stdout" "$A/corpus_commit/principal_optimized.stdout"; then echo "principal : -B == -B -O"; else echo "principal : -B != -B -O"; fi >> "$A/corpus_commit/comparaison_avec_recu_20260910.txt"
if cmp -s "$A/corpus_commit/mutant_drop_extra_ball.stdout" "$CA/mutants/drop_extra_ball.json"; then echo "mutant drop_extra_ball == reçu du 10 sept."; else echo "mutant drop_extra_ball != reçu"; fi >> "$A/corpus_commit/comparaison_avec_recu_20260910.txt"

echo "== front par lots (probe pinné)"
cp "$OLD/front_temoins/witness_front_scale_probe.cpp" "$PIN/bench/"
$REC "$A/front_commit" compile_witness_front_scale_probe_o2 0 -- g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -O2 "$PIN/bench/witness_front_scale_probe.cpp" -o "$W/gates/witness_front_scale_probe_o2"
: > "$A/front_commit/comparaison_avec_recu_20260910.txt"
OLDR="$OLD/front_temoins/wf_scale.results"
for cfg in "2000 8 4096 1 uniform" "2000 8 100000 4 uniform" "2000 10 4096 2 uniform" "2000 12 4096 2 uniform" "2000 8 4096 2 scanline_overlap_multiecho" "2000 8 4096 2 eight_clusters" "2000 8 4096 2 terrain" "8000 8 65536 4 uniform" "8000 8 1000 4 scanline_overlap_multiecho"; do
  set -- $cfg; name="front_$5_n$1_s$2_lot$3_t$4"
  $REC "$A/front_commit" "$name" 0 -- "$W/gates/witness_front_scale_probe_o2" "$1" "$2" "$3" "$4" "$5"
  new=$(head -1 "$A/front_commit/$name.stdout")
  old=$(grep -m1 "\"family\":\"$5\",\"n\":$1,\"s\":$2,\"lot\":$3,\"threads\":$4," "$OLDR")
  if [ -n "$new" ] && [ "$new" = "$old" ]; then echo "$name == ligne du reçu du 10 sept."; else echo "$name != reçu : new=$new old=$old"; fi >> "$A/front_commit/comparaison_avec_recu_20260910.txt"
done

echo "== empreintes des sources pinnées ad7ffd28 consultées"
sha256sum "$PIN"/src/forest/full_ball_tower.hpp "$PIN"/src/forest/full_coverage_certificate.hpp \
  "$PIN"/tests/facet_resolver_cache_gate.cpp "$PIN"/tests/full_ball_tower_gate.cpp "$PIN"/tests/full_ball_work_gate.cpp \
  "$PIN"/tests/witness_front_gate.cpp "$PIN"/src/pipeline/witness_front.hpp "$PIN"/src/spindle/witness_batch.hpp \
  "$PIN"/bench/full_ball_tower_probe.cpp "$PIN"/bench/full_gabriel_semantic_digest.hpp "$PIN"/CMakeLists.txt \
  | sed "s#$PIN/#ad7ffd28:morsehgp3D_v7/#" > "$A/SHA256SUMS_sources.txt"
echo "rejeu pinné terminé"
