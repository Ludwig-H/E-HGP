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
# Un recu de REPRISE (superviseur perdu) n'est jamais requalifie en resultat.
case "$(field issue)" in reprise_*) echo "REFUS : recu de reprise (issue=$(field issue)) — jamais une decision ni une mesure recevable" >&2; exit 2 ;; esac
COMMIT="$(field source_commit)"
PAYLOAD="$(field source_payload_sha256)"
MANIFEST="$(field protocol_manifest_sha256)"
# § 5.18.5 : les codes de session viennent des lignes UNIQUES du journal
# (remote_campaign_rc=, scp_rc=), jamais du rc global du recu ni d'un zero.
RC_REMOTE="$(grep -E '^remote_campaign_rc=[0-9]+$' "${RECU}/session.log" | sed 's/.*=//' | sort -u)"
SCP_RC="$(grep -E '^scp_rc=[0-9]+$' "${RECU}/session.log" | sed 's/.*=//' | sort -u)"
[[ "${RC_REMOTE}" =~ ^[0-9]+$ ]] || { echo "REFUS : remote_campaign_rc absent ou multiple dans session.log" >&2; exit 2; }
[[ "${SCP_RC}" =~ ^[0-9]+$ ]] || { echo "REFUS : scp_rc absent ou multiple dans session.log" >&2; exit 2; }
STOP_RC="$(field stop_rc)"
PROFIL_NOM="$(sed -n 's/^profil_canonique=//p' "${RECU}/profil_campagne.txt" | head -n 1)"
[[ "${COMMIT}" =~ ^[0-9a-f]{40}$ ]] || { echo "REFUS : source_commit illisible" >&2; exit 2; }
[[ "${PROFIL_NOM}" =~ ^[a-z0-9_]+$ ]] || { echo "REFUS : profil_canonique illisible" >&2; exit 2; }
WORK="$(mktemp -d "${TMPDIR:-/tmp}/ehgp-revalid.XXXXXXXX")"
trap 'rm -rf "${WORK}"' EXIT
# Le repertoire des resumes ne peut etre ni le recu ni un sous-repertoire.
RECU_ABS="$(cd "${RECU}" && pwd -P)"
case "$(cd "${WORK}" && pwd -P)/" in "${RECU_ABS}/"*) echo "REFUS : V6_RESUMES_DIR dans le recu" >&2; exit 2 ;; esac
cp -r "${RECU}/out" "${WORK}/out"
git -C "${ROOT}" show "${COMMIT}:gcp-migration/profils/${PROFIL_NOM}.env" > "${WORK}/${PROFIL_NOM}.env"
echo "recu ${RECU} : commit ${COMMIT:0:12}, profil ${PROFIL_NOM}, remote_campaign_rc ${RC_REMOTE}, scp_rc ${SCP_RC}, stop_rc ${STOP_RC}"
rc=0
V6_RESUMES_DIR="${WORK}" python3 "${VALIDATOR}" "${WORK}/out" "${COMMIT}" "${PAYLOAD}" "${MANIFEST}" \
  "${RC_REMOTE}" "${SCP_RC}" "${RECU}/profil_campagne.txt" "${WORK}/${PROFIL_NOM}.env" "${RECU}/manifest_revalide.txt" || rc=$?
echo "validateur (${VALIDATOR}) : rc=${rc}"
for r in bench queue sweep gpu frontier matrice gpuv6; do
  if [ -f "${RECU}/${r}_resume.txt" ] && [ -f "${WORK}/${r}_resume.txt" ]; then
    if cmp -s "${RECU}/${r}_resume.txt" "${WORK}/${r}_resume.txt"; then
      echo "resume ${r} : identique au recu"
    else
      echo "resume ${r} : DIFFERENT du recu (le recu reste intact)"
      { diff "${RECU}/${r}_resume.txt" "${WORK}/${r}_resume.txt" || true; } | head -5  # affichage seul, jamais un court-circuit
    fi
  fi
done
( cd "${RECU}" && sha256sum -c --quiet SHA256SUMS ) && echo "recu intact apres re-validation (SHA256SUMS verifie)"
exit "${rc}"
