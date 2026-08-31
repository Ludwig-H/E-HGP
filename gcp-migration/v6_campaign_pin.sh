#!/usr/bin/env bash
# PIN DU PROTOCOLE de campagne v6 (meme doctrine que v5_campaign_pin.sh) :
# refuse un worktree qui differe de HEAD sur les CHEMINS NORMATIFS — les DEUX
# moteurs (v6 mesure, v5 sujet differentiel du bench apparie et source de la
# reference de conformite a 50 k) ET le protocole qui choisit les runs et les
# juge (lanceur, runner distant, validateur, ce script lui-meme). Puis
# materialise depuis le COMMIT : le bundle transfere a la VM et les scripts
# epingles executes localement — le worktree n'intervient plus apres.
#
# Usage : v6_campaign_pin.sh WORK_DIR
# Produit : WORK_DIR/bundle.tgz, WORK_DIR/pinned/gcp-migration/{runner,validateur}
# Imprime : source_commit= / source_payload_sha256= / protocol_manifest_sha256=
set -euo pipefail
WORK="${1:?repertoire de travail requis}"

# `morsehgp3D_v5/audits` et `morsehgp3D_v6/audits` sont les canaux
# documentaires des auditeurs : jamais construits, leurs modifications en
# cours n'empechent pas une session (le bundle est extrait du COMMIT).
PROTOCOL_PATHS=(
  morsehgp3D_v6
  ':(exclude)morsehgp3D_v6/audits'
  morsehgp3D_v5
  ':(exclude)morsehgp3D_v5/audits'
  gcp-migration/session_campagne_v6_g4.sh
  gcp-migration/v6_campaign_remote.sh
  gcp-migration/validate_v6_campaign.py
  gcp-migration/v6_campaign_pin.sh
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

mkdir -p "${WORK}/pinned"
tar xzf "${BUNDLE}" -C "${WORK}/pinned" \
  gcp-migration/v6_campaign_remote.sh gcp-migration/validate_v6_campaign.py

# Manifeste du protocole : digest des VERSIONS GIT exactes des scripts qui
# organisent et jugent la mesure.
PROTOCOL_MANIFEST_SHA256="$({
  git show "${SOURCE_COMMIT}:gcp-migration/session_campagne_v6_g4.sh"
  git show "${SOURCE_COMMIT}:gcp-migration/v6_campaign_remote.sh"
  git show "${SOURCE_COMMIT}:gcp-migration/validate_v6_campaign.py"
} | sha256sum | awk '{print $1}')"

echo "source_commit=${SOURCE_COMMIT}"
echo "source_payload_sha256=${SOURCE_PAYLOAD_SHA256}"
echo "protocol_manifest_sha256=${PROTOCOL_MANIFEST_SHA256}"
