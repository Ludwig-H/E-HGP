#!/usr/bin/env bash
# RE-VALIDATION D'UN RECU DURABLE v6 SANS LE MODIFIER (audit serie C) : le
# validateur du worktree (eventuellement corrige apres la session) rejuge
# une COPIE du recu ; le profil canonique est tire de `git show
# <source_commit>:gcp-migration/profils/<PROFIL_NOM>.env` (le fichier du
# worktree peut avoir change de sha256 depuis) ; les resumes sont ecrits
# dans le repertoire de travail (V6_RESUMES_DIR), jamais dans le recu, et
# compares a ceux du recu. Ne touche JAMAIS GCP. Sortie : verdict du
# validateur + diff des resumes ; code = code du validateur.
#   ./gcp-migration/revalidate_v6_receipt.sh morsehgp3D_v6/receipts/<run_id> [validateur]
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${HERE}/.." && pwd)"
RECU="${1:?recu durable requis}"
VALIDATOR="${2:-${HERE}/validate_v6_campaign.py}"
[ -f "${RECU}/RECU_SESSION.txt" ] && [ -d "${RECU}/out" ] && [ -f "${RECU}/SHA256SUMS" ] \
  || { echo "REFUS : ${RECU} n'est pas un recu durable complet (RECU_SESSION.txt, out/, SHA256SUMS)" >&2; exit 2; }
( cd "${RECU}" && sha256sum -c --quiet SHA256SUMS ) || { echo "REFUS : SHA256SUMS du recu non verifie" >&2; exit 2; }
field() { sed -n "s/^$1=//p" "${RECU}/RECU_SESSION.txt" | head -n 1; }
COMMIT="$(field source_commit)"
PAYLOAD="$(field source_payload_sha256)"
MANIFEST="$(field protocol_manifest_sha256)"
RC_REMOTE="$(field rc)"
STOP_RC="$(field stop_rc)"
PROFIL_NOM="$(sed -n 's/^profil_canonique=//p' "${RECU}/profil_campagne.txt" | head -n 1)"
[[ "${COMMIT}" =~ ^[0-9a-f]{40}$ ]] || { echo "REFUS : source_commit illisible" >&2; exit 2; }
[[ "${PROFIL_NOM}" =~ ^[a-z0-9_]+$ ]] || { echo "REFUS : profil_canonique illisible" >&2; exit 2; }
WORK="$(mktemp -d "${TMPDIR:-/tmp}/ehgp-revalid.XXXXXXXX")"
trap 'rm -rf "${WORK}"' EXIT
cp -r "${RECU}/out" "${WORK}/out"
git -C "${ROOT}" show "${COMMIT}:gcp-migration/profils/${PROFIL_NOM}.env" > "${WORK}/${PROFIL_NOM}.env"
echo "recu ${RECU} : commit ${COMMIT:0:12}, profil ${PROFIL_NOM}, rc distant ${RC_REMOTE}, stop_rc ${STOP_RC}"
rc=0
V6_RESUMES_DIR="${WORK}" python3 "${VALIDATOR}" "${WORK}/out" "${COMMIT}" "${PAYLOAD}" "${MANIFEST}" \
  "${RC_REMOTE}" 0 "${RECU}/profil_campagne.txt" "${WORK}/${PROFIL_NOM}.env" "${RECU}/manifest_revalide.txt" || rc=$?
echo "validateur (${VALIDATOR}) : rc=${rc}"
for r in bench queue sweep gpu frontier matrice gpuv6; do
  if [ -f "${RECU}/${r}_resume.txt" ] && [ -f "${WORK}/${r}_resume.txt" ]; then
    if cmp -s "${RECU}/${r}_resume.txt" "${WORK}/${r}_resume.txt"; then
      echo "resume ${r} : identique au recu"
    else
      echo "resume ${r} : DIFFERENT du recu (le recu reste intact)"; diff "${RECU}/${r}_resume.txt" "${WORK}/${r}_resume.txt" | head -5
    fi
  fi
done
( cd "${RECU}" && sha256sum -c --quiet SHA256SUMS ) && echo "recu intact apres re-validation (SHA256SUMS verifie)"
exit "${rc}"
