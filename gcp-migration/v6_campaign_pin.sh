#!/usr/bin/env bash
# PIN DU PROTOCOLE de campagne v6 (P0-2 de l'audit GCP v6 du 31 aout) :
# refuse un worktree qui differe de HEAD sur les CHEMINS NORMATIFS — les DEUX
# moteurs (v6 mesure, v5 sujet differentiel et source de la reference de
# conformite), le protocole (bootstrap, cycle de vie, runner, validateur, ce
# script) ET LES TROIS GARDES GCP (set_max/start/stop : controles SPOT, STOP,
# duree maximale, double coupe-circuit, arret cloture). Puis materialise TOUT
# depuis le COMMIT : le bundle transfere a la VM et les copies epinglees
# executees localement (gardes comprises, y compris dans le trap) — le
# worktree n'intervient plus apres.
#
# Usage : v6_campaign_pin.sh WORK_DIR
# Produit : WORK_DIR/bundle.tgz, WORK_DIR/pinned/gcp-migration/* (protocole
#           et gardes), WORK_DIR/pin_manifest.txt (SHA-256 individuels)
# Imprime : source_commit= / source_payload_sha256= / protocol_manifest_sha256=
#           + une ligne sha256 par fichier du protocole.
set -euo pipefail
WORK="${1:?repertoire de travail requis}"

# L'ORDRE de cette liste est NORMATIF : le manifeste est le digest de la
# concatenation des versions git de ces fichiers, dans cet ordre exact.
PROTOCOL_FILES=(
  gcp-migration/session_campagne_v6_g4.sh
  gcp-migration/v6_session_lifecycle.sh
  gcp-migration/v6_campaign_pin.sh
  gcp-migration/v6_campaign_remote.sh
  gcp-migration/validate_v6_campaign.py
  gcp-migration/set_max_run_duration_and_verify.sh
  gcp-migration/start_and_verify.sh
  gcp-migration/stop_and_verify.sh
)

# `morsehgp3D_v5/audits` et `morsehgp3D_v6/audits` sont les canaux
# documentaires des auditeurs : jamais construits, leurs modifications en
# cours n'empechent pas une session (le bundle est extrait du COMMIT).
PROTOCOL_PATHS=(
  morsehgp3D_v6
  ':(exclude)morsehgp3D_v6/audits'
  morsehgp3D_v5
  ':(exclude)morsehgp3D_v5/audits'
  "${PROTOCOL_FILES[@]}"
)
SOURCE_COMMIT="$(git rev-parse HEAD)"
git diff --quiet -- "${PROTOCOL_PATHS[@]}" || {
  echo "REFUS : chemins normatifs modifies dans le worktree — committer d'abord" >&2
  exit 2
}
git diff --cached --quiet -- "${PROTOCOL_PATHS[@]}" || {
  echo "REFUS : chemins normatifs modifies dans l'index — committer d'abord" >&2
  exit 2
}
if [ -n "$(git ls-files --others --exclude-standard -- "${PROTOCOL_PATHS[@]}")" ]; then
  echo "REFUS : fichiers non suivis dans les chemins normatifs" >&2
  exit 2
fi

BUNDLE="${WORK}/bundle.tgz"
git archive --format=tar.gz -o "${BUNDLE}" "${SOURCE_COMMIT}" \
  morsehgp3D_v5 \
  morsehgp3D_v6 \
  gcp-migration/v6_campaign_remote.sh \
  gcp-migration/validate_v6_campaign.py
SOURCE_PAYLOAD_SHA256="$(sha256sum "${BUNDLE}" | awk '{print $1}')"

# MATERIALISATION depuis le COMMIT de TOUT le protocole (gardes comprises) :
# seules ces copies sont executees ensuite.
mkdir -p "${WORK}/pinned/gcp-migration"
for f in "${PROTOCOL_FILES[@]}"; do
  git show "${SOURCE_COMMIT}:${f}" > "${WORK}/pinned/${f}"
  chmod +x "${WORK}/pinned/${f}"
done

# Manifeste ORDONNE : digest de la concatenation des versions git exactes,
# plus les SHA-256 individuels (enregistres et imprimes).
PROTOCOL_MANIFEST_SHA256="$(for f in "${PROTOCOL_FILES[@]}"; do
  git show "${SOURCE_COMMIT}:${f}"
done | sha256sum | awk '{print $1}')"
{
  echo "source_commit=${SOURCE_COMMIT}"
  echo "ordre_manifeste=${PROTOCOL_FILES[*]}"
  for f in "${PROTOCOL_FILES[@]}"; do
    printf 'sha256 %s %s\n' "$(sha256sum "${WORK}/pinned/${f}" | awk '{print $1}')" "${f}"
  done
} > "${WORK}/pin_manifest.txt"

echo "source_commit=${SOURCE_COMMIT}"
echo "source_payload_sha256=${SOURCE_PAYLOAD_SHA256}"
echo "protocol_manifest_sha256=${PROTOCOL_MANIFEST_SHA256}"
sed -n 's/^sha256 /pinned_sha256 /p' "${WORK}/pin_manifest.txt"
