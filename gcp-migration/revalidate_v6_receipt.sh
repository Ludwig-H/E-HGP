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
# § 5.22 : le validateur est le CANONIQUE du worktree ; un autre chemin n'est
# admis qu'en mode selftest EXPLICITE (EHGP_REVALIDATE_SELFTEST=1) ; son
# sha256 est grave dans la sortie.
VALIDATOR_CANON="${HERE}/validate_v6_campaign.py"
VALIDATOR="${2:-${VALIDATOR_CANON}}"
if [ "$(readlink -f "${VALIDATOR}")" != "$(readlink -f "${VALIDATOR_CANON}")" ] && [ "${EHGP_REVALIDATE_SELFTEST:-0}" != "1" ]; then
  echo "REFUS : validateur non canonique (${VALIDATOR}) hors mode selftest explicite" >&2; exit 2
fi
[ -f "${VALIDATOR}" ] || { echo "REFUS : validateur absent (${VALIDATOR})" >&2; exit 2; }
VALIDATOR_SHA="$(sha256sum "${VALIDATOR}" | awk '{print $1}')"
[ -f "${RECU}/RECU_SESSION.txt" ] && [ -d "${RECU}/out" ] && [ -f "${RECU}/SHA256SUMS" ] \
  || { echo "REFUS : ${RECU} n'est pas un recu durable complet (RECU_SESSION.txt, out/, SHA256SUMS)" >&2; exit 2; }
( cd "${RECU}" && sha256sum -c --quiet SHA256SUMS ) || { echo "REFUS : SHA256SUMS du recu non verifie" >&2; exit 2; }
# ENSEMBLE EXACT (§ 5.19.3) : tout fichier present est liste (aucun fichier
# supplementaire non hache) et les sept resumes durables + les pieces de
# session existent — un recu ampute ou augmente n'est jamais re-juge.
# Inventaire de TOUTES les entrees : seuls des fichiers reguliers et les
# repertoires attendus (out, marques) — un lien symbolique ou un type
# special non hache n'est jamais un recu. § 5.22 : enregistrements NUL de bout
# en bout (type, mode, nom) pour un inventaire INJECTIF ; un nom portant un
# saut de ligne est refuse d'emblee ; seul le manifeste RACINE est special
# (un out/SHA256SUMS ou marques/SHA256SUMS est un fichier comme un autre).
if [ -n "$(cd "${RECU}" && find . -mindepth 1 -name "$(printf '*\n*')" -print -quit)" ]; then
  echo "REFUS : nom d'entree contenant un saut de ligne dans le recu" >&2; exit 2
fi
IRREGULIERS="$(cd "${RECU}" && find . -mindepth 1 ! -type f ! -type d -printf '%y %P\n')"
[ -z "${IRREGULIERS}" ] || { echo "REFUS : entree non reguliere dans le recu (lien ou type special) : ${IRREGULIERS}" >&2; exit 2; }
REPERTOIRES="$(cd "${RECU}" && find . -mindepth 1 -type d -printf '%P\n' | sort)"
for d in ${REPERTOIRES}; do
  case "${d}" in out|marques) ;; *) echo "REFUS : repertoire inattendu dans le recu (${d})" >&2; exit 2 ;; esac
done
inventaire_sha() { # empreinte NUL-separee (type, mode, nom) de toutes les entrees hors ./SHA256SUMS
  ( cd "${RECU}" && find . -mindepth 1 ! -path ./SHA256SUMS -printf '%y %m %P\0' | sort -z | sha256sum | awk '{print $1}' )
}
LISTES="$(awk '{sub(/^\*/, "", $2); sub(/^\.\//, "", $2); print $2}' "${RECU}/SHA256SUMS" | sort)"
PRESENTS="$(cd "${RECU}" && find . -type f ! -path ./SHA256SUMS -printf '%P\n' | sort)"
[ "${LISTES}" = "${PRESENTS}" ] || {
  echo "REFUS : ensemble des fichiers du recu != SHA256SUMS (fichier non liste, liste sans fichier ou entree dupliquee)" >&2
  diff <(printf '%s\n' "${LISTES}") <(printf '%s\n' "${PRESENTS}") | head -n 5 >&2 || true
  exit 2
}
# Pieces durables exigees : celles de la session ; les resumes serie C
# (matrice, gpuv6) seulement quand le profil du recu porte ces axes (recus
# anterieurs a la serie C : cinq resumes).
PIECES="RECU_SESSION.txt session.log validation.txt profil_campagne.txt manifest_revalide.txt bench_resume.txt queue_resume.txt sweep_resume.txt gpu_resume.txt frontier_resume.txt"
if grep -qE '^matrice_points=' "${RECU}/profil_campagne.txt"; then
  PIECES="${PIECES} matrice_resume.txt gpuv6_resume.txt"
fi
for f in ${PIECES}; do
  [ -f "${RECU}/${f}" ] || { echo "REFUS : piece durable absente du recu (${f})" >&2; exit 2; }
