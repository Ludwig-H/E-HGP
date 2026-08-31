#!/usr/bin/env bash
# Session G4 — CAMPAGNE v6. BOOTSTRAP DEUX ETAGES (P0-2, deuxieme tour de
# l'audit GCP v6) : l'etage 1, seul code du worktree jamais execute, ne fait
# QUE capturer le commit, materialiser bootstrap + pin par `git show`,
# verifier sa propre identite octet pour octet contre la copie du commit,
# puis `exec` l'etage 2 (la copie materialisee). L'etage 2 execute le pin
# MATERIALISE avec le commit IMPOSE (aucun nouveau rev-parse), puis le cycle
# de vie materialise avec les gardes materialisees.
#
# POINT D'ENTREE DE CONFIANCE MAXIMAL (documente, audit point 5) : lancer le
# bootstrap directement depuis le commit, sans passer par le worktree :
#   C=$(git rev-parse HEAD) && git show "$C:gcp-migration/session_campagne_v6_g4.sh" \
#     > /tmp/boot.sh && MHGP6_BOOTSTRAP_COMMIT="$C" bash /tmp/boot.sh
# (l'etage 1 detecte alors qu'il est deja la copie du commit et continue).
#
# Les garanties d'arret vivent dans le cycle de vie
# (gcp-migration/v6_session_lifecycle.sh) ; leurs scenarios de rupture sont
# prouves par selftest_cycle_vie_v6.sh, et le runner/validateur par
# selftest_campagne_v6.sh — les DEUX a lancer A LA MAIN avant toute session
# payante. AUCUN lancement avant le GO d'un audit statique frais.
set -euo pipefail

if [ -z "${MHGP6_BOOTSTRAP_STAGE2:-}" ]; then
  # ---- ETAGE 1 (worktree ou copie du commit) : capture unique du commit,
  # materialisation, authentification de soi-meme, exec de l'etage 2.
  REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
  cd "${REPO_ROOT}"
  SOURCE_COMMIT="${MHGP6_BOOTSTRAP_COMMIT:-$(git rev-parse HEAD)}"
  WORK="$(mktemp -d /tmp/ehgp-v6session.XXXXXXXX)"
  echo "session dans ${WORK} (commit impose ${SOURCE_COMMIT})"
  git show "${SOURCE_COMMIT}:gcp-migration/session_campagne_v6_g4.sh" > "${WORK}/bootstrap.sh"
  git show "${SOURCE_COMMIT}:gcp-migration/v6_campaign_pin.sh" > "${WORK}/pin.sh"
  chmod +x "${WORK}/bootstrap.sh" "${WORK}/pin.sh"
  SELF_SHA="$(sha256sum "$0" | awk '{print $1}')"
  PINNED_SHA="$(sha256sum "${WORK}/bootstrap.sh" | awk '{print $1}')"
  if [ "${SELF_SHA}" != "${PINNED_SHA}" ]; then
    echo "REFUS : le bootstrap execute (${SELF_SHA}) differe de la version du commit (${PINNED_SHA}) — committer d'abord" >&2
    exit 2
  fi
  MHGP6_BOOTSTRAP_STAGE2=1 MHGP6_BOOTSTRAP_WORK="${WORK}" \
    MHGP6_BOOTSTRAP_COMMIT="${SOURCE_COMMIT}" MHGP6_BOOTSTRAP_REPO_ROOT="${REPO_ROOT}" \
    exec bash "${WORK}/bootstrap.sh"
fi

# ---- ETAGE 2 : execute UNIQUEMENT des copies materialisees depuis le commit.
WORK="${MHGP6_BOOTSTRAP_WORK:?etage 2 sans WORK}"
SOURCE_COMMIT="${MHGP6_BOOTSTRAP_COMMIT:?etage 2 sans commit}"
cd "${MHGP6_BOOTSTRAP_REPO_ROOT:?etage 2 sans racine}"
echo "etage 2 : bootstrap materialise depuis ${SOURCE_COMMIT}"

PIN_OUT="$(bash "${WORK}/pin.sh" "${WORK}" "${SOURCE_COMMIT}")"
printf '%s\n' "${PIN_OUT}"
SOURCE_PAYLOAD_SHA256="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^source_payload_sha256=//p')"
PROTOCOL_MANIFEST_SHA256="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^protocol_manifest_sha256=//p')"
[ -n "${SOURCE_PAYLOAD_SHA256}" ] && [ -n "${PROTOCOL_MANIFEST_SHA256}" ] \
  || { echo "REFUS : pin incomplet" >&2; exit 2; }

# Le pin materialise doit etre identique au pin execute (defense en
# profondeur : l'etage 1 l'a materialise depuis le meme commit).
PIN_A="$(sha256sum "${WORK}/pin.sh" | awk '{print $1}')"
PIN_B="$(sha256sum "${WORK}/pinned/gcp-migration/v6_campaign_pin.sh" | awk '{print $1}')"
[ "${PIN_A}" = "${PIN_B}" ] || { echo "REFUS : pin execute != pin du manifeste" >&2; exit 2; }

# Reçu durable OBLIGATOIRE (audit deuxieme tour) : par defaut dans les
# receipts du depot, date + commit court.
export DURABLE_RECEIPT_DIR="${DURABLE_RECEIPT_DIR:-${MHGP6_BOOTSTRAP_REPO_ROOT}/morsehgp3D_v6/receipts/session_g4_$(date -u +%Y%m%d)_${SOURCE_COMMIT:0:12}}"

export MHGP6_LIFECYCLE_WORK="${WORK}"
export MHGP6_LIFECYCLE_GUARDS_DIR="${WORK}/pinned/gcp-migration"
export MHGP6_LIFECYCLE_SOURCE_COMMIT="${SOURCE_COMMIT}"
export MHGP6_LIFECYCLE_PAYLOAD_SHA256="${SOURCE_PAYLOAD_SHA256}"
export MHGP6_LIFECYCLE_MANIFEST_SHA256="${PROTOCOL_MANIFEST_SHA256}"
exec bash "${WORK}/pinned/gcp-migration/v6_session_lifecycle.sh"
