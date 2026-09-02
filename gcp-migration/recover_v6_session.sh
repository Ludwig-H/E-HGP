#!/usr/bin/env bash
# REPRISE APRES PERTE DU SUPERVISEUR (audit serie C § 5.18.6). Deux etages,
# comme le bootstrap : l'etage 1 (seul code non epingle execute) lit
# session.env, se re-authentifie contre `git show <commit>` (lui-meme ET la
# copie epinglee), puis `exec` la copie epinglee en etage 2. L'etage 2 :
#   - ne demarre JAMAIS la VM (aucun set_max, aucun start) ;
#   - refuse tant qu'un processus de la session vit (pid + starttime +
#     boot_id, et tout processus referencant WORK), ou si la session est
#     deja conclue (recu_publie), ou si la cible/generation est ambigue ;
#   - registre `targeted_stopped` de la generation => deja certifie, aucun
#     appel ;
#   - sans `double_guard_verified` => stop_and_verify immediat sur la
#     generation exacte, aucune scp (issue=reprise_sans_double_garde) ;
#   - avec => describe, scp bornee des sorties distantes si la VM tourne
#     encore et la cle n'est pas expiree, stop_and_verify, validateur
#     epingle borne, classification FORCEE partiel_ou_invalide
#     (issue=reprise_apres_perte_superviseur) ;
#   - generation inconnue => BLOCAGE 71 : describe en lecture seule affiche,
#     commande d'arret a copier, jamais un arret aveugle ;
#   - recu durable `<prefix>_<gen_epoch>_reprise_<epoch>`, jamais une
#     decision ; temoin recu_publie et purge des credentials seulement si
#     l'arret est certifie au registre.
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
GUARDS_DIR="$(env_field GUARDS_DIR)"   # gardes de la session (epinglees en production, factices au harnais)
# Chaque fichier epingle egale sa version du commit (jamais un manifeste
# auto-reference recalcule sur le WORK lui-meme) ; la garde d'arret n'est
# verifiee contre le commit que si les gardes sont les copies epinglees.
for f in v6_session_lifecycle.sh validate_v6_campaign.py recover_v6_session.sh; do
  [ "$(sha256sum "${PINNED_DIR}/${f}" | awk '{print $1}')" = "$(git -C "${REPO_ROOT}" show "${SOURCE_COMMIT}:gcp-migration/${f}" | sha256sum | awk '{print $1}')" ] \
    || { echo "REFUS : ${f} epingle differe du commit ${SOURCE_COMMIT}" >&2; exit 2; }
done
if [ "${GUARDS_DIR}" = "${PINNED_DIR}" ]; then
  [ "$(sha256sum "${GUARDS_DIR}/stop_and_verify.sh" | awk '{print $1}')" = "$(git -C "${REPO_ROOT}" show "${SOURCE_COMMIT}:gcp-migration/stop_and_verify.sh" | sha256sum | awk '{print $1}')" ] \
    || { echo "REFUS : stop_and_verify.sh epingle differe du commit ${SOURCE_COMMIT}" >&2; exit 2; }
else
  echo "gardes non epinglees (${GUARDS_DIR}) : harnais de selftest seulement"
fi
[ -x "${GUARDS_DIR}/stop_and_verify.sh" ] || { echo "REFUS : stop_and_verify.sh absent de ${GUARDS_DIR}" >&2; exit 2; }

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
VALIDATOR_TIMEOUT_S="$(env_field VALIDATOR_TIMEOUT_S)"
CAMPAIGN_PROFILE="$(env_field CAMPAIGN_PROFILE)"
EFFECTIVE_PROFILE="$(env_field EFFECTIVE_PROFILE)"
GCP_SSH_KEY_EXPIRATION_UTC="$(env_field GCP_SSH_KEY_EXPIRATION_UTC)"
REMOTE_DIR="$(env_field REMOTE_DIR)"
for v in MAX_RUN_SECONDS EFFECTIVE_CUTOFF_S GRACE_S SCP_STEP_TIMEOUT_S DESCRIBE_TIMEOUT_S VALIDATOR_TIMEOUT_S; do
  [[ "${!v}" =~ ^[1-9][0-9]*$ ]] || { echo "REFUS : ${v} non entier dans session.env" >&2; exit 2; }
