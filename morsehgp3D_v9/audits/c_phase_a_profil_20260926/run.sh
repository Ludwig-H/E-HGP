#!/usr/bin/env bash
# Auditeur C, 26 septembre 2026 : profil local de la phase A.
# Usage : run.sh <binaire instrumente> <binaire propre> <racine du depot> <sortie>
# Le binaire instrumente se construit en appliquant c_phase_a_20260926/phaseA_profile.diff
# a une copie de morsehgp3D_v9, puis cmake Release avec BOOST_ROOT.
set -u
PROF=$1; CLEAN=$2; ROOT=$3; OUT=$4; mkdir -p "$OUT"
NG=$ROOT/morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le
RAW=$ROOT/morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i/scene_00_000000_grid/full.u32le
export MHGP9_PROFA_STRIDE=16 MHGP9_PROFA_DISCARD=1000000   # un lot sur 16 ; rejet au-dela de 1e6 tics
run() { # etiquette fichier K
  { echo "start $(date +%T) load: $(cat /proc/loadavg)"; } > "$OUT/$1.meta"
  /usr/bin/time -v nice -n 5 "$PROF" "$2" "$3" 8 --s=8 --static=8 --catalogue-digest \
    > "$OUT/$1.json" 2> "$OUT/$1.err"
  echo "exit=$? end $(date +%T) load: $(cat /proc/loadavg)" >> "$OUT/$1.meta"
}
run ng_k5  "$NG"  5
run raw_k5 "$RAW" 5
run ng_k10 "$NG"  10
# Temoins entrelaces sur la meme trame : propre, compteurs seuls, compteurs + echantillonnage.
for arm in propre s0 s16; do
  case $arm in
    propre) BIN=$CLEAN; ST=16;;
    s0)     BIN=$PROF;  ST=0;;
    s16)    BIN=$PROF;  ST=16;;
  esac
  MHGP9_PROFA_STRIDE=$ST nice -n 5 "$BIN" "$NG" 5 8 --s=8 --static=8 \
    > "$OUT/temoin_$arm.json" 2> "$OUT/temoin_$arm.err"
done
