#!/usr/bin/env bash
# Rejoue toute la preparation des donnees v12 (local ou VM G4), dans l'ordre, sans rien ecrire hors de $ROOT.
#
#   ROOT=/chemin/v12_donnees PY=/chemin/python PATCHWORK_ARCHIVES=... KITTI_FRAMES="d1 d2 ..." KITTI_CONTROL=... \
#     bash <depot>/morsehgp3D_v12/bench/data/replay_all.sh [outils] [kitti] [small] [ign] [eth3d] [forinstance] \
#       [boreas] [crops] [verify] [bundles]
#
# Outils : les voisins de ce script (le dossier bench/data/ du depot, ou le meme dossier envoye sur la VM), cherches a
# cote de replay_all.sh quel que soit le repertoire courant, jamais sous $ROOT (CST-0216). Ils sont epingles par
# SHA256SUMS.txt (meme dossier, chemins relatifs a ce dossier).
# Etape `outils` : rejeu a blanc, sans donnee, sans ROOT ni Python (sha256sum de coreutils) : chaque fichier de
# SHA256SUMS.txt est present et a son empreinte, chaque outil appele par une etape y est epingle, et aucun script
# (.py, .sh) du dossier n'echappe a l'epingle. Toute autre etape commence par ce meme controle : un outil absent,
# modifie ou non epingle refuse le rejeu (code 1) avant toute ecriture.
#
# ROOT : racine des sorties (data/, work/, raw/, bundles/), obligatoire pour les etapes de donnees ; jamais dans
#       l'arbre morsehgp3D_v12 (refus, code 2).
# PY  : Python >= 3.10 avec numpy, plus laspy + lazrs (LAZ de l'IGN) et py7zr (7z d'ETH3D) ; sur G4 : le Python
#       portable envoye comme donnee (la VM n'a ni pip ni numpy), ou bien preparer ici et n'envoyer que les paquets.
#       Lance en -I -B (aucun __pycache__ dans le dossier des outils, qui doit rester egal a son epingle).
# PATCHWORK_ARCHIVES : dossier contenant patchwork_sources.tar.gz, eigen3.tar.gz (epingles) et 08_000000.bin
#       (controle du masque) ; g++ requis (sonde de sol).
# BOUTS : dossier des bouts de scene de la v11 (bouts.json + data/), pour les petits nuages.
# B : dossier des constructions du depot (defaut <depot>/build), d'ou viennent les defauts des trames KITTI.
# Sans argument : toutes les etapes de donnees. Chaque etape est idempotente (telechargements verifies par SHA-256
# epingle, conversions deterministes a l'octet).
# Codes : 0 conforme ; 1 outil absent, modifie ou non epingle ; 2 usage (etape inconnue, ROOT absent ou dans l'arbre
# v12) ; sinon le code de l'outil en echec.
set -euo pipefail
S=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)
V12=$(cd "$S/../.." && pwd -P)
REPO=$(cd "$V12/.." && pwd -P)
STEPS=${*:-kitti small ign eth3d forinstance boreas crops verify bundles}
TOOLS="prepare_semantickitti.py prepare_small.py prepare_ign_lidarhd.py prepare_eth3d.py prepare_forinstance.py
prepare_boreas.py crop_scenes.py verify_inputs.py make_g4_bundle.py replay_all.sh"

check_tools() {
  local sums=$S/SHA256SUMS.txt bad=0 listed tool file
  if [ ! -f "$sums" ]; then
    echo "outils : SHA256SUMS.txt absent de $S" >&2
    return 1
  fi
  if ! (cd "$S" && sha256sum --check --strict --quiet SHA256SUMS.txt) >&2; then
    echo "outils : empreinte differente ou fichier absent (SHA256SUMS.txt)" >&2
    bad=1
  fi
  listed=$(awk '{ print $2 }' "$sums")
  for tool in $TOOLS; do
    if ! grep -qxF "$tool" <<<"$listed"; then
      echo "outils : $tool appele par une etape mais non epingle" >&2
      bad=1
    fi
  done
  while IFS= read -r file; do
    if ! grep -qxF "$file" <<<"$listed"; then
      echo "outils : $file present mais non epingle" >&2
      bad=1
    fi
  done < <(cd "$S" && find . -type f \( -name '*.py' -o -name '*.sh' \) ! -path '*/__pycache__/*' |
             sed 's|^\./||' | sort)
  if [ "$bad" -ne 0 ]; then
    return 1
  fi
  echo "outils conformes : $(wc -l <"$sums") fichiers epingles dans $S" >&2
}

