#!/usr/bin/env bash
# Auditeur C — differentiel moteur / lots (S2, reference CPU) a la taille d'interet, K5 et K10.
# Usage : run_batch_diff.sh <build_v9_release> <boost_include> <dossier_entrees> <sortie_neuve>
set -u -o pipefail
die() { echo "infrastructure: $*" >&2; exit 2; }
BUILD=$1; BOOST_INC=$2; D=$3; O=$4
HERE=$(cd "$(dirname "$0")" && pwd) || die "chemin"
SRC=$(cd "$HERE/../../.." && pwd) || die "racine"
[ ! -e "$O" ] || die "sortie deja presente : $O"
mkdir -p "$O" || die "mkdir"
st=$(git -C "$SRC" status --porcelain -- morsehgp3D_v9/src morsehgp3D_v9/tests/gen morsehgp3D_v9/CMakeLists.txt \
  morsehgp3D_v9/audits/c_omission_20260923/batch_diff.cpp morsehgp3D_v9/audits/c_omission_20260923/run_batch_diff.sh) || die "git status"
[ -z "$st" ] || die "sources modifiees : $st"
commit=$(git -C "$SRC" rev-parse HEAD) || die "git rev-parse"
srccommit=$(git -C "$SRC" log -1 --format=%H -- morsehgp3D_v9/src) || die "git log"
python3 "$HERE/regen_inputs.py" --check "$D" > "$O/inputs_check.txt" 2>&1 || die "entrees non conformes"
nice -n 19 cmake --build "$BUILD" --parallel 3 --target mhgp9_chain mhgp9_gen mhgp9_gpu > "$O/build_libs.log" 2>&1 || die "build"
X="$O/batch_diff"
g++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -I"$SRC/morsehgp3D_v9" -I"$SRC/morsehgp3D_v9/src/gen" \
  -isystem "$BOOST_INC" "$HERE/batch_diff.cpp" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" "$BUILD/libmhgp9_gpu.a" \
  -lpthread -o "$X" || die "compilation"
{ printf 'commit=%s\nsrc_commit=%s\n' "$commit" "$srccommit"
  sha256sum "$HERE/batch_diff.cpp" "$HERE/run_batch_diff.sh" "$X" "$BUILD/libmhgp9_chain.a" "$BUILD/libmhgp9_gen.a" "$BUILD/libmhgp9_gpu.a"
} > "$O/PROVENANCE.txt" || die "provenance"
fail=0
run() {  # run <nom> <attendu> <arguments...>
  local name=$1 want=$2; shift 2
  : > "$O/$name.txt" || die "sortie $name"
  nice -n 19 "$X" "$@" >> "$O/$name.txt" 2> "$O/$name.err"
  local c=$?
  printf 'exit=%s expected=%s\n' "$c" "$want" >> "$O/$name.txt" || die "ecriture $name"
  [ "$c" -eq "$want" ] || fail=1
}
L() { echo "$D/$1_k5_s8_w8_r0_nested_8000.u32le"; }
run mutant_drop_one_s02_k5 1 file "$(L s02)" 5 2 --inject=drop-one
for k in 5 10; do
  for s in s00 s01 s02; do run "lidar_${s}_8000_k$k" 0 file "$(L $s)" $k 2; done
  run "lidar_scene00_full_k$k" 0 file "$D/scene_00_full.u32le" $k 2
done
run uniform_8000_k10 0 family uniform 8000 10 2
printf 'status=%d\n' "$fail" > "$O/STATUS" || die "STATUS"
exit $fail
