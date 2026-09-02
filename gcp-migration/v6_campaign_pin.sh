#!/usr/bin/env bash
# PIN DU PROTOCOLE de campagne v6 (P0-2 de l'audit GCP v6, deuxieme tour) :
# le COMMIT est IMPOSE par l'appelant (l'etage 2 du bootstrap, deja
# authentifie) — aucun `rev-parse HEAD` ici. Refuse un worktree qui differe
# de ce commit sur les CHEMINS NORMATIFS — moteurs v5/v6 (hors audits/),
# protocole (bootstrap, cycle de vie, pin, runner, validateur, profils
# canoniques) ET les trois gardes GCP. Puis materialise TOUT depuis le
# commit ; le manifeste est CANONIQUE : une ligne `sha256<TAB>taille<TAB>
# chemin` par fichier dans l'ordre normatif, precedee du schema et du
# commit — `protocol_manifest_sha256` est le SHA-256 de CE manifeste (les
# frontieres de fichiers sont liees, contrairement a une concatenation nue).
#
# Usage : v6_campaign_pin.sh WORK_DIR SOURCE_COMMIT
# Produit : WORK_DIR/bundle.tgz, WORK_DIR/pinned/gcp-migration/*,
#           WORK_DIR/pin_manifest.txt (le manifeste canonique lui-meme)
# Imprime : source_commit= / source_payload_sha256= / protocol_manifest_sha256=
#           + une ligne pinned_sha256 par fichier.
set -euo pipefail
WORK="${1:?repertoire de travail requis}"
SOURCE_COMMIT="${2:?commit impose requis (capture par le premier etage authentifie, une seule fois)}"
git cat-file -e "${SOURCE_COMMIT}^{commit}" 2>/dev/null || {
  echo "REFUS : commit impose inconnu (${SOURCE_COMMIT})" >&2
  exit 2
}

# L'ORDRE de cette liste est NORMATIF (manifeste canonique).
PROTOCOL_FILES=(
  gcp-migration/session_campagne_v6_g4.sh
  gcp-migration/v6_session_lifecycle.sh
  gcp-migration/v6_campaign_pin.sh
  gcp-migration/v6_campaign_remote.sh
  gcp-migration/validate_v6_campaign.py
  gcp-migration/profils/decision_v1.env
  gcp-migration/profils/smoke_v1.env
  gcp-migration/profils/g4_mesure_v1.env
  gcp-migration/profils/g4_serie_c_v1.env
  gcp-migration/profils/g4_tests_v1.env
  gcp-migration/profils/g4_tests_v2.env
  morsehgp3D_v6/tests/pilote_juge.py
  gcp-migration/set_max_run_duration_and_verify.sh
  gcp-migration/start_and_verify.sh
  gcp-migration/stop_and_verify.sh
  gcp-migration/recover_v6_session.sh
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
git diff --quiet "${SOURCE_COMMIT}" -- "${PROTOCOL_PATHS[@]}" || {
  echo "REFUS : chemins normatifs modifies dans le worktree par rapport au commit impose — committer d'abord" >&2
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

# MATERIALISATION depuis le COMMIT de TOUT le protocole (gardes et profils
# compris) : seules ces copies sont executees ensuite.
mkdir -p "${WORK}/pinned/gcp-migration/profils" "${WORK}/pinned/morsehgp3D_v6/tests"
for f in "${PROTOCOL_FILES[@]}"; do
  git show "${SOURCE_COMMIT}:${f}" > "${WORK}/pinned/${f}"
  chmod +x "${WORK}/pinned/${f}"
done

# MANIFESTE CANONIQUE : schema + commit + une ligne par fichier (sha256,
# taille en octets, chemin) dans l'ordre normatif ; le digest du protocole
# est le SHA-256 de ce manifeste exact. Le cycle de vie le RECALCULE depuis
# les copies materialisees avant toute execution.
{
  echo "schema=e-hgp.protocol-manifest.v1"
  echo "commit=${SOURCE_COMMIT}"
  for f in "${PROTOCOL_FILES[@]}"; do
    printf '%s\t%s\t%s\n' \
      "$(sha256sum "${WORK}/pinned/${f}" | awk '{print $1}')" \
      "$(wc -c < "${WORK}/pinned/${f}")" \
      "${f}"
  done
} > "${WORK}/pin_manifest.txt"
PROTOCOL_MANIFEST_SHA256="$(sha256sum "${WORK}/pin_manifest.txt" | awk '{print $1}')"

echo "source_commit=${SOURCE_COMMIT}"
echo "source_payload_sha256=${SOURCE_PAYLOAD_SHA256}"
echo "protocol_manifest_sha256=${PROTOCOL_MANIFEST_SHA256}"
awk -F'\t' '/\t/ {print "pinned_sha256 " $1 " " $3}' "${WORK}/pin_manifest.txt"
