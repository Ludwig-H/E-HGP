#!/usr/bin/env bash
# Auditeur C, 25 septembre 2026 : campagne du juge a supports independants (v3), reproduction.
# Usage : run_campaign.sh <racine du depot> <binaire du juge> <dossier de sortie>
# Le binaire se compile depuis raw_support_judge.cpp contre les bibliotheques CPU Release de la base BASE.txt (README,
# section Reproduction). Entrees verifiees par SHA-256 et FNV, catalogue par son condense : ../c_raw_pins_20260924/PINS_RAW.json.
set -u
ROOT=$1; J=$2; OUT=$3
R=$ROOT/morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i
PINS=$ROOT/morsehgp3D_v9/audits/c_raw_pins_20260924/PINS_RAW.json
mkdir -p "$OUT"; : > "$OUT/codes.txt"
declare -A F=([b00]=scene_00_000000_grid [b01]=scene_01_000100_grid [b02]=scene_02_000200_grid)
pin() { python3 -c "import json,sys; print([p[sys.argv[4]] for p in json.load(open(sys.argv[1]))['pins'] if p['scene']==sys.argv[2] and p['K']==int(sys.argv[3])][0])" "$PINS" "$1" "$2" "$3"; }
for s in b00 b01 b02; do
  got=$(sha256sum "$R/${F[$s]}/full.u32le" | cut -d' ' -f1)
  [ "$got" = "$(pin $s 5 input_sha256)" ] || { echo "input $s sha256 differs" | tee -a "$OUT/codes.txt"; exit 2; }
done
arg() { echo "--expect-catalogue-digest=$(pin $1 $2 catalogue_digest) --expect-input-fnv=$(pin $1 $2 input_fnv)"; }
for s in b00 b01 b02; do for K in 5 10; do
  /usr/bin/time -v "$J" file "$R/${F[$s]}/full.u32le" $K 8 $(arg $s $K) > "$OUT/${s}_k${K}.txt" 2> "$OUT/${s}_k${K}.time"
  echo "$s K$K rc=$?" | tee -a "$OUT/codes.txt"
done; done
for inj in drop-q2 drop-q3 drop-q4 shell-sub level interior-sub; do
  "$J" file "$R/${F[b00]}/full.u32le" 5 8 $(arg b00 5) --inject=$inj > "$OUT/b00_k5_mutant_${inj}.txt" 2>&1
  echo "mutant $inj b00 K5 rc=$?" | tee -a "$OUT/codes.txt"
done
for K in 5 10; do
  "$J" fixture-cospheric $K 4 --expect-extended=8 > "$OUT/fixture_cospheric_k${K}.txt" 2>&1
  echo "fixture_cospheric K$K rc=$?" | tee -a "$OUT/codes.txt"
done
for inj in drop-q2 drop-q3 drop-q4 shell-sub ext-trim ext-arity level interior-sub; do
  "$J" fixture-cospheric 5 4 --inject=$inj > "$OUT/fixture_cospheric_k5_mutant_${inj}.txt" 2>&1
  echo "mutant $inj fixture K5 rc=$?" | tee -a "$OUT/codes.txt"
done
# Attendu : 6 x rc=0 sur les trames, 2 x rc=0 sur la fixture, 14 x rc=4 (mutant tue par son seul marqueur).
