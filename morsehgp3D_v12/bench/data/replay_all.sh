#!/usr/bin/env bash
# Rejoue toute la preparation des donnees v12 (local ou VM G4), dans l'ordre, sans rien ecrire hors de $ROOT.
#
#   PY=/chemin/python ROOT=/chemin/v12_donnees PATCHWORK_ARCHIVES=... KITTI_FRAMES="d1 d2 ..." KITTI_CONTROL=... \
#     bash scripts/replay_all.sh [kitti] [small] [ign] [eth3d] [forinstance] [boreas] [crops] [verify] [bundles]
#
# PY  : Python >= 3.10 avec numpy, plus laspy + lazrs (LAZ de l'IGN) et py7zr (7z d'ETH3D) ; sur G4 : le Python
#       portable envoye comme donnee (la VM n'a ni pip ni numpy), ou bien preparer ici et n'envoyer que les paquets.
# PATCHWORK_ARCHIVES : dossier contenant patchwork_sources.tar.gz, eigen3.tar.gz (epingles) et 08_000000.bin
#       (controle du masque) ; g++ requis (sonde de sol).
# BOUTS : dossier des bouts de scene de la v11 (bouts.json + data/), pour les petits nuages.
# Sans argument : toutes les etapes. Chaque etape est idempotente (telechargements verifies par SHA-256 epingle,
# conversions deterministes a l'octet).
set -euo pipefail
ROOT=${ROOT:-$(cd "$(dirname "$0")/.." && pwd)}
PY=${PY:-$ROOT/work/venv/bin/python}
S=$ROOT/scripts
STEPS=${*:-kitti small ign eth3d forinstance boreas crops verify bundles}
B=/workspaces/E-HGP/build
PATCHWORK_ARCHIVES=${PATCHWORK_ARCHIVES:-$B/v11-persist/data_points3}
KITTI_CONTROL=${KITTI_CONTROL:-/workspaces/E-HGP/morsehgp3D_v8/audits/lidar08_20260914/data/dataset/sequences/08/velodyne}
BOUTS=${BOUTS:-$B/v11-persist/bouts}
KITTI_FRAMES=${KITTI_FRAMES:-"$B/v11-persist/kitti_cache $B/v11-persist/data_points3 $B/v11-persist/p08 $B/v11-persist/e1_p08 $B/v10-lidar-demos/_cache $B/v11-videos-20261004/Zoltan/demos/_cache /workspaces/E-HGP/morsehgp3D_v8/audits/lidar08_20260914/data"}

for step in $STEPS; do
  echo "== $step $(date -u +%H:%M:%S)" >&2
  case $step in
    kitti)
      # shellcheck disable=SC2086
      "$PY" -I "$S/prepare_semantickitti.py" --archives "$PATCHWORK_ARCHIVES" --frames $KITTI_FRAMES \
        --control "$KITTI_CONTROL" --out "$ROOT/data/semantickitti" --work "$ROOT/work/semantickitti" --labels ;;
    small)
      "$PY" -I "$S/prepare_small.py" --out "$ROOT/data/small" --kitti "$ROOT/data/semantickitti" \
        --bouts "$BOUTS" ;;
    ign)
      "$PY" -I "$S/prepare_ign_lidarhd.py" --raw "$ROOT/raw/ign_lidarhd" --out "$ROOT/data/ign_lidarhd" ;;
    eth3d)
      "$PY" -I "$S/prepare_eth3d.py" --raw "$ROOT/raw/eth3d" --out "$ROOT/data/eth3d" ;;
    forinstance)
      "$PY" -I "$S/prepare_forinstance.py" --raw "$ROOT/raw/forinstance" --out "$ROOT/data/forinstance" ;;
    boreas)
      "$PY" -I "$S/prepare_boreas.py" --raw "$ROOT/raw/boreas" --out "$ROOT/data/boreas" \
        --archives "$PATCHWORK_ARCHIVES" --work "$ROOT/work/boreas" --check ;;
    crops)
      for jeu in ign_lidarhd eth3d forinstance boreas; do
        m=$ROOT/data/$jeu/manifest.json
        [ -f "$m" ] && "$PY" -I "$S/crop_scenes.py" --manifest "$m"
      done ;;
    verify)
      for m in "$ROOT"/data/*/manifest.json "$ROOT"/data/semantickitti/manifest_v12set.json; do
        "$PY" -I "$S/verify_inputs.py" "$m" --measure
      done ;;
    bundles)
      "$PY" -I "$S/make_g4_bundle.py" --out "$ROOT/bundles/g4_kitti_v12set" \
        "$ROOT/data/semantickitti/manifest_v12set.json" --with-labels
      "$PY" -I "$S/make_g4_bundle.py" --out "$ROOT/bundles/g4_small" "$ROOT/data/small/manifest.json"
      for jeu in ign_lidarhd eth3d forinstance boreas; do
        "$PY" -I "$S/make_g4_bundle.py" --out "$ROOT/bundles/g4_$jeu" "$ROOT/data/$jeu/manifest.json" \
          --prefer-distinct --with-crops
      done
      for b in "$ROOT"/bundles/g4_*/bundle_manifest.json; do "$PY" -I "$S/verify_inputs.py" "$b"; done ;;
    *) echo "etape inconnue : $step" >&2; exit 2 ;;
  esac
done
