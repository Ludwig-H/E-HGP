#!/usr/bin/env bash
# REPRISE APRES PERTE DU SUPERVISEUR (audit serie C § 5.18.6, durcie par
# CONTRE_AUDIT_REPRISE_PERSISTANTE_V6_20260902). Deux etages, comme le
# bootstrap : l'etage 1 (seul code non epingle execute) lit session.env, se
# re-authentifie contre `git show <commit>` (lui-meme ET la copie epinglee),
# puis `exec` la copie epinglee en etage 2. L'etage 2 :
#   - ne demarre JAMAIS la VM (aucun set_max, aucun start) ;
#   - n'execute QUE la garde d'arret EPINGLEE et re-authentifiee contre le
#     commit (jamais un chemin de gardes lu dans session.env) ;
#   - EXCLUSION par verrou noyau (`flock -n` sur un descripteur du WORK,
#     tenu jusqu'a la sortie) ; pid/starttime ne sont qu'un diagnostic ;
#   - refuse tant qu'un processus de la session vit : pid + starttime +
#     boot_id du superviseur, TOUT membre vivant de sa session/groupe de
#     processus (sid/pgid graves), et tout processus referencant WORK ;
#   - refuse si la session est deja conclue (recu_publie) ;
#   - registre strict : un etat qui implique une cible demarree porte une
#     generation NON VIDE, sinon BLOCAGE 71 sans conclusion ni purge ;
#   - marques parsees comme des objets stricts (fichier regulier, keyset
#     exact, cles uniques, mark=<nom>, generation non vide, cible exacte) ;
#   - registre `targeted_stopped` de la generation => deja certifie : aucun
#     appel GCP ; purge des credentials (re-jouable) PUIS temoin terminal ;
#   - entree en `targeted_stop_failed` / `targeted_stopping` => STOP-FIRST :
#     aucun describe, scp, validateur ni copie avant l'arret certifie ;
#   - sans `double_guard_verified` => stop_and_verify immediat sur la
#     generation exacte, aucune scp (issue=reprise_sans_double_garde) ;
#   - avec => describe (tuple status/generation EXACT), scp bornee vers un
#     STAGING, relecture du tuple apres la scp, promotion seulement si la
#     generation est inchangee ; toute divergence rend 71, detruit le staging
#     et ne STOPPE JAMAIS une autre generation ; REMOTE_DIR lie a
#     (SOURCE_COMMIT, epoque de la generation) ;
#   - stop_and_verify epingle (une tentative), validateur epingle borne
#     SEULEMENT apres un arret certifie, classification FORCEE
#     partiel_ou_invalide (issue=reprise_apres_perte_superviseur) ;
#   - arret NON certifie => rc 70 avec un temoin MINIMAL borne (aucune copie
#     recursive, aucun validateur, aucun hash massif) ;
#   - purge des credentials VERIFIEE avant le temoin terminal (mkstemp +
#     rename + fsync du parent) ; purge en echec => aucun temoin, rc 67, et
#     une reprise ulterieure re-purge SANS appel GCP ;
#   - generation inconnue => BLOCAGE 71 : describe en lecture seule affiche,
#     commande d'arret a copier, jamais un arret aveugle ;
#   - recu durable `<prefix>_<gen_epoch>_reprise_<epoch>_<pid>`, jamais une
#     decision.
# POLITIQUE DES REJEUX (explicite) : les rejeux sont MANUELS, idempotents,
# serialises par le verrou, lies a la generation et toujours stop-first ;
# aucune boucle automatique ; ils sont BORNES PAR TENTATIVE (un arret cible
# par rejeu), pas par un ledger persistant — aucun compteur ni deadline par
# generation n'est promis. PORTEE de la vivacite : l'identite sid/pgid
# enregistree par le superviseur ; un descendant qui refait `setsid` et retire
# WORK de son argv n'est pas couvert (un cgroup dedie le serait).
# Usage : bash recover_v6_session.sh <WORK>   (point d'entree de confiance :
#   git -C <racine> show <commit>:gcp-migration/recover_v6_session.sh > /tmp/rec.sh
#   bash /tmp/rec.sh <WORK>)
set -euo pipefail
umask 077

