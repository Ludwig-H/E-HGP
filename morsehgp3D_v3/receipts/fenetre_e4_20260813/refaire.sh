#!/usr/bin/env bash
# Rejeu exact du recu `fenetre_e4_20260813`. Aucune commande GCP.
set -euo pipefail
cd "$(dirname "$0")/../../.."
P=build/v3/mhgp3v_wspd_wavefront_probe
R=morsehgp3D_v3/receipts/fenetre_e4_20260813
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --target mhgp3v_wspd_wavefront_probe --parallel

C="--tight --vwave --window=512 --window-ledger"
$P --family=uniform        --points=3000,6000,12000,24000 --sep-euclid=8/1  $C --max-slope=9 > "$R/uniform_s8.txt"        2>&1 || true
$P --family=terrain        --points=3000,6000,12000,24000 --sep-euclid=8/1  $C              > "$R/terrain_s8.txt"        2>&1 || true
$P --family=terrain        --points=3000,6000,12000       --sep-euclid=16/1 $C              > "$R/terrain_s16.txt"       2>&1 || true
$P --family=eight_clusters --points=3000,6000,12000,24000 --sep-euclid=8/1  $C              > "$R/eight_clusters_s8.txt" 2>&1 || true
for n in 3000 6000 12000; do
  printf 'n=%-6s ' "$n"
  $P --family=eight_clusters --points=$n --sep-euclid=8/1 $C --inflation=4000 --inflation-lane=2 2>&1 |
    grep -o 'coeur q4 par paire.*'
done > "$R/coeur_par_paire_amas.txt" 2>&1

# MANIFESTE : sujet, binaire et chaque sortie. Un recu sans empreintes n'est pas
# un recu, et le contre-audit `b96751c` le refuse a juste titre.
{
  echo "# manifeste fenetre_e4_20260813"
  echo "git_head=$(git rev-parse HEAD)"
  echo "git_propre=$(git status --porcelain | wc -l) fichiers modifies"
  echo "compilateur=$(g++ --version | head -1)"
  sha256sum morsehgp3D_v3/prototype/wspd_wavefront_probe.cpp "$P" "$R"/*_s8.txt "$R"/*_s16.txt "$R"/coeur_par_paire_amas.txt
} > "$R/manifeste.txt"
echo "RECU_REFAIT"