done
# Le MANIFESTE INITIAL est lie par ses octets : un validateur qui altererait
# une piece puis regenererait SHA256SUMS ne peut pas passer le controle final.
MANIFESTE_INITIAL_SHA="$(sha256sum "${RECU}/SHA256SUMS" | awk '{print $1}')"
INVENTAIRE_INITIAL_SHA="$(inventaire_sha)"
field() { sed -n "s/^$1=//p" "${RECU}/RECU_SESSION.txt" | head -n 1; }
# Un recu de REPRISE (superviseur perdu) n'est jamais requalifie en resultat.
case "$(field issue)" in reprise_*) echo "REFUS : recu de reprise (issue=$(field issue)) — jamais une decision ni une mesure recevable" >&2; exit 2 ;; esac
COMMIT="$(field source_commit)"
PAYLOAD="$(field source_payload_sha256)"
MANIFEST="$(field protocol_manifest_sha256)"
# § 5.18.5 : les codes de session viennent des lignes UNIQUES du journal
# (remote_campaign_rc=, scp_rc=), jamais du rc global du recu ni d'un zero.
# EXACTEMENT UNE occurrence de chaque ligne (§ 5.19.3 : `sort -u` acceptait
# deux lignes identiques — un journal rejoue n'est pas un journal).
[ "$(grep -cE '^remote_campaign_rc=[0-9]+$' "${RECU}/session.log")" = "1" ] \
  || { echo "REFUS : remote_campaign_rc absent ou multiple dans session.log (exactement une ligne exigee)" >&2; exit 2; }
[ "$(grep -cE '^scp_rc=[0-9]+$' "${RECU}/session.log")" = "1" ] \
  || { echo "REFUS : scp_rc absent ou multiple dans session.log (exactement une ligne exigee)" >&2; exit 2; }
RC_REMOTE="$(grep -E '^remote_campaign_rc=[0-9]+$' "${RECU}/session.log" | sed 's/.*=//')"
SCP_RC="$(grep -E '^scp_rc=[0-9]+$' "${RECU}/session.log" | sed 's/.*=//')"
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
echo "validateur_sha256=${VALIDATOR_SHA} ($( [ "${VALIDATOR}" = "${VALIDATOR_CANON}" ] && echo canonique || echo "selftest : ${VALIDATOR}" ))"
rc=0
V6_RESUMES_DIR="${WORK}" python3 "${VALIDATOR}" "${WORK}/out" "${COMMIT}" "${PAYLOAD}" "${MANIFEST}" \
  "${RC_REMOTE}" "${SCP_RC}" "${RECU}/profil_campagne.txt" "${WORK}/${PROFIL_NOM}.env" "${RECU}/manifest_revalide.txt" || rc=$?
echo "validateur (${VALIDATOR}) : rc=${rc}"
# § 5.22 : chaque resume attendu (present au recu) doit avoir ete RE-PRODUIT
# par le validateur et est compare — un validateur muet ou arbitraire qui
# rend 0 sans ecrire les resumes est refuse (rc 3), jamais « recu intact ».
RESUMES_ABSENTS=""
for r in bench queue sweep gpu frontier matrice gpuv6; do
  [ -f "${RECU}/${r}_resume.txt" ] || continue
  if [ ! -f "${WORK}/${r}_resume.txt" ]; then RESUMES_ABSENTS="${RESUMES_ABSENTS} ${r}"; continue; fi
  if cmp -s "${RECU}/${r}_resume.txt" "${WORK}/${r}_resume.txt"; then
    echo "resume ${r} : identique au recu"
  else
    echo "resume ${r} : DIFFERENT du recu (le recu reste intact)"
    { diff "${RECU}/${r}_resume.txt" "${WORK}/${r}_resume.txt" || true; } | head -5  # affichage seul, jamais un court-circuit
  fi
done
if [ -n "${RESUMES_ABSENTS}" ]; then
  echo "VALIDATEUR MUET : resumes attendus non produits (${RESUMES_ABSENTS# }) — verdict rejete" >&2
  exit 3
fi
# Le controle final DOMINE le code du validateur (§ 5.19.3 : un validateur
# qui altere le recu ne peut pas rendre 0 par un `&&` a gauche).
# § 5.21 : l'inventaire des REPERTOIRES est lui aussi recompare (un
# validateur qui cree seulement un repertoire vide n'est pas « intact »).
# « intact » = noms, types, modes et octets : manifeste initial identique,
# hashes verifies, inventaire NUL (type, mode, nom) identique.
if [ "$(sha256sum "${RECU}/SHA256SUMS" | awk '{print $1}')" = "${MANIFESTE_INITIAL_SHA}" ] \
   && ( cd "${RECU}" && sha256sum -c --quiet SHA256SUMS ) \
   && [ "$(inventaire_sha)" = "${INVENTAIRE_INITIAL_SHA}" ]; then
  echo "recu intact apres re-validation (manifeste initial identique, SHA256SUMS verifie, inventaire types/modes/noms inchange)"
else
  echo "RECU ALTERE PENDANT LA RE-VALIDATION (manifeste, hashes ou inventaire types/modes/noms) — verdict rejete" >&2
  exit 3
fi
exit "${rc}"