WORK="${1:?repertoire de session requis}"
[[ "${WORK}" = /* ]] || { echo "REFUS : WORK doit etre absolu" >&2; exit 2; }
[ ! -L "${WORK}" ] && [ -d "${WORK}" ] && [ "$(stat -c '%a %u' "${WORK}")" = "700 $(id -u)" ] \
  || { echo "REFUS : WORK (${WORK}) doit etre un repertoire 0700 non symbolique du proprietaire" >&2; exit 2; }
SESSION_ENV="${WORK}/session.env"
[ -f "${SESSION_ENV}" ] || { echo "REFUS : session.env absent (session jamais preparee)" >&2; exit 2; }

# Lecteur STRICT de session.env : cle=valeur, cles connues, alphabets fermes.
env_field() { # $1 = cle ; vide si absente
  awk -F= -v k="$1" '$1==k {sub(/^[^=]*=/, ""); print; exit}' "${SESSION_ENV}"
}
head -n 1 "${SESSION_ENV}" | grep -qx 'schema=e-hgp.session-env.v1' || { echo "REFUS : session.env hors schema" >&2; exit 2; }
_path_re='^/[A-Za-z0-9_./-]+$'
_tok_re='^[A-Za-z0-9_:.-]*$'
for k in REPO_ROOT DURABLE_RECEIPT_BASE GUARDS_DIR; do
  [[ "$(env_field "$k")" =~ ${_path_re} ]] || { echo "REFUS : session.env ${k} hors alphabet de chemin" >&2; exit 2; }
done
for k in GCP_PROJECT_ID GCP_ZONE GCP_INSTANCE_NAME SOURCE_COMMIT SOURCE_PAYLOAD_SHA256 PROTOCOL_MANIFEST_SHA256 \
         DURABLE_RECEIPT_PREFIX MAX_RUN_SECONDS GUEST_SHUTDOWN_MINUTES EFFECTIVE_CUTOFF_S GRACE_S \
         SCP_STEP_TIMEOUT_S DESCRIBE_TIMEOUT_S STOP_RESERVE_S VALIDATOR_TIMEOUT_S CAMPAIGN_PROFILE \
         EFFECTIVE_PROFILE GCP_SSH_KEY_EXPIRATION_UTC REMOTE_DIR; do
  [[ "$(env_field "$k")" =~ ${_tok_re} ]] || { echo "REFUS : session.env ${k} hors alphabet" >&2; exit 2; }
done
REPO_ROOT="$(env_field REPO_ROOT)"
SOURCE_COMMIT="$(env_field SOURCE_COMMIT)"
[[ "${SOURCE_COMMIT}" =~ ^[0-9a-f]{40}$ ]] || { echo "REFUS : SOURCE_COMMIT illisible" >&2; exit 2; }

if [ -z "${MHGP6_RECOVER_STAGE2:-}" ]; then
  # ---- ETAGE 1 : re-authentification contre le COMMIT, puis exec de la copie.
  git -C "${REPO_ROOT}" cat-file -e "${SOURCE_COMMIT}^{commit}" 2>/dev/null \
    || { echo "REFUS : commit ${SOURCE_COMMIT} absent du depot ${REPO_ROOT}" >&2; exit 2; }
  SELF_SHA="$(sha256sum "$0" | awk '{print $1}')"
  REF_SHA="$(git -C "${REPO_ROOT}" show "${SOURCE_COMMIT}:gcp-migration/recover_v6_session.sh" | sha256sum | awk '{print $1}')"
  [ "${SELF_SHA}" = "${REF_SHA}" ] \
    || { echo "REFUS : la reprise executee differe de la version du commit — passer par le point d'entree de confiance" >&2; exit 2; }
  PINNED="${WORK}/pinned/gcp-migration/recover_v6_session.sh"
  [ -f "${PINNED}" ] || { echo "REFUS : copie epinglee de la reprise absente (${PINNED})" >&2; exit 2; }
  [ "$(sha256sum "${PINNED}" | awk '{print $1}')" = "${REF_SHA}" ] \
    || { echo "REFUS : copie epinglee de la reprise alteree" >&2; exit 2; }
  echo "etage 1 : reprise re-authentifiee contre ${SOURCE_COMMIT}"
  MHGP6_RECOVER_STAGE2=1 exec bash "${PINNED}" "${WORK}"
fi

# ---- ETAGE 2 (copie epinglee) : jamais une entree directe.
[ "$(readlink -f "$0")" = "$(readlink -f "${WORK}/pinned/gcp-migration/recover_v6_session.sh")" ] \
  || { echo "REFUS : entree directe en etage 2 de la reprise" >&2; exit 2; }
PINNED_DIR="${WORK}/pinned/gcp-migration"
# Chaque fichier epingle egale sa version du commit (jamais un manifeste
# auto-reference recalcule sur le WORK lui-meme). La garde d'arret executee
# est TOUJOURS la copie epinglee re-authentifiee : le GUARDS_DIR de
# session.env n'est ni lu ni execute par la reprise (contre-audit, point 5).
for f in v6_session_lifecycle.sh validate_v6_campaign.py recover_v6_session.sh stop_and_verify.sh; do
  [ "$(sha256sum "${PINNED_DIR}/${f}" | awk '{print $1}')" = "$(git -C "${REPO_ROOT}" show "${SOURCE_COMMIT}:gcp-migration/${f}" | sha256sum | awk '{print $1}')" ] \
    || { echo "REFUS : ${f} epingle differe du commit ${SOURCE_COMMIT}" >&2; exit 2; }
done
STOP_GUARD="${PINNED_DIR}/stop_and_verify.sh"
[ -x "${STOP_GUARD}" ] || { echo "REFUS : garde d'arret epinglee non executable (${STOP_GUARD})" >&2; exit 2; }

export GCP_PROJECT_ID="$(env_field GCP_PROJECT_ID)"
export GCP_ZONE="$(env_field GCP_ZONE)"
export GCP_INSTANCE_NAME="$(env_field GCP_INSTANCE_NAME)"
SOURCE_PAYLOAD_SHA256="$(env_field SOURCE_PAYLOAD_SHA256)"
PROTOCOL_MANIFEST_SHA256="$(env_field PROTOCOL_MANIFEST_SHA256)"
DURABLE_RECEIPT_BASE="$(env_field DURABLE_RECEIPT_BASE)"
DURABLE_RECEIPT_PREFIX="$(env_field DURABLE_RECEIPT_PREFIX)"
MAX_RUN_SECONDS="$(env_field MAX_RUN_SECONDS)"
GUEST_SHUTDOWN_MINUTES="$(env_field GUEST_SHUTDOWN_MINUTES)"
EFFECTIVE_CUTOFF_S="$(env_field EFFECTIVE_CUTOFF_S)"
GRACE_S="$(env_field GRACE_S)"
SCP_STEP_TIMEOUT_S="$(env_field SCP_STEP_TIMEOUT_S)"
DESCRIBE_TIMEOUT_S="$(env_field DESCRIBE_TIMEOUT_S)"
STOP_RESERVE_S="$(env_field STOP_RESERVE_S)"
VALIDATOR_TIMEOUT_S="$(env_field VALIDATOR_TIMEOUT_S)"
CAMPAIGN_PROFILE="$(env_field CAMPAIGN_PROFILE)"
EFFECTIVE_PROFILE="$(env_field EFFECTIVE_PROFILE)"
GCP_SSH_KEY_EXPIRATION_UTC="$(env_field GCP_SSH_KEY_EXPIRATION_UTC)"
REMOTE_DIR="$(env_field REMOTE_DIR)"
for v in MAX_RUN_SECONDS EFFECTIVE_CUTOFF_S GRACE_S SCP_STEP_TIMEOUT_S DESCRIBE_TIMEOUT_S STOP_RESERVE_S VALIDATOR_TIMEOUT_S; do
  [[ "${!v}" =~ ^[1-9][0-9]*$ ]] || { echo "REFUS : ${v} non entier dans session.env" >&2; exit 2; }
done
export CLOUDSDK_CONFIG="${WORK}/gcloud-config"
export GCP_SSH_KEY_FILE="${WORK}/ssh/id_ed25519"
export GCP_SSH_KEY_EXPIRATION_UTC
STATE_FILE="${WORK}/etat_cycle_vie"; HANDOFF="${WORK}/handoff.json"; MARKS_DIR="${WORK}/marques"
PID_FILE="${WORK}/superviseur.pid"; LOCK_FILE="${WORK}/reprise.lock"; LOCK_DIAG="${WORK}/reprise.pid"; RLOG="${WORK}/reprise.log"
rlog() { printf '[%s] %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" | tee -a "${RLOG}"; }

# ---- VERROU NOYAU (contre-audit, point 1) : flock -n sur un descripteur du
# WORK, tenu jusqu'a la sortie du processus ; deux reprises simultanees —
# meme depuis la meme session POSIX — ne peuvent pas franchir ensemble.
# § 5.21 : TOUS les choix terminaux (session deja conclue, registre, marques,
# generation) sont faits SOUS le verrou — un repreneur suspendu avant le
# verrou ne peut pas conclure sur un etat lu avant.
exec 9>>"${LOCK_FILE}"
flock -n 9 || { echo "REFUS : une reprise est deja en cours (verrou ${LOCK_FILE})" >&2; exit 2; }
printf '%s %s %s\n' "$$" "$(sed 's/.*) //' /proc/$$/stat | awk '{print $20}')" "$(cat /proc/sys/kernel/random/boot_id)" > "${LOCK_DIAG}"
# Un temoin est un FICHIER REGULIER ; toute autre entree a ce nom n'est pas
# une conclusion (elle fera echouer la publication atomique => code 68).
[ ! -f "${WORK}/recu_publie" ] || { echo "REFUS : session deja conclue (recu $(cat "${WORK}/recu_publie"))" >&2; exit 2; }

# ---- VIVACITE : le superviseur (pid + starttime + boot_id), TOUT membre
# vivant de sa session / de son groupe de processus (sid, pgid graves par
# le cycle de vie), et tout processus referencant WORK doivent etre morts.
proc_alive() { # $1 = pid, $2 = starttime, $3 = boot_id  -> 0 si vivant et identique
  local pid="$1" st="$2" bid="$3" cur
  [ "$(cat /proc/sys/kernel/random/boot_id)" = "${bid}" ] || return 1
  [ -r "/proc/${pid}/stat" ] || return 1
  cur="$(sed 's/.*) //' "/proc/${pid}/stat" | awk '{print $20}')"
  [ "${cur}" = "${st}" ]
}
sup_pid=""; sup_st=""; sup_bid=""; sup_sid=""; sup_pgid=""
if [ -f "${PID_FILE}" ]; then
  read -r sup_pid sup_st sup_bid sup_sid sup_pgid < "${PID_FILE}" || true
  if [[ "${sup_pid:-}" =~ ^[0-9]+$ ]] && proc_alive "${sup_pid}" "${sup_st:-x}" "${sup_bid:-x}"; then
    echo "REFUS : superviseur vivant (pid ${sup_pid}) — aucune reprise pendant qu'il court" >&2; exit 2
  fi
  # Membres orphelins de la session/du groupe du superviseur (meme boot) :
  # un fils dont l'argv ne porte plus WORK est quand meme un survivant.
  if [ "$(cat /proc/sys/kernel/random/boot_id)" = "${sup_bid:-x}" ]; then
    my_sid_early="$(ps -o sid= -p "$$" | tr -d ' ')"
    for kind in s g; do
      id=""; [ "${kind}" = s ] && id="${sup_sid:-}"; [ "${kind}" = g ] && id="${sup_pgid:-}"
      [[ "${id}" =~ ^[0-9]+$ ]] || continue
      [ "${id}" != "${my_sid_early}" ] || continue
      surv="$(pgrep "-${kind}" "${id}" 2>/dev/null | grep -vx "$$" || true)"
      if [ -n "${surv}" ]; then
        echo "REFUS : des membres de la session/du groupe du superviseur (${kind}id ${id}) vivent encore :" >&2
        ps -o pid,ppid,sid,pgid,args --no-headers -p "${surv//$'\n'/,}" >&2 || true
        exit 2
      fi
    done
  fi
fi
# Motif a crochet (la commande qui le porte ne se matche pas) ET exclusion
# de toute notre propre session de processus (les sous-shells d'une
# substitution partagent notre cmdline) : seuls les survivants d'une AUTRE
# session — celle du superviseur — comptent.
my_sid="$(ps -o sid= -p "$$" | tr -d ' ')"
others=""
for cand in $(pgrep -f -- "${WORK%?}[${WORK: -1}]" 2>/dev/null || true); do
  cand_sid="$(ps -o sid= -p "${cand}" 2>/dev/null | tr -d ' ' || true)"  # pid deja mort : ps rend 1
  [ -n "${cand_sid}" ] || continue
  [ "${cand_sid}" != "${my_sid}" ] || continue
  others="${others}${cand}"$'\n'
done
others="${others%$'\n'}"
if [ -n "${others}" ]; then
  echo "REFUS : des processus referencent encore ${WORK} — attendre ou tuer la session :" >&2
  ps -o pid,ppid,sid,pgid,args --no-headers -p "${others//$'\n'/,}" >&2 || true
  exit 2
fi
rlog "reprise ${WORK} : cible ${GCP_PROJECT_ID}/${GCP_ZONE}/${GCP_INSTANCE_NAME}, commit ${SOURCE_COMMIT:0:12}, verrou flock tenu"

# ---- SOURCES DE GENERATION (registre strict, handoff, marques) : toutes les
# sources presentes doivent porter la meme generation et la cible exacte.
snap_state=""; snap_gen=""; snap_rc=0
snap_out="$(python3 - "${STATE_FILE}" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}" <<'PY'
import re, sys
path, project, zone, instance = sys.argv[1:5]
ALLOWED = {"start_may_have_been_requested", "targeted_running", "targeted_stopping",
           "targeted_stopped", "targeted_stop_failed"}
try:
    with open(path, "rb") as fh:
        raw = fh.read()
except FileNotFoundError:
    sys.exit(3)
except OSError:
    sys.exit(4)
if not raw.endswith(b"\n"):
    sys.exit(4)
fields = {}
for ln in raw.decode("utf-8", "replace").splitlines():
    if "=" not in ln:
        sys.exit(4)
    k, v = ln.split("=", 1)
    if k in fields or not re.match(r"^[A-Za-z0-9._:-]*$", v):
        sys.exit(4)
    fields[k] = v
if set(fields) != {"schema", "state", "project", "zone", "instance", "generation"}:
    sys.exit(4)
if fields["schema"] != "e-hgp.lifecycle-state.v1" or fields["state"] not in ALLOWED:
    sys.exit(4)
# Contre-audit, point 2 : tout etat qui implique une cible demarree porte
# une generation NON VIDE (comme le parseur du cycle de vie).
if fields["state"] != "start_may_have_been_requested" and not fields["generation"]:
    sys.exit(4)
if (fields["project"], fields["zone"], fields["instance"]) != (project, zone, instance):
    sys.exit(5)
print(fields["state"], fields["generation"])
PY
)" || snap_rc=$?
case "${snap_rc}" in
  0) read -r snap_state snap_gen <<< "${snap_out}" ;;
  3) snap_state="absent" ;;
  5) echo "REFUS : registre d'une AUTRE cible que session.env — blocage" >&2; exit 71 ;;
  *) echo "BLOCAGE : registre illisible ou incoherent (rc=${snap_rc} : etat sans generation, schema, keyset) — controle manuel : gcloud compute instances describe ${GCP_INSTANCE_NAME} --project=${GCP_PROJECT_ID} --zone=${GCP_ZONE} --format='value(status,lastStartTimestamp)'" >&2; exit 71 ;;
esac
hand_gen=""
if [ -f "${HANDOFF}" ]; then
  hand_gen="$(python3 - "${HANDOFF}" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}" <<'PY'
import json, sys
try:
    d = json.load(open(sys.argv[1]))
except Exception:
    sys.exit(4)
if d.get("schema") != "e-hgp.start-handoff.v3" or (d.get("project"), d.get("zone"), d.get("instance")) != tuple(sys.argv[2:5]):
    sys.exit(4)
print(d.get("last_start_timestamp", ""))
PY
)" || { echo "BLOCAGE : handoff illisible ou d'une autre cible" >&2; exit 71; }
fi
# Marques : objets STRICTS (contre-audit) — fichier regulier, keyset exact,
# cles uniques, mark=<nom du fichier>, generation non vide, cible exacte.
mark_gen() { # $1 = nom ; vide si absente ; BLOCAGE 71 si presente mais invalide
  [ -e "${MARKS_DIR}/$1" ] || return 0
  [ -f "${MARKS_DIR}/$1" ] && [ ! -L "${MARKS_DIR}/$1" ] || { echo "BLOCAGE : marque $1 n'est pas un fichier regulier" >&2; exit 71; }
  python3 - "${MARKS_DIR}/$1" "$1" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}" <<'PY' || { echo "BLOCAGE : marque $1 hors schema, keyset, nom ou cible" >&2; exit 71; }
import re, sys
path, name, project, zone, instance = sys.argv[1:6]
raw = open(path, "rb").read()
if not raw.endswith(b"\n"):
    sys.exit(1)
fields = {}
for ln in raw.decode("utf-8", "replace").splitlines():
    if "=" not in ln:
        sys.exit(1)
    k, v = ln.split("=", 1)
    if k in fields or not re.match(r"^[A-Za-z0-9._:-]*$", v):
        sys.exit(1)
    fields[k] = v
if set(fields) != {"schema", "mark", "project", "zone", "instance", "generation",
                   "max_run_seconds", "guest_shutdown_minutes", "date_utc"}:
    sys.exit(1)
if fields["schema"] != "e-hgp.guard-mark.v1" or fields["mark"] != name or not fields["generation"]:
    sys.exit(1)
if (fields["project"], fields["zone"], fields["instance"]) != (project, zone, instance):
    sys.exit(1)
print(fields["generation"])
PY
}
m1_gen="$(mark_gen guest_guard_pending)"; m2_gen="$(mark_gen double_guard_verified)"
GENERATION=""
for g in "${snap_gen}" "${hand_gen}" "${m1_gen}" "${m2_gen}"; do
  [ -n "${g}" ] || continue
  if [ -z "${GENERATION}" ]; then GENERATION="${g}"; elif [ "${GENERATION}" != "${g}" ]; then
    echo "BLOCAGE : generations discordantes entre registre/handoff/marques (${snap_gen:-∅} ${hand_gen:-∅} ${m1_gen:-∅} ${m2_gen:-∅}) — aucun appel" >&2; exit 71
  fi
done
rlog "registre=${snap_state} generation=${GENERATION:-inconnue} marques=$(ls "${MARKS_DIR}" 2>/dev/null | tr '\n' ' ')"

# ---- RECU (patron du cycle de vie, champs de reprise en plus). Jamais une
# decision. Purge des credentials VERIFIEE puis temoin recu_publie, seulement
# sur un registre targeted_stopped ; temoin MINIMAL (aucune copie recursive)
# tant que l'arret n'est pas certifie.
GEN_EPOCH=""
if [ -n "${GENERATION}" ]; then
  GEN_EPOCH="$(python3 - "${GENERATION}" <<'PY'
from datetime import datetime
import sys
try:
    s = sys.argv[1].replace("Z", "+00:00")
    print(int(datetime.fromisoformat(s).timestamp()))
except Exception:
    print("")
PY
)"
fi
purge_credentials() { # -> 0 si les credentials sont absents apres purge (idempotent, aucun appel GCP)
  rm -rf "${WORK}/gcloud-config" 2>/dev/null || true
  if [ -f "${WORK}/ssh/id_ed25519" ]; then
    shred -u "${WORK}/ssh/id_ed25519" 2>/dev/null || rm -f "${WORK}/ssh/id_ed25519" 2>/dev/null || true
  fi
  [ ! -e "${WORK}/gcloud-config" ] && [ ! -e "${WORK}/ssh/id_ed25519" ]
}
publish_witness() { # $1 = chemin du recu ; temoin atomique + fsync du parent
  python3 - "${WORK}/recu_publie" "$1" <<'PY'
import os, sys, tempfile
path, content = sys.argv[1:3]
d = os.path.dirname(path)
fd, tmp = tempfile.mkstemp(prefix=".recu_publie.", suffix=".partial", dir=d)
os.write(fd, (content + "\n").encode()); os.fsync(fd); os.close(fd); os.replace(tmp, path)
dfd = os.open(d, os.O_RDONLY); os.fsync(dfd); os.close(dfd)
PY
}
PURGE_RC=0
WITNESS_RC=0
finalize_receipt() { # $1 = issue, $2 = stop_rc, $3 = rc, $4 = classification, $5 = scp_rc, $6 = validate_rc, $7 = minimal(0|1)
  # epoch + pid : deux reprises dans la meme seconde ne collisionnent pas
  local run_id="${DURABLE_RECEIPT_PREFIX}_${GEN_EPOCH:-avorte}_reprise_$(date +%s)_$$"
  local dir="${DURABLE_RECEIPT_BASE}/${run_id}" tmp residus final_state minimal="${7:-0}"
  final_state="$(sed -n 's/^state=//p' "${STATE_FILE}" 2>/dev/null | head -n 1)"
  residus="$(ls -d "${DURABLE_RECEIPT_BASE}/${DURABLE_RECEIPT_PREFIX}_"*.partial.* 2>/dev/null | tr '\n' ' ')"
  {
    [ ! -e "${dir}" ] &&
    mkdir -p "${DURABLE_RECEIPT_BASE}" &&
    tmp="$(mktemp -d "${dir}.partial.XXXXXX")" &&
    {
      printf 'schema=e-hgp.v6-session-receipt.v3\nrun_id=%s\nissue=%s\nrc=%s\nstop_rc=%s\n' "${run_id}" "$1" "$3" "${2:-na}"
      printf 'reprise=1\nclassification=%s\ndecision=aucune\nscp_rc=%s\nvalidate_rc=%s\n' "$4" "${5:-na}" "${6:-na}"
      printf 'temoin_minimal=%s\nstop_first=%s\nverrou=flock\n' "${minimal}" "${STOP_FIRST:-0}"
      printf 'profil=%s profil_canonique=%s\n' "${EFFECTIVE_PROFILE:-inconnu}" "${CAMPAIGN_PROFILE:-inconnu}"
      printf 'generation=%s\nprojet=%s zone=%s instance=%s\n' "${GENERATION:-inconnue}" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}"
      printf 'source_commit=%s\nsource_payload_sha256=%s\nprotocol_manifest_sha256=%s\n' "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}"
      printf 'max_run_seconds=%s guest_shutdown_minutes=%s\n' "${MAX_RUN_SECONDS}" "${GUEST_SHUTDOWN_MINUTES}"
      printf 'etat_cycle_vie=%s\nmarques=%s\nresidus_recus=%s\n' "${final_state:-absent}" "$(ls "${MARKS_DIR}" 2>/dev/null | tr '\n' ' ')" "${residus}"
      printf 'superviseur_pid=%s\ndate_utc=%s\n' "${sup_pid:-inconnu}" "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    } > "${tmp}/RECU_SESSION.txt" &&
    { if [ "${minimal}" = 1 ]; then
        # § 5.21 : temoin MINIMAL BORNE — tails plafonnes (64 Kio), aucune
        # copie recursive (ni out/ ni marques/), aucun sync ; le signalement
        # (code 70/71/78) ne doit pas attendre un archivage.
        { [ ! -f "${WORK}/session.log" ] || tail -c 65536 "${WORK}/session.log" > "${tmp}/session.tail.log"; } &&
        tail -c 65536 "${RLOG}" > "${tmp}/reprise.tail.log"
      else
        { [ ! -f "${WORK}/session.log" ] || cp -f "${WORK}/session.log" "${tmp}/session.log"; } &&
        cp -f "${RLOG}" "${tmp}/reprise.log" &&
        { [ ! -d "${MARKS_DIR}" ] || cp -r "${MARKS_DIR}" "${tmp}/marques"; }
      fi; } &&
    { [ "${minimal}" = 1 ] || [ ! -f "${WORK}/profil_campagne.txt" ] || cp -f "${WORK}/profil_campagne.txt" "${tmp}/"; } &&
    { [ "${minimal}" = 1 ] || [ ! -f "${WORK}/validation.txt" ] || cp -f "${WORK}/validation.txt" "${tmp}/"; } &&
    { [ "${minimal}" = 1 ] || [ ! -f "${WORK}/manifest_revalide.txt" ] || cp -f "${WORK}/manifest_revalide.txt" "${tmp}/"; } &&
    { [ "${minimal}" = 1 ] || [ ! -d "${WORK}/out" ] || cp -r "${WORK}/out" "${tmp}/out"; } &&
    ( cd "${tmp}" && { find . -type f ! -name 'SHA256SUMS*' -print0 | sort -z | xargs -0 sha256sum; } > SHA256SUMS.tmp \
      && mv SHA256SUMS.tmp SHA256SUMS && sha256sum -c --quiet SHA256SUMS >/dev/null ) &&
    mv -Tn "${tmp}" "${dir}" && [ ! -e "${tmp}" ] && { [ "${minimal}" = 1 ] || sync; }
  } 2>/dev/null && rlog "recu de reprise publie : ${dir}" || return 1
  # Contre-audit, point 6 : PURGE VERIFIEE d'abord, TEMOIN ensuite — jamais
  # un temoin terminal au-dessus de credentials encore presents.
  if [ "${final_state}" = "targeted_stopped" ]; then
    if purge_credentials; then
      if publish_witness "${dir}"; then
        rlog "credentials purges, temoin recu_publie publie"
      else
        WITNESS_RC=68
        rlog "TEMOIN NON PUBLIE : purge faite mais recu_publie non ecrit (mkstemp/rename/fsync en echec) — rejouer la reprise (aucun appel GCP)"
      fi
    else
      PURGE_RC=67
      rlog "PURGE INCOMPLETE : credentials encore presents dans ${WORK} — aucun temoin ; relancer la reprise (purge locale, aucun appel GCP)"
    fi
  fi
  return 0
}
publish_state() { # $1 = etat (ecriture atomique, meme schema que le garde)
  python3 - "${STATE_FILE}" "$1" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}" "${GENERATION}" <<'PY'
import os, sys, tempfile
path, state, project, zone, instance, generation = sys.argv[1:7]
data = f"schema=e-hgp.lifecycle-state.v1\nstate={state}\nproject={project}\nzone={zone}\ninstance={instance}\ngeneration={generation}\n".encode()
fd, tmp = tempfile.mkstemp(prefix=".etat.", suffix=".partial", dir=os.path.dirname(path))
os.write(fd, data); os.fsync(fd); os.close(fd); os.replace(tmp, path)
PY
}
describe_ro() { # lecture seule, bornee ; tuple "STATUS<TAB>GENERATION" ou describe_indisponible
  /usr/bin/timeout -k "${GRACE_S}" "${DESCRIBE_TIMEOUT_S}" gcloud compute instances describe "${GCP_INSTANCE_NAME}" \
    --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --format='value(status,lastStartTimestamp)' 2>/dev/null || echo "describe_indisponible"
}
tuple_status() { printf '%s' "${1%%$'\t'*}"; }
tuple_gen() { case "$1" in *$'\t'*) printf '%s' "${1#*$'\t'}" ;; *) printf '' ;; esac; }

# ---- CAS 1 : aucune generation => jamais un arret aveugle (BLOCAGE 71).
if [ -z "${GENERATION}" ]; then
  obs="$(describe_ro)"
  rlog "BLOCAGE : generation inconnue (registre=${snap_state}) ; describe : ${obs}"
  echo "commande d'arret A LANCER A LA MAIN apres controle : GCP_PROJECT_ID=${GCP_PROJECT_ID} GCP_ZONE=${GCP_ZONE} GCP_INSTANCE_NAME=${GCP_INSTANCE_NAME} CLOUDSDK_CONFIG=${CLOUDSDK_CONFIG} ${STOP_GUARD} --yes --expected-last-start-timestamp <generation observee>" | tee -a "${RLOG}"
  issue="reprise_sans_generation"; [ "${snap_state}" = "absent" ] && [ -z "${m1_gen}" ] && issue="reprise_sans_start"
  finalize_receipt "${issue}" na 71 aucune na na 1 || true
  exit 71
fi
# REMOTE_DIR lie a (SOURCE_COMMIT, epoque de la generation) — contre-audit, point 3.
if [ -n "${REMOTE_DIR}" ] && [ "${REMOTE_DIR}" != "v6camp_${SOURCE_COMMIT:0:12}_${GEN_EPOCH}" ]; then
  rlog "BLOCAGE : REMOTE_DIR (${REMOTE_DIR}) != v6camp_${SOURCE_COMMIT:0:12}_${GEN_EPOCH} (commit, generation)"
  finalize_receipt reprise_remote_dir_discordant na 71 aucune na na 1 || true
  exit 71
fi

# ---- CAS 2 : registre deja targeted_stopped sur cette generation => aucun
# appel GCP ; purge (re-jouable) puis temoin.
if [ "${snap_state}" = "targeted_stopped" ]; then
  rlog "arret deja certifie au registre pour ${GENERATION} — aucun appel"
  finalize_receipt reprise_deja_certifiee 0 0 aucune na na 1 || { echo "RECU NON PUBLIE" >&2; exit 66; }
  [ "${PURGE_RC}" -eq 0 ] || exit "${PURGE_RC}"
  exit "${WITNESS_RC}"
fi

# ---- STOP-FIRST (contre-audit, point 4) : apres un arret incertain, rien
# ne precede la garde d'arret — ni describe, ni scp, ni validateur.
STOP_FIRST=0
case "${snap_state}" in targeted_stop_failed|targeted_stopping) STOP_FIRST=1 ;; esac
[ "${STOP_FIRST}" -eq 0 ] || rlog "stop-first : registre ${snap_state}, aucun appel avant l'arret cible"

# ---- CAS 3 : scp bornee SEULEMENT avec double_guard_verified, hors
# stop-first, tuple describe EXACT (RUNNING, generation), cle non expiree et
# fenetre de rapatriement ouverte ; staging puis relecture du tuple.
SCP_RC=na; VALIDATE_RC=na; classification=aucune
STAGING="${WORK}/out.staging"
if [ -n "${m2_gen}" ] && [ "${STOP_FIRST}" -eq 0 ]; then
  obs="$(describe_ro)"; rlog "describe : ${obs}"
  obs_status="$(tuple_status "${obs}")"; obs_gen="$(tuple_gen "${obs}")"
  if [ "${obs}" != "describe_indisponible" ] && [ -n "${obs_gen}" ] && [ "${obs_gen}" != "${GENERATION}" ]; then
    rlog "BLOCAGE : la cible porte une AUTRE generation (${obs_gen}) que la session (${GENERATION}) — aucune scp, aucun arret de cette generation"
    finalize_receipt reprise_generation_concurrente na 71 aucune na na 1 || true
    exit 71
  fi
  now="$(date +%s)"
  key_ok=0
  if [ -n "${GCP_SSH_KEY_EXPIRATION_UTC}" ]; then
    exp_epoch="$(python3 -c "from datetime import datetime; import sys; print(int(datetime.fromisoformat(sys.argv[1].replace('Z','+00:00')).timestamp()))" "${GCP_SSH_KEY_EXPIRATION_UTC}" 2>/dev/null || echo 0)"
    [ "${now}" -lt "$((exp_epoch - 60))" ] && key_ok=1
  fi
  fenetre_ok=0
  if [ -n "${GEN_EPOCH}" ]; then
    scp_worst=$(( SCP_STEP_TIMEOUT_S + GRACE_S + 2 * (DESCRIBE_TIMEOUT_S + GRACE_S) + STOP_RESERVE_S ))
    [ $(( now + scp_worst )) -lt $(( GEN_EPOCH + EFFECTIVE_CUTOFF_S )) ] && fenetre_ok=1
  fi
  if [ "${obs_status}" = "RUNNING" ] && [ "${obs_gen}" = "${GENERATION}" ] && [ "${key_ok}" -eq 1 ] && [ "${fenetre_ok}" -eq 1 ] \
     && [ -n "${REMOTE_DIR}" ] && [ -f "${GCP_SSH_KEY_FILE}" ]; then
    rlog "rapatriement borne de ~/${REMOTE_DIR}/out vers un staging (${SCP_STEP_TIMEOUT_S} s)"
    rm -rf "${STAGING}"; mkdir -p "${STAGING}"
    # Code du scp lu sur le PIPELINE lui-meme (set +e : jamais un `|| true`
    # qui masquerait l'echec derriere le code de `true`).
    set +e
    /usr/bin/timeout -k "${GRACE_S}" "${SCP_STEP_TIMEOUT_S}" gcloud compute scp --recurse \
      --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
      --ssh-key-file="${GCP_SSH_KEY_FILE}" --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" \
      "${GCP_INSTANCE_NAME}:~/${REMOTE_DIR}/out" "${STAGING}/" 2>&1 | tee -a "${RLOG}"
    SCP_RC="${PIPESTATUS[0]}"
    set -e
    rlog "scp_rc=${SCP_RC}"
    # Relecture du tuple APRES la scp : promotion seulement si la generation
    # est inchangee ; sinon le staging est detruit et rien n'est promu.
    obs2="$(describe_ro)"; rlog "describe apres scp : ${obs2}"
    obs2_status="$(tuple_status "${obs2}")"; obs2_gen="$(tuple_gen "${obs2}")"
    if [ "${obs2}" != "describe_indisponible" ] && [ -n "${obs2_gen}" ] && [ "${obs2_gen}" != "${GENERATION}" ]; then
      rm -rf "${STAGING}"
      rlog "BLOCAGE : generation changee pendant le rapatriement (${obs2_gen}) — staging detruit, rien promu, aucun arret"
      finalize_receipt reprise_generation_concurrente na 71 aucune "${SCP_RC}" na 1 || true
      exit 71
    fi
    # § 5.21 : l'espace canonique out/ n'est remplace que si la scp a REUSSI
    # et si le tuple posterieur est lisible, exact et de la generation
    # attendue ; sinon le partiel est conserve sous un nom explicite et seul
    # l'arret cible de la generation connue se poursuit.
    if [ "${SCP_RC}" -eq 0 ] && [ "${obs2}" != "describe_indisponible" ] && [ "${obs2_status}" = "RUNNING" ] \
       && [ "${obs2_gen}" = "${GENERATION}" ] && [ -d "${STAGING}/out" ]; then
      rm -rf "${WORK}/out"; mv -T "${STAGING}/out" "${WORK}/out"
      rlog "staging promu en out/ (scp reussie, tuple posterieur exact)"
    elif [ -d "${STAGING}/out" ] && [ -n "$(ls -A "${STAGING}/out" 2>/dev/null)" ]; then
      partiel="${WORK}/out.partiel_$(date +%s)"
      mv -T "${STAGING}" "${partiel}"
      rlog "staging NON promu (scp_rc=${SCP_RC}, describe apres scp : ${obs2}) — conserve sous ${partiel}"
    fi
    rm -rf "${STAGING}"
  else
    SCP_RC=77; rlog "rapatriement saute (status=${obs_status}, generation_observee=${obs_gen:-∅}, cle_ok=${key_ok}, fenetre_ok=${fenetre_ok})"
  fi
fi

# ---- CAS 4 : ARRET CIBLE par la garde EPINGLEE (une tentative par rejeu).
publish_state targeted_stopping
rlog "arret cible : ${STOP_GUARD} --yes --expected-last-start-timestamp ${GENERATION}"
# Code de la garde d'arret lu sur le PIPELINE (set +e) : un arret en echec
# ne doit JAMAIS etre publie targeted_stopped.
set +e
"${STOP_GUARD}" --yes --expected-last-start-timestamp "${GENERATION}" 2>&1 | tee -a "${RLOG}"
STOP_RC="${PIPESTATUS[0]}"
set -e
if [ "${STOP_RC}" -eq 0 ]; then publish_state targeted_stopped; else publish_state targeted_stop_failed; fi
rlog "stop_rc=${STOP_RC}"

# ---- CAS 5 : validateur epingle sur les sorties rapatriees, SEULEMENT apres
# un arret certifie (classification FORCEE partiel_ou_invalide : le code
# distant est inconnu par construction).
if [ -n "${m2_gen}" ]; then
  classification=partiel_ou_invalide
  # Validateur SEULEMENT sur un out/ promu et non vide (un out/ vide cree par
  # le cycle de vie n'est pas un rapatriement).
  if [ "${STOP_RC}" -eq 0 ] && [ -n "$(ls -A "${WORK}/out" 2>/dev/null)" ] && [ -f "${WORK}/profil_campagne.txt" ]; then
    canon="${WORK}/pinned/gcp-migration/profils/${CAMPAIGN_PROFILE}.env"
    VALIDATE_RC=0
    V6_RESUMES_DIR="${WORK}" /usr/bin/timeout -k "${GRACE_S}" "${VALIDATOR_TIMEOUT_S}" python3 "${PINNED_DIR}/validate_v6_campaign.py" \
      "${WORK}/out" "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}" 75 "${SCP_RC}" \
      "${WORK}/profil_campagne.txt" "${canon}" "${WORK}/manifest_revalide.txt" > "${WORK}/validation.txt" 2>&1 || VALIDATE_RC=$?
    rlog "validateur : rc=${VALIDATE_RC} (classification forcee ${classification})"
  elif [ "${STOP_RC}" -ne 0 ]; then
    VALIDATE_RC=na; rlog "arret non certifie : aucun validateur, aucune copie (temoin minimal)"
  else
    VALIDATE_RC=na; rlog "aucune sortie rapatriee : classification ${classification}"
  fi
  issue=reprise_apres_perte_superviseur
else
  issue=reprise_sans_double_garde
fi

# ---- Controle du tuple post-arret, recu, code.
if [ "${STOP_RC}" -eq 0 ]; then
  [ "$(sed -n 's/^state=//p' "${STATE_FILE}")" = "targeted_stopped" ] && [ "$(sed -n 's/^generation=//p' "${STATE_FILE}")" = "${GENERATION}" ] \
    || { rlog "INCOHERENCE : registre post-arret != targeted_stopped ${GENERATION}"; finalize_receipt "${issue}" "${STOP_RC}" 78 "${classification}" "${SCP_RC}" "${VALIDATE_RC}" 1 || true; exit 78; }
fi
rc=0; minimal=0; [ "${STOP_RC}" -eq 0 ] || { rc=70; minimal=1; }
finalize_receipt "${issue}" "${STOP_RC}" "${rc}" "${classification}" "${SCP_RC}" "${VALIDATE_RC}" "${minimal}" || { echo "RECU NON PUBLIE" >&2; exit 66; }
if [ "${rc}" -ne 0 ]; then
  echo "ARRET NON CERTIFIE (stop_rc=${STOP_RC}) — rejeu MANUEL de la reprise (stop-first) ou arret a la main : ${STOP_GUARD} --yes --expected-last-start-timestamp ${GENERATION}" >&2
  exit "${rc}"
fi
[ "${PURGE_RC}" -eq 0 ] || exit "${PURGE_RC}"
exit "${WITNESS_RC}"
