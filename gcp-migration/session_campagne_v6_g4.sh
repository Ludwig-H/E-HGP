#!/usr/bin/env bash
# Session G4 — CAMPAGNE v6. BOOTSTRAP MINIMAL (P0-2 de l'audit GCP v6) :
# ce lanceur ne touche JAMAIS GCP lui-meme. Il epingle le protocole complet
# depuis le COMMIT (v6_campaign_pin.sh : moteurs v5+v6, lanceur, cycle de
# vie, runner, validateur ET les trois gardes start/stop/set_max), verifie sa
# propre identite contre la copie materialisee, puis EXECUTE la copie du
# cycle de vie issue du commit (gcp-migration/v6_session_lifecycle.sh) avec
# les gardes materialisees — y compris dans le trap d'arret.
#
# Les garanties d'arret (trap avant mutation, temoin de mutation durable,
# table temoin/handoff, blocage explicite si generation illisible, arret
# cible unique) vivent dans le cycle de vie ; ses scenarios de rupture sont
# prouves par gcp-migration/selftest_cycle_vie_v6.sh (fausses gardes), et le
# protocole runner/validateur par selftest_campagne_v6.sh — les DEUX a
# lancer A LA MAIN avant toute session payante.
set -euo pipefail

# EXECUTION DEPUIS UNE COPIE : bash lit un script paresseusement ; une
# edition du fichier pendant une session longue corromprait sa suite.
if [ -z "${MHGP6_SESSION_SELF_COPY:-}" ]; then
  _self_copy="$(mktemp /tmp/ehgp-v6session-copy.XXXXXXXX.sh)"
  cp "$0" "${_self_copy}"
  MHGP6_SESSION_REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
  export MHGP6_SESSION_REPO_ROOT
  MHGP6_SESSION_SELF_COPY="${_self_copy}" MHGP6_SESSION_SOURCE="$0" exec bash "${_self_copy}" "$@"
fi
echo "bootstrap execute depuis la copie ${MHGP6_SESSION_SELF_COPY} (source : ${MHGP6_SESSION_SOURCE:-?})"

REPO_ROOT="${MHGP6_SESSION_REPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "${REPO_ROOT}"

WORK="$(mktemp -d /tmp/ehgp-v6session.XXXXXXXX)"
echo "session dans ${WORK}"

# ---- PIN DU PROTOCOLE COMPLET, avant toute action GCP.
PIN_OUT="$(./gcp-migration/v6_campaign_pin.sh "${WORK}")"
printf '%s\n' "${PIN_OUT}"
SOURCE_COMMIT="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^source_commit=//p')"
SOURCE_PAYLOAD_SHA256="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^source_payload_sha256=//p')"
PROTOCOL_MANIFEST_SHA256="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^protocol_manifest_sha256=//p')"
[ -n "${SOURCE_COMMIT}" ] && [ -n "${SOURCE_PAYLOAD_SHA256}" ] && [ -n "${PROTOCOL_MANIFEST_SHA256}" ] \
  || { echo "REFUS : pin incomplet" >&2; exit 2; }

# ---- IDENTITE DU BOOTSTRAP : la copie en cours d'execution doit etre
# OCTET POUR OCTET la version materialisee depuis le commit.
SELF_SHA="$(sha256sum "${MHGP6_SESSION_SELF_COPY}" | awk '{print $1}')"
PINNED_SHA="$(sha256sum "${WORK}/pinned/gcp-migration/session_campagne_v6_g4.sh" | awk '{print $1}')"
if [ "${SELF_SHA}" != "${PINNED_SHA}" ]; then
  echo "REFUS : le bootstrap execute (${SELF_SHA}) differe de la version du commit (${PINNED_SHA})" >&2
  exit 2
fi

# ---- EXECUTION DU CYCLE DE VIE EPINGLE, gardes epinglees incluses.
export MHGP6_LIFECYCLE_WORK="${WORK}"
export MHGP6_LIFECYCLE_GUARDS_DIR="${WORK}/pinned/gcp-migration"
export MHGP6_LIFECYCLE_SOURCE_COMMIT="${SOURCE_COMMIT}"
export MHGP6_LIFECYCLE_PAYLOAD_SHA256="${SOURCE_PAYLOAD_SHA256}"
export MHGP6_LIFECYCLE_MANIFEST_SHA256="${PROTOCOL_MANIFEST_SHA256}"
exec bash "${WORK}/pinned/gcp-migration/v6_session_lifecycle.sh"
