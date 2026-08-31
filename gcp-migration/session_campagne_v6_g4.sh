#!/usr/bin/env bash
# Session G4 — CAMPAGNE v6. BOOTSTRAP DEUX ETAGES (P0-2 des audits GCP v6,
# trois tours) : l'etage 1, seul code du worktree jamais execute, ne fait
# QUE valider la racine du depot, resoudre le commit, materialiser
# bootstrap + pin par `git -C <racine> show`, verifier sa propre identite,
# puis `exec` l'etage 2 (la copie du commit). L'etage 2 REFUSE une entree
# directe (marqueur d'etage forge) : il exige d'ETRE le bootstrap.sh du
# repertoire prive ET re-authentifie son propre contenu et le pin contre
# `git show` du commit — un WORK forge par l'appelant est refuse.
#
# POINT D'ENTREE DE CONFIANCE MAXIMAL (troisieme tour : execute HORS du
# depot, la racine est EXPLICITE) :
#   R=/chemin/vers/E-HGP
#   C=$(git -C "$R" rev-parse HEAD)
#   git -C "$R" show "$C:gcp-migration/session_campagne_v6_g4.sh" > /tmp/boot.sh
#   MHGP6_BOOTSTRAP_REPO_ROOT="$R" MHGP6_BOOTSTRAP_COMMIT="$C" bash /tmp/boot.sh
#
# Les garanties d'arret vivent dans le cycle de vie
# (gcp-migration/v6_session_lifecycle.sh) ; selftests a lancer A LA MAIN :
# selftest_cycle_vie_v6.sh et selftest_campagne_v6.sh. AUCUN lancement avant
# le GO d'un audit statique frais.
set -euo pipefail

if [ -z "${MHGP6_BOOTSTRAP_STAGE2:-}" ]; then
  # ---- ETAGE 1 : racine EXPLICITE ou derivee, canonisee et VALIDEE ; commit
  # normalise en hash complet et RESOLU ; materialisation ; identite ; exec.
  REPO_ROOT="${MHGP6_BOOTSTRAP_REPO_ROOT:-$(cd "$(dirname "$0")/.." && pwd)}"
  REPO_ROOT="$(cd "${REPO_ROOT}" 2>/dev/null && pwd -P)" \
    || { echo "REFUS : racine de depot invalide (${MHGP6_BOOTSTRAP_REPO_ROOT:-derivee})" >&2; exit 2; }
  git -C "${REPO_ROOT}" rev-parse --git-dir >/dev/null 2>&1 \
    || { echo "REFUS : ${REPO_ROOT} n'est pas un depot git" >&2; exit 2; }
  SOURCE_COMMIT="$(git -C "${REPO_ROOT}" rev-parse --verify "${MHGP6_BOOTSTRAP_COMMIT:-HEAD}^{commit}" 2>/dev/null)" \
    || { echo "REFUS : commit impose irresoluble (${MHGP6_BOOTSTRAP_COMMIT:-HEAD})" >&2; exit 2; }
  WORK="$(mktemp -d /tmp/ehgp-v6session.XXXXXXXX)"
  echo "session dans ${WORK} (racine ${REPO_ROOT}, commit ${SOURCE_COMMIT})"
  git -C "${REPO_ROOT}" show "${SOURCE_COMMIT}:gcp-migration/session_campagne_v6_g4.sh" > "${WORK}/bootstrap.sh"
  git -C "${REPO_ROOT}" show "${SOURCE_COMMIT}:gcp-migration/v6_campaign_pin.sh" > "${WORK}/pin.sh"
  chmod +x "${WORK}/bootstrap.sh" "${WORK}/pin.sh"
  SELF_SHA="$(sha256sum "$0" | awk '{print $1}')"
  PINNED_SHA="$(sha256sum "${WORK}/bootstrap.sh" | awk '{print $1}')"
  if [ "${SELF_SHA}" != "${PINNED_SHA}" ]; then
    echo "REFUS : le bootstrap execute (${SELF_SHA}) differe de la version du commit (${PINNED_SHA}) — committer d'abord ou passer par le point d'entree de confiance" >&2
    exit 2
  fi
  MHGP6_BOOTSTRAP_STAGE2=1 MHGP6_BOOTSTRAP_WORK="${WORK}" \
    MHGP6_BOOTSTRAP_COMMIT="${SOURCE_COMMIT}" MHGP6_BOOTSTRAP_REPO_ROOT="${REPO_ROOT}" \
    exec bash "${WORK}/bootstrap.sh"