done
export CLOUDSDK_CONFIG="${WORK}/gcloud-config"
export GCP_SSH_KEY_FILE="${WORK}/ssh/id_ed25519"
export GCP_SSH_KEY_EXPIRATION_UTC
STATE_FILE="${WORK}/etat_cycle_vie"; HANDOFF="${WORK}/handoff.json"; MARKS_DIR="${WORK}/marques"
PID_FILE="${WORK}/superviseur.pid"; LOCK="${WORK}/reprise.pid"; RLOG="${WORK}/reprise.log"
rlog() { printf '[%s] %s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$*" | tee -a "${RLOG}"; }

[ ! -e "${WORK}/recu_publie" ] || { echo "REFUS : session deja conclue (recu $(cat "${WORK}/recu_publie"))" >&2; exit 2; }

# ---- VIVACITE : le superviseur (pid + starttime + boot_id) et TOUT
# processus referencant WORK doivent etre morts.
proc_alive() { # $1 = pid, $2 = starttime, $3 = boot_id  -> 0 si vivant et identique
  local pid="$1" st="$2" bid="$3" cur
  [ "$(cat /proc/sys/kernel/random/boot_id)" = "${bid}" ] || return 1
  [ -r "/proc/${pid}/stat" ] || return 1
  cur="$(sed 's/.*) //' "/proc/${pid}/stat" | awk '{print $20}')"
  [ "${cur}" = "${st}" ]
}
if [ -f "${PID_FILE}" ]; then
  read -r sup_pid sup_st sup_bid < "${PID_FILE}" || true
  if [[ "${sup_pid:-}" =~ ^[0-9]+$ ]] && proc_alive "${sup_pid}" "${sup_st:-x}" "${sup_bid:-x}"; then
    echo "REFUS : superviseur vivant (pid ${sup_pid}) — aucune reprise pendant qu'il court" >&2; exit 2
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
  # candidat deja mort (sous-shell transitoire de cette substitution) : ignore
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
# Verrou de reprise avec vivacite (un verrou perime est remplace).
if [ -f "${LOCK}" ]; then
  read -r l_pid l_st l_bid < "${LOCK}" || true
  if [[ "${l_pid:-}" =~ ^[0-9]+$ ]] && proc_alive "${l_pid}" "${l_st:-x}" "${l_bid:-x}"; then
    echo "REFUS : une reprise est deja en cours (pid ${l_pid})" >&2; exit 2
  fi
  rm -f "${LOCK}"
fi
printf '%s %s %s\n' "$$" "$(sed 's/.*) //' /proc/$$/stat | awk '{print $20}')" "$(cat /proc/sys/kernel/random/boot_id)" > "${LOCK}"
rlog "reprise ${WORK} : cible ${GCP_PROJECT_ID}/${GCP_ZONE}/${GCP_INSTANCE_NAME}, commit ${SOURCE_COMMIT:0:12}"

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
if (fields["project"], fields["zone"], fields["instance"]) != (project, zone, instance):
    sys.exit(5)
print(fields["state"], fields["generation"])
PY
)" || snap_rc=$?
case "${snap_rc}" in
  0) read -r snap_state snap_gen <<< "${snap_out}" ;;
  3) snap_state="absent" ;;
  5) echo "REFUS : registre d'une AUTRE cible que session.env — blocage" >&2; exit 71 ;;
  *) echo "BLOCAGE : registre illisible (rc=${snap_rc}) — controle manuel : gcloud compute instances describe ${GCP_INSTANCE_NAME} --project=${GCP_PROJECT_ID} --zone=${GCP_ZONE} --format='value(status,lastStartTimestamp)'" >&2; exit 71 ;;
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
mark_gen() { # $1 = nom ; vide si absente
  [ -f "${MARKS_DIR}/$1" ] || return 0
  grep -qx 'schema=e-hgp.guard-mark.v1' "${MARKS_DIR}/$1" || { echo "BLOCAGE : marque $1 hors schema" >&2; exit 71; }
  [ "$(sed -n 's/^project=//p' "${MARKS_DIR}/$1")" = "${GCP_PROJECT_ID}" ] && [ "$(sed -n 's/^zone=//p' "${MARKS_DIR}/$1")" = "${GCP_ZONE}" ] \
    && [ "$(sed -n 's/^instance=//p' "${MARKS_DIR}/$1")" = "${GCP_INSTANCE_NAME}" ] || { echo "BLOCAGE : marque $1 d'une autre cible" >&2; exit 71; }
  sed -n 's/^generation=//p' "${MARKS_DIR}/$1"
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
# decision. Le temoin recu_publie et la purge des credentials n'ont lieu
# que sur un registre targeted_stopped.
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
finalize_receipt() { # $1 = issue, $2 = stop_rc, $3 = rc, $4 = classification, $5 = scp_rc, $6 = validate_rc
  # epoch + pid : deux reprises dans la meme seconde ne collisionnent pas
  local run_id="${DURABLE_RECEIPT_PREFIX}_${GEN_EPOCH:-avorte}_reprise_$(date +%s)_$$"
  local dir="${DURABLE_RECEIPT_BASE}/${run_id}" tmp residus final_state
  final_state="$(sed -n 's/^state=//p' "${STATE_FILE}" 2>/dev/null | head -n 1)"
  residus="$(ls -d "${DURABLE_RECEIPT_BASE}/${DURABLE_RECEIPT_PREFIX}_"*.partial.* 2>/dev/null | tr '\n' ' ')"
  {
    [ ! -e "${dir}" ] &&
    mkdir -p "${DURABLE_RECEIPT_BASE}" &&
    tmp="$(mktemp -d "${dir}.partial.XXXXXX")" &&
    {
      printf 'schema=e-hgp.v6-session-receipt.v3\nrun_id=%s\nissue=%s\nrc=%s\nstop_rc=%s\n' "${run_id}" "$1" "$3" "${2:-na}"
      printf 'reprise=1\nclassification=%s\ndecision=aucune\nscp_rc=%s\nvalidate_rc=%s\n' "$4" "${5:-na}" "${6:-na}"
      printf 'profil=%s profil_canonique=%s\n' "${EFFECTIVE_PROFILE:-inconnu}" "${CAMPAIGN_PROFILE:-inconnu}"
      printf 'generation=%s\nprojet=%s zone=%s instance=%s\n' "${GENERATION:-inconnue}" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}"
      printf 'source_commit=%s\nsource_payload_sha256=%s\nprotocol_manifest_sha256=%s\n' "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}"
      printf 'max_run_seconds=%s guest_shutdown_minutes=%s\n' "${MAX_RUN_SECONDS}" "${GUEST_SHUTDOWN_MINUTES}"
      printf 'etat_cycle_vie=%s\nmarques=%s\nresidus_recus=%s\n' "${final_state:-absent}" "$(ls "${MARKS_DIR}" 2>/dev/null | tr '\n' ' ')" "${residus}"
      printf 'superviseur_pid=%s\ndate_utc=%s\n' "${sup_pid:-inconnu}" "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    } > "${tmp}/RECU_SESSION.txt" &&
    { [ ! -f "${WORK}/session.log" ] || cp -f "${WORK}/session.log" "${tmp}/session.log"; } &&
    cp -f "${RLOG}" "${tmp}/reprise.log" &&
    { [ ! -d "${MARKS_DIR}" ] || cp -r "${MARKS_DIR}" "${tmp}/marques"; } &&
    { [ ! -f "${WORK}/profil_campagne.txt" ] || cp -f "${WORK}/profil_campagne.txt" "${tmp}/"; } &&
    { [ ! -f "${WORK}/validation.txt" ] || cp -f "${WORK}/validation.txt" "${tmp}/"; } &&
    { [ ! -f "${WORK}/manifest_revalide.txt" ] || cp -f "${WORK}/manifest_revalide.txt" "${tmp}/"; } &&
    { [ ! -d "${WORK}/out" ] || cp -r "${WORK}/out" "${tmp}/out"; } &&
    ( cd "${tmp}" && { find . -type f ! -name 'SHA256SUMS*' -print0 | sort -z | xargs -0 sha256sum; } > SHA256SUMS.tmp \
      && mv SHA256SUMS.tmp SHA256SUMS && sha256sum -c --quiet SHA256SUMS >/dev/null ) &&
    mv -Tn "${tmp}" "${dir}" && [ ! -e "${tmp}" ] && sync &&
    { if [ "${final_state}" = "targeted_stopped" ]; then
        printf '%s\n' "${dir}" > "${WORK}/recu_publie"
        rm -rf "${WORK}/gcloud-config"
        [ ! -f "${WORK}/ssh/id_ed25519" ] || shred -u "${WORK}/ssh/id_ed25519" 2>/dev/null || rm -f "${WORK}/ssh/id_ed25519"
      fi; true; }
  } 2>/dev/null && rlog "recu de reprise publie : ${dir}"
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
describe_ro() { # lecture seule, bornee
  /usr/bin/timeout -k "${GRACE_S}" "${DESCRIBE_TIMEOUT_S}" gcloud compute instances describe "${GCP_INSTANCE_NAME}" \
    --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --format='value(status,lastStartTimestamp)' 2>/dev/null || echo "describe_indisponible"
}

# ---- CAS 1 : aucune generation => jamais un arret aveugle (BLOCAGE 71).
if [ -z "${GENERATION}" ]; then
  obs="$(describe_ro)"
  rlog "BLOCAGE : generation inconnue (registre=${snap_state}) ; describe : ${obs}"
  echo "commande d'arret A LANCER A LA MAIN apres controle : GCP_PROJECT_ID=${GCP_PROJECT_ID} GCP_ZONE=${GCP_ZONE} GCP_INSTANCE_NAME=${GCP_INSTANCE_NAME} CLOUDSDK_CONFIG=${CLOUDSDK_CONFIG} ${GUARDS_DIR}/stop_and_verify.sh --yes --expected-last-start-timestamp <generation observee>" | tee -a "${RLOG}"
  issue="reprise_sans_generation"; [ "${snap_state}" = "absent" ] && [ -z "${m1_gen}" ] && issue="reprise_sans_start"
  finalize_receipt "${issue}" na 71 aucune na na || true
  exit 71
fi

# ---- CAS 2 : registre deja targeted_stopped sur cette generation => rien.
if [ "${snap_state}" = "targeted_stopped" ]; then
  rlog "arret deja certifie au registre pour ${GENERATION} — aucun appel"
  finalize_receipt reprise_deja_certifiee 0 0 aucune na na || { echo "RECU NON PUBLIE" >&2; exit 66; }
  exit 0
fi

# ---- CAS 3 : scp bornee SEULEMENT avec double_guard_verified, VM RUNNING,
# cle non expiree et fenetre de rapatriement ouverte.
SCP_RC=na; VALIDATE_RC=na; classification=aucune
if [ -n "${m2_gen}" ]; then
  obs="$(describe_ro)"; rlog "describe : ${obs}"
  now="$(date +%s)"
  key_ok=0
  if [ -n "${GCP_SSH_KEY_EXPIRATION_UTC}" ]; then
    exp_epoch="$(python3 -c "from datetime import datetime; import sys; print(int(datetime.fromisoformat(sys.argv[1].replace('Z','+00:00')).timestamp()))" "${GCP_SSH_KEY_EXPIRATION_UTC}" 2>/dev/null || echo 0)"
    [ "${now}" -lt "$((exp_epoch - 60))" ] && key_ok=1
  fi
  fenetre_ok=0
  if [ -n "${GEN_EPOCH}" ]; then
    scp_worst=$(( SCP_STEP_TIMEOUT_S + GRACE_S + 2 * (DESCRIBE_TIMEOUT_S + GRACE_S) + $(env_field STOP_RESERVE_S) ))
    [ $(( now + scp_worst )) -lt $(( GEN_EPOCH + EFFECTIVE_CUTOFF_S )) ] && fenetre_ok=1
  fi
  if [[ "${obs}" == RUNNING* ]] && [ "${key_ok}" -eq 1 ] && [ "${fenetre_ok}" -eq 1 ] && [ -n "${REMOTE_DIR}" ] && [ -f "${GCP_SSH_KEY_FILE}" ]; then
    rlog "rapatriement borne de ~/${REMOTE_DIR}/out (${SCP_STEP_TIMEOUT_S} s)"
    mkdir -p "${WORK}/out"
    # Code du scp lu sur le PIPELINE lui-meme (set +e : jamais un `|| true`
    # qui masquerait l'echec derriere le code de `true`).
    set +e
    /usr/bin/timeout -k "${GRACE_S}" "${SCP_STEP_TIMEOUT_S}" gcloud compute scp --recurse \
      --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
      --ssh-key-file="${GCP_SSH_KEY_FILE}" --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" \
      "${GCP_INSTANCE_NAME}:~/${REMOTE_DIR}/out" "${WORK}/" 2>&1 | tee -a "${RLOG}"
    SCP_RC="${PIPESTATUS[0]}"
    set -e
    rlog "scp_rc=${SCP_RC}"
  else
    SCP_RC=77; rlog "rapatriement saute (status=${obs%%	*}, cle_ok=${key_ok}, fenetre_ok=${fenetre_ok})"
  fi
fi

# ---- CAS 4 : ARRET CIBLE (une tentative ; une seconde seulement si le
# registre porte deja targeted_stop_failed, jamais plus).
allowance=1; [ "${snap_state}" = "targeted_stop_failed" ] && allowance=1
publish_state targeted_stopping
# Code de la garde d'arret lu sur le PIPELINE (set +e) : un arret en echec
# ne doit JAMAIS etre publie targeted_stopped.
set +e
"${GUARDS_DIR}/stop_and_verify.sh" --yes --expected-last-start-timestamp "${GENERATION}" 2>&1 | tee -a "${RLOG}"
STOP_RC="${PIPESTATUS[0]}"
set -e
if [ "${STOP_RC}" -eq 0 ]; then publish_state targeted_stopped; else publish_state targeted_stop_failed; fi
rlog "stop_rc=${STOP_RC}"

# ---- CAS 5 : validateur epingle sur les sorties rapatriees (classification
# FORCEE partiel_ou_invalide : le code distant est inconnu par construction).
if [ -n "${m2_gen}" ]; then
  classification=partiel_ou_invalide
  if [ -d "${WORK}/out" ] && [ -f "${WORK}/profil_campagne.txt" ]; then
    canon="${WORK}/pinned/gcp-migration/profils/${CAMPAIGN_PROFILE}.env"
    VALIDATE_RC=0
    V6_RESUMES_DIR="${WORK}" /usr/bin/timeout -k "${GRACE_S}" "${VALIDATOR_TIMEOUT_S}" python3 "${PINNED_DIR}/validate_v6_campaign.py" \
      "${WORK}/out" "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}" 75 "${SCP_RC}" \
      "${WORK}/profil_campagne.txt" "${canon}" "${WORK}/manifest_revalide.txt" > "${WORK}/validation.txt" 2>&1 || VALIDATE_RC=$?
    rlog "validateur : rc=${VALIDATE_RC} (classification forcee ${classification})"
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
    || { rlog "INCOHERENCE : registre post-arret != targeted_stopped ${GENERATION}"; finalize_receipt "${issue}" "${STOP_RC}" 78 "${classification}" "${SCP_RC}" "${VALIDATE_RC}" || true; exit 78; }
fi
rc=0; [ "${STOP_RC}" -eq 0 ] || rc=70
finalize_receipt "${issue}" "${STOP_RC}" "${rc}" "${classification}" "${SCP_RC}" "${VALIDATE_RC}" || { echo "RECU NON PUBLIE" >&2; exit 66; }
rm -f "${LOCK}"
if [ "${rc}" -ne 0 ]; then
  echo "ARRET NON CERTIFIE (stop_rc=${STOP_RC}) — relancer la reprise ou arreter a la main : ${GUARDS_DIR}/stop_and_verify.sh --yes --expected-last-start-timestamp ${GENERATION}" >&2
fi
exit "${rc}"