for step in $STEPS; do
  case $step in
    outils|kitti|small|ign|eth3d|forinstance|boreas|crops|verify|bundles) ;;
    *) echo "etape inconnue : $step" >&2; exit 2 ;;
  esac
done
check_tools || exit 1
if [ "$STEPS" = outils ]; then
  exit 0
fi
if [ -z "${ROOT:-}" ]; then
  echo "ROOT obligatoire (racine des sorties, hors du depot)" >&2
  exit 2
fi
ROOT=$(realpath -m -- "$ROOT")
case "$ROOT/" in
  "$V12"/*) echo "ROOT dans l'arbre morsehgp3D_v12 : refus ($ROOT)" >&2; exit 2 ;;
esac
PY=${PY:-$ROOT/work/venv/bin/python}
B=${B:-$REPO/build}
PATCHWORK_ARCHIVES=${PATCHWORK_ARCHIVES:-$B/v11-persist/data_points3}
KITTI_CONTROL=${KITTI_CONTROL:-$REPO/morsehgp3D_v8/audits/lidar08_20260914/data/dataset/sequences/08/velodyne}
BOUTS=${BOUTS:-$B/v11-persist/bouts}
KITTI_FRAMES=${KITTI_FRAMES:-"$B/v11-persist/kitti_cache $B/v11-persist/data_points3 $B/v11-persist/p08 $B/v11-persist/e1_p08 $B/v10-lidar-demos/_cache $B/v11-videos-20261004/Zoltan/demos/_cache $REPO/morsehgp3D_v8/audits/lidar08_20260914/data"}

for step in $STEPS; do
  [ "$step" = outils ] && continue
  echo "== $step $(date -u +%H:%M:%S)" >&2
  case $step in
    kitti)
      # shellcheck disable=SC2086
      "$PY" -I -B "$S/prepare_semantickitti.py" --archives "$PATCHWORK_ARCHIVES" --frames $KITTI_FRAMES \
        --control "$KITTI_CONTROL" --out "$ROOT/data/semantickitti" --work "$ROOT/work/semantickitti" --labels ;;
    small)
      "$PY" -I -B "$S/prepare_small.py" --out "$ROOT/data/small" --kitti "$ROOT/data/semantickitti" \
        --bouts "$BOUTS" ;;
    ign)
      "$PY" -I -B "$S/prepare_ign_lidarhd.py" --raw "$ROOT/raw/ign_lidarhd" --out "$ROOT/data/ign_lidarhd" ;;
    eth3d)
      "$PY" -I -B "$S/prepare_eth3d.py" --raw "$ROOT/raw/eth3d" --out "$ROOT/data/eth3d" ;;
    forinstance)
      "$PY" -I -B "$S/prepare_forinstance.py" --raw "$ROOT/raw/forinstance" --out "$ROOT/data/forinstance" ;;
    boreas)
      "$PY" -I -B "$S/prepare_boreas.py" --raw "$ROOT/raw/boreas" --out "$ROOT/data/boreas" \
        --archives "$PATCHWORK_ARCHIVES" --work "$ROOT/work/boreas" --check ;;
    crops)
      for jeu in ign_lidarhd eth3d forinstance boreas; do
        m=$ROOT/data/$jeu/manifest.json
        if [ -f "$m" ]; then
          "$PY" -I -B "$S/crop_scenes.py" --manifest "$m"
        else
          echo "crops : manifeste absent, jeu saute : $jeu" >&2
        fi
      done ;;
    verify)
      # Un motif sans correspondance reste litteral : verify_inputs le refuse (code 2), jamais un vert vide.
      for m in "$ROOT"/data/*/manifest.json "$ROOT"/data/semantickitti/manifest_v12set.json; do
        "$PY" -I -B "$S/verify_inputs.py" "$m" --measure
      done ;;
    bundles)
      "$PY" -I -B "$S/make_g4_bundle.py" --out "$ROOT/bundles/g4_kitti_v12set" \
        "$ROOT/data/semantickitti/manifest_v12set.json" --with-labels
      "$PY" -I -B "$S/make_g4_bundle.py" --out "$ROOT/bundles/g4_small" "$ROOT/data/small/manifest.json"
      for jeu in ign_lidarhd eth3d forinstance boreas; do
        "$PY" -I -B "$S/make_g4_bundle.py" --out "$ROOT/bundles/g4_$jeu" "$ROOT/data/$jeu/manifest.json" \
          --prefer-distinct --with-crops
      done
      for b in "$ROOT"/bundles/g4_*/bundle_manifest.json; do "$PY" -I -B "$S/verify_inputs.py" "$b"; done ;;
  esac
done