fi

# ---- ETAGE 2 : REFUSE une entree directe. Le marqueur d'etage est un
# parametre public — la preuve du handoff est que $0 EST le bootstrap.sh du
# repertoire prive ET que son contenu comme le pin egalent `git show` du
# commit (re-authentification independante de WORK).
WORK="${MHGP6_BOOTSTRAP_WORK:?etage 2 sans WORK}"
SOURCE_COMMIT="${MHGP6_BOOTSTRAP_COMMIT:?etage 2 sans commit}"
REPO_ROOT="${MHGP6_BOOTSTRAP_REPO_ROOT:?etage 2 sans racine}"
if [ "$(readlink -f "$0")" != "$(readlink -f "${WORK}/bootstrap.sh")" ]; then
  echo "REFUS : entree directe en etage 2 (\$0 n'est pas le bootstrap du repertoire prive) — passer par l'etage 1" >&2
  exit 2
fi
SELF_SHA="$(sha256sum "$0" | awk '{print $1}')"
COMMIT_SHA="$(git -C "${REPO_ROOT}" show "${SOURCE_COMMIT}:gcp-migration/session_campagne_v6_g4.sh" | sha256sum | awk '{print $1}')"
[ "${SELF_SHA}" = "${COMMIT_SHA}" ] \
  || { echo "REFUS : l'etage 2 en cours ne correspond pas au bootstrap du commit" >&2; exit 2; }
PIN_SHA="$(sha256sum "${WORK}/pin.sh" | awk '{print $1}')"
PIN_COMMIT_SHA="$(git -C "${REPO_ROOT}" show "${SOURCE_COMMIT}:gcp-migration/v6_campaign_pin.sh" | sha256sum | awk '{print $1}')"
[ "${PIN_SHA}" = "${PIN_COMMIT_SHA}" ] \
  || { echo "REFUS : pin du repertoire prive different du pin du commit (WORK forge ?)" >&2; exit 2; }
cd "${REPO_ROOT}"
echo "etage 2 : bootstrap et pin re-authentifies contre ${SOURCE_COMMIT}"

PIN_OUT="$(bash "${WORK}/pin.sh" "${WORK}" "${SOURCE_COMMIT}")"
printf '%s\n' "${PIN_OUT}"
SOURCE_PAYLOAD_SHA256="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^source_payload_sha256=//p')"
PROTOCOL_MANIFEST_SHA256="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^protocol_manifest_sha256=//p')"
[ -n "${SOURCE_PAYLOAD_SHA256}" ] && [ -n "${PROTOCOL_MANIFEST_SHA256}" ] \
  || { echo "REFUS : pin incomplet" >&2; exit 2; }

# Reçu durable OBLIGATOIRE : BASE seulement — le cycle de vie y publie un
# run UNIQUE (identifiant avec generation) de facon atomique.
export DURABLE_RECEIPT_BASE="${DURABLE_RECEIPT_BASE:-${REPO_ROOT}/morsehgp3D_v6/receipts}"
export DURABLE_RECEIPT_PREFIX="session_g4_$(date -u +%Y%m%d)_${SOURCE_COMMIT:0:12}"

export MHGP6_LIFECYCLE_WORK="${WORK}"
export MHGP6_LIFECYCLE_GUARDS_DIR="${WORK}/pinned/gcp-migration"
export MHGP6_LIFECYCLE_SOURCE_COMMIT="${SOURCE_COMMIT}"
export MHGP6_LIFECYCLE_PAYLOAD_SHA256="${SOURCE_PAYLOAD_SHA256}"
export MHGP6_LIFECYCLE_MANIFEST_SHA256="${PROTOCOL_MANIFEST_SHA256}"
exec bash "${WORK}/pinned/gcp-migration/v6_session_lifecycle.sh"
