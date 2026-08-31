#!/usr/bin/env bash
# CYCLE DE VIE de la session G4 v6 — execute EXCLUSIVEMENT comme copie
# materialisee depuis le COMMIT par le bootstrap session_campagne_v6_g4.sh
# (P0-2 de l'audit GCP v6), ou par le selftest de cycle de vie
# (selftest_cycle_vie_v6.sh, fausses gardes + faux gcloud). Il n'execute que
# les gardes du repertoire MHGP6_LIFECYCLE_GUARDS_DIR (copies epinglees en
# production), y compris dans le trap.
#
# GARANTIES P0-1 (audit GCP v6 du 31 aout) :
#  - le trap d'arret est installe AVANT toute mutation GCP ;
#  - la garde de demarrage ecrit un TEMOIN DE MUTATION durable et atomique
#    (--mutation-witness-file) au point exact ou le start GCE commence ;
#  - cleanup commence par neutraliser sa recursion et `set +e`, preserve le
#    code initial, et AUCUNE ecriture de journal ne peut empecher l'arret ;
#  - table de decision : temoin absent => refus avant mutation, propager sans
#    arret ni blocage ; temoin present + generation lisible (handoff relu et
#    valide si besoin) => UNE tentative d'arret ciblee ; temoin present +
#    generation illisible => BLOCAGE explicite (projet, zone, instance,
#    commande de controle), jamais un arret non cible ni un silence.
#
# P1 : echeance derivee du lastStartTimestamp certifie ; preflight budgetaire
# declare ; profil de campagne epingle AVANT la campagne et transmis au
# validateur ; journaux ctest complets avec planchers ; handshake de
# generation et de boot_id autour des SSH ; chemins distants propres a la
# (pin, generation) ; SSH/SCP bornes par timeout ; recu durable apres arret.
set -euo pipefail

WORK="${MHGP6_LIFECYCLE_WORK:?MHGP6_LIFECYCLE_WORK requis}"
GUARDS_DIR="${MHGP6_LIFECYCLE_GUARDS_DIR:?MHGP6_LIFECYCLE_GUARDS_DIR requis}"
SOURCE_COMMIT="${MHGP6_LIFECYCLE_SOURCE_COMMIT:?commit requis}"
SOURCE_PAYLOAD_SHA256="${MHGP6_LIFECYCLE_PAYLOAD_SHA256:?sha du payload requis}"
PROTOCOL_MANIFEST_SHA256="${MHGP6_LIFECYCLE_MANIFEST_SHA256:?manifeste requis}"
for g in set_max_run_duration_and_verify.sh start_and_verify.sh stop_and_verify.sh; do
  [ -x "${GUARDS_DIR}/${g}" ] || { echo "REFUS : garde epinglee absente (${GUARDS_DIR}/${g})" >&2; exit 2; }
done
RUNNER="${WORK}/pinned/gcp-migration/v6_campaign_remote.sh"
VALIDATOR="${WORK}/pinned/gcp-migration/validate_v6_campaign.py"
BUNDLE="${WORK}/bundle.tgz"
for f in "${RUNNER}" "${VALIDATOR}" "${BUNDLE}"; do
  [ -e "${f}" ] || { echo "REFUS : artefact epingle absent (${f})" >&2; exit 2; }
done
echo "${SOURCE_PAYLOAD_SHA256}  ${BUNDLE}" | sha256sum -c - >/dev/null \
  || { echo "REFUS : bundle epingle au mauvais sha256" >&2; exit 2; }

# REVALIDATION DU MANIFESTE CANONIQUE avant toute execution (audit GCP v6,
# deuxieme tour) : le manifeste est RECONSTRUIT depuis les copies
# materialisees (schema + commit + une ligne sha256/taille/chemin par
# fichier, ordre normatif identique au pin) et son SHA-256 doit EGALER le
# digest transmis par la chaine authentifiee du bootstrap.
PROTOCOL_FILES=(
  gcp-migration/session_campagne_v6_g4.sh
  gcp-migration/v6_session_lifecycle.sh
  gcp-migration/v6_campaign_pin.sh
  gcp-migration/v6_campaign_remote.sh
  gcp-migration/validate_v6_campaign.py
  gcp-migration/profils/decision_v1.env
  gcp-migration/profils/smoke_v1.env
  gcp-migration/set_max_run_duration_and_verify.sh
  gcp-migration/start_and_verify.sh
  gcp-migration/stop_and_verify.sh
)
{
  echo "schema=e-hgp.protocol-manifest.v1"
  echo "commit=${SOURCE_COMMIT}"
  for f in "${PROTOCOL_FILES[@]}"; do
    [ -f "${WORK}/pinned/${f}" ] || { echo "REFUS : copie epinglee absente (${f})" >&2; exit 2; }
    printf '%s\t%s\t%s\n' \
      "$(sha256sum "${WORK}/pinned/${f}" | awk '{print $1}')" \
      "$(wc -c < "${WORK}/pinned/${f}")" \
      "${f}"
  done
} > "${WORK}/manifest_revalide.txt"
REVALIDATED="$(sha256sum "${WORK}/manifest_revalide.txt" | awk '{print $1}')"
if [ "${REVALIDATED}" != "${PROTOCOL_MANIFEST_SHA256}" ]; then
  echo "REFUS : manifeste revalide (${REVALIDATED}) != digest du protocole (${PROTOCOL_MANIFEST_SHA256})" >&2
  exit 2
fi

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
# 8 h (maximum autorise par AGENTS.md) : la matrice de decision par defaut
# tient dans cette fenetre d'apres le preflight budgetaire ci-dessous.
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-28800}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-470}"
RAPATRIEMENT_MARGE_S="${RAPATRIEMENT_MARGE_S:-1500}"
SSH_STEP_TIMEOUT_S="${SSH_STEP_TIMEOUT_S:-3600}"
SCP_STEP_TIMEOUT_S="${SCP_STEP_TIMEOUT_S:-1800}"

# PROFIL CANONIQUE (audit deuxieme tour : le fichier d'autorite est
# versionne et hashe, jamais fabrique par l'environnement). CAMPAIGN_PROFILE
# nomme un profil epingle (decision_v1 par defaut, smoke_v1 pour la fumee) ;
# TOUTE surcharge d'un axe par l'environnement degrade le profil effectif en
# `custom` — un validateur ne peut alors jamais l'appeler decision.
CAMPAIGN_PROFILE="${CAMPAIGN_PROFILE:-decision_v1}"
[[ "${CAMPAIGN_PROFILE}" =~ ^[a-z0-9_]+$ ]] || { echo "REFUS : nom de profil mal forme" >&2; exit 2; }
PROFILE_SRC="${WORK}/pinned/gcp-migration/profils/${CAMPAIGN_PROFILE}.env"
[ -f "${PROFILE_SRC}" ] || { echo "REFUS : profil canonique inconnu (${CAMPAIGN_PROFILE})" >&2; exit 2; }
_ov_CONF_SPECS="${CONF_SPECS:-}"; _ov_BENCH_SPECS="${BENCH_SPECS:-}"
_ov_QUEUE_FAMILIES="${QUEUE_FAMILIES:-}"; _ov_QUEUE_N="${QUEUE_N:-}"
_ov_QUEUE_SEEDS="${QUEUE_SEEDS:-}"; _ov_RUN_TIMEOUT="${RUN_TIMEOUT:-}"
_ov_THREADS_VM="${THREADS_VM:-}"; _ov_V5_GATE_MIN="${V5_GATE_MIN:-}"; _ov_V6_GATE_MIN="${V6_GATE_MIN:-}"
# shellcheck disable=SC1090
source "${PROFILE_SRC}"
EFFECTIVE_PROFILE="${CAMPAIGN_PROFILE}"
for v in CONF_SPECS BENCH_SPECS QUEUE_FAMILIES QUEUE_N QUEUE_SEEDS RUN_TIMEOUT THREADS_VM V5_GATE_MIN V6_GATE_MIN; do
  ov="_ov_${v}"
  if [ -n "${!ov}" ] && [ "${!ov}" != "${!v}" ]; then
    EFFECTIVE_PROFILE="custom"
    printf -v "${v}" '%s' "${!ov}"
  fi
done
_param_re='^[A-Za-z0-9_: -]*$'
for v in CONF_SPECS BENCH_SPECS QUEUE_FAMILIES QUEUE_N QUEUE_SEEDS RUN_TIMEOUT THREADS_VM; do
  [[ "${!v}" =~ ${_param_re} ]] || { echo "REFUS : parametre ${v} avec caractere hors alphabet sur" >&2; exit 2; }
done
echo "profil canonique : ${CAMPAIGN_PROFILE} ($(sha256sum "${PROFILE_SRC}" | awk '{print $1}')) — profil effectif : ${EFFECTIVE_PROFILE}"

HANDOFF="${WORK}/handoff.json"
STATE_FILE="${WORK}/etat_cycle_vie"
LOG="${WORK}/session.log"
PROFILE="${WORK}/profil_campagne.txt"
: > "${LOG}"
DURABLE_RECEIPT_BASE="${DURABLE_RECEIPT_BASE:?DURABLE_RECEIPT_BASE requis (recu durable obligatoire)}"
DURABLE_RECEIPT_PREFIX="${DURABLE_RECEIPT_PREFIX:?DURABLE_RECEIPT_PREFIX requis}"

# Lecture de l'enregistrement de cycle de vie partage avec le garde.
state_field() { # $1 = cle ; vide si fichier absent
  [ -s "${STATE_FILE}" ] || return 0
  sed -n "s/^$1=//p" "${STATE_FILE}" 2>/dev/null | head -1
}
# PUBLICATION de transition par le cleanup EXTERIEUR (audit troisieme tour :
# le registre doit decrire aussi l'arret nominal) — meme schema et meme
# patron atomique que le garde ; best-effort (jamais bloquant pour l'arret).
lifecycle_publish_state() { # $1 = etat
  python3 - "${STATE_FILE}" "$1" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}" \
    "${GENERATION}" 2>/dev/null <<'PY' || true
import os
import sys
import tempfile
from pathlib import Path

path = Path(sys.argv[1])
state, project, zone, instance, generation = sys.argv[2:7]
if not path.is_absolute() or path.is_symlink():
    sys.exit(1)
data = ("schema=e-hgp.lifecycle-state.v1\n"
        f"state={state}\n"
        f"project={project}\n"
        f"zone={zone}\n"
        f"instance={instance}\n"
        f"generation={generation}\n").encode()
descriptor, temporary = tempfile.mkstemp(prefix=f".{path.name}.", suffix=".partial", dir=str(path.parent))
try:
    offset = 0
    while offset < len(data):
        offset += os.write(descriptor, data[offset:])
    os.fsync(descriptor)
finally:
    os.close(descriptor)
os.replace(temporary, path)
parent = os.open(str(path.parent), os.O_DIRECTORY)
try:
    os.fsync(parent)
finally:
    os.close(parent)
PY
}

log() { printf '%s\n' "$*" | tee -a "${LOG}"; }
log_safe() { { printf '%s\n' "$*" >> "${LOG}"; } 2>/dev/null || true; }

# Lecture STRICTE du handoff (schema, cible exacte, generation non vide).
parse_handoff() {
  python3 - "${HANDOFF}" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}" <<'PY'
import json, sys
try:
    with open(sys.argv[1], encoding="utf-8") as fh:
        record = json.load(fh)
except Exception:
    sys.exit(1)
if record.get("schema") != "e-hgp.start-handoff.v3":
    sys.exit(1)
if (record.get("project"), record.get("zone"), record.get("instance")) != tuple(sys.argv[2:5]):
    sys.exit(1)
generation = record.get("last_start_timestamp", "")
if not generation or not isinstance(generation, str) or any(c in generation for c in " '\"\n"):
    sys.exit(1)
print(generation)
PY
}

# ---- ARRET CERTIFIE : trap installe AVANT toute mutation GCP (P0-1).
GENERATION=""
SESSION_RC=0
STOP_ATTEMPTED=0
finalize_receipt() { # $1 = issue, $2 = stop_rc, $3 = rc ; rend 0 ssi le recu COMPLET est publie
  # RUN UNIQUE incluant la generation (audit troisieme tour) : construction
  # dans un temporaire, publication ATOMIQUE, dossier preexistant REFUSE.
  local run_id="${DURABLE_RECEIPT_PREFIX}_${GEN_EPOCH:-avorte_$(date +%s)}"
  local dir="${DURABLE_RECEIPT_BASE}/${run_id}"
  local tmp="${dir}.partial"
  {
    [ ! -e "${dir}" ] && [ ! -e "${tmp}" ] &&
    mkdir -p "${tmp}" &&
    {
      printf 'schema=e-hgp.v6-session-receipt.v3\nrun_id=%s\nissue=%s\nrc=%s\nstop_rc=%s\n' \
        "${run_id}" "$1" "$3" "${2:-na}"
      printf 'profil=%s profil_canonique=%s\n' "${EFFECTIVE_PROFILE:-inconnu}" "${CAMPAIGN_PROFILE:-inconnu}"
      printf 'generation=%s\nprojet=%s zone=%s instance=%s\n' "${GENERATION:-inconnue}" \
        "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}"
      printf 'source_commit=%s\nsource_payload_sha256=%s\nprotocol_manifest_sha256=%s\n' \
        "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}"
      printf 'max_run_seconds=%s guest_shutdown_minutes=%s\n' "${MAX_RUN_SECONDS}" "${GUEST_SHUTDOWN_MINUTES}"
      printf 'etat_cycle_vie=%s\n' "$(state_field state)"
      printf 'date_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    } > "${tmp}/RECU_SESSION.txt" &&
    cp -f "${LOG}" "${tmp}/session.log" &&
    { [ ! -f "${PROFILE}" ] || cp -f "${PROFILE}" "${tmp}/profil_campagne.txt"; } &&
    { [ ! -f "${WORK}/validation.txt" ] || cp -f "${WORK}/validation.txt" "${tmp}/validation.txt"; } &&
    { [ ! -f "${WORK}/manifest_revalide.txt" ] || cp -f "${WORK}/manifest_revalide.txt" "${tmp}/"; } &&
    { [ ! -d "${WORK}/out" ] || cp -r "${WORK}/out" "${tmp}/out"; } &&
    ( cd "${tmp}" && { find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum; } > SHA256SUMS.tmp \
      && mv SHA256SUMS.tmp SHA256SUMS ) &&
    mv "${tmp}" "${dir}"
  } 2>/dev/null
}
cleanup() {
  local rc=$?
  trap - EXIT HUP INT TERM
  set +e
  if [ "${SESSION_RC}" -ne 0 ]; then rc="${SESSION_RC}"; fi
  log_safe "--- arret certifie (rc=${rc}) ---"
  local lc_state
  lc_state="$(state_field state)"
  local lc_gen
  lc_gen="$(state_field generation)"
  if [ -z "${GENERATION}" ] && [ -n "${lc_gen}" ]; then GENERATION="${lc_gen}"; fi
  if [ -z "${GENERATION}" ] && [ -s "${HANDOFF}" ]; then
    GENERATION="$(parse_handoff 2>/dev/null)" || GENERATION=""
  fi
  local receipt_rc=0
  if [ -z "${lc_state}" ]; then
    # Aucun start GCE atteste : refus avant mutation — ni arret ni blocage.
    log_safe "aucun enregistrement de cycle de vie : refus avant demarrage, aucun arret a certifier"
    finalize_receipt refus_avant_mutation "" "${rc}" || receipt_rc=66
    if [ "${receipt_rc}" -ne 0 ]; then
      echo "[RECU NON PUBLIE] le recu durable n'a pas pu etre ecrit (${DURABLE_RECEIPT_DIR})" >&2
      exit 66
    fi
    exit "${rc}"
  fi
  # TERMINAL PARTAGE : le garde a deja certifie l'arret cible de cette
  # generation — ni second arret, ni faux blocage (audit deuxieme tour).
  if [ "${lc_state}" = "targeted_stopped" ] && [ -n "${GENERATION}" ] \
     && [ "$(state_field project)" = "${GCP_PROJECT_ID}" ] \
     && [ "$(state_field zone)" = "${GCP_ZONE}" ] \
     && [ "$(state_field instance)" = "${GCP_INSTANCE_NAME}" ]; then
    log_safe "arret deja certifie par le garde (generation ${GENERATION}) : aucun second arret"
    echo "arret deja certifie par le garde (generation ${GENERATION})"
    finalize_receipt arret_certifie_par_le_garde 0 "${rc}" || {
      echo "[RECU NON PUBLIE] le recu durable n'a pas pu etre ecrit (${DURABLE_RECEIPT_DIR})" >&2
      exit 66
    }
    exit "${rc}"
  fi
  if [ -z "${GENERATION}" ]; then
    {
      echo "[BLOCAGE] mutation de demarrage ATTESTEE (etat ${lc_state}) mais generation illisible — passage de relais requis."
      echo "[BLOCAGE] projet=${GCP_PROJECT_ID} zone=${GCP_ZONE} instance=${GCP_INSTANCE_NAME}"
      echo "[BLOCAGE] dernier etat certifie avant mutation : TERMINATED ; enregistrement : ${STATE_FILE}"
      echo "[BLOCAGE] controle : gcloud compute instances describe ${GCP_INSTANCE_NAME} --project=${GCP_PROJECT_ID} --zone=${GCP_ZONE} --format='value(status,lastStartTimestamp)'"
    } >&2
    finalize_receipt blocage_generation_illisible "" 71 || true
    exit 71
  fi
  # REPRISE BORNEE : une tentative d'arret ciblee par le cleanup exterieur —
  # deliberement RE-tentee si l'arret interne du garde a echoue (etat
  # targeted_stop_failed) ; jamais une unicite globale de l'appel d'arret.
  # Le registre est mis a jour par CE cleanup aussi (audit troisieme tour :
  # l'arret nominal doit y figurer) : targeted_stopping avant, puis
  # targeted_stopped / targeted_stop_failed selon le resultat.
  local so="${LOG}"
  ( : >> "${so}" ) 2>/dev/null || so="$(mktemp 2>/dev/null || echo /dev/stderr)"
  STOP_ATTEMPTED=1
  lifecycle_publish_state "targeted_stopping"
  local stop_rc=0
  "${GUARDS_DIR}/stop_and_verify.sh" --yes \
    --expected-last-start-timestamp "${GENERATION}" >> "${so}" 2>&1 || stop_rc=$?
  if [ "${stop_rc}" -eq 0 ]; then
    lifecycle_publish_state "targeted_stopped"
  else
    lifecycle_publish_state "targeted_stop_failed"
  fi
  log_safe "stop_and_verify (generation ${GENERATION}) : rc=${stop_rc}"
  echo "journal complet : ${LOG}"
  echo "resultats rapatries : ${WORK}/out (si l'etape scp a ete atteinte)"
  if [ "${stop_rc}" -eq 0 ] && [ "$(state_field state)" != "targeted_stopped" ]; then
    echo "[INCOHERENCE] arret certifie mais registre != targeted_stopped" >&2
    receipt_rc=66
  fi
  finalize_receipt arret_tente "${stop_rc}" "${rc}" || receipt_rc=66
  if [ "${stop_rc}" -ne 0 ]; then
    echo "[ARRET NON CERTIFIE] stop_and_verify a rendu ${stop_rc} — echec bloquant" >&2
    exit 70
  fi
  if [ "${receipt_rc}" -ne 0 ]; then
    echo "[RECU NON PUBLIE] arret certifie mais recu durable manquant (${DURABLE_RECEIPT_DIR})" >&2
    exit 66
  fi
  exit "${rc}"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

# CONFIGURATION GCLOUD PRIVEE a la session (audit deuxieme tour : aucune
# mutation de la configuration partagee) — copie de la configuration
# existante (credentials compris) dans le WORK, puis toute commande gcloud
# de cette session n'ecrit que dans cette copie ; --project/--zone restent
# explicites sur chaque commande.
# TOUJOURS prive (audit troisieme tour) : le repertoire est cree dans tous
# les cas ; la configuration source (CLOUDSDK_CONFIG herite, sinon la
# configuration par defaut) y est copiee si elle existe — credentials
# compris — puis seule la copie est exportee.
_gcloud_src="${CLOUDSDK_CONFIG:-${HOME}/.config/gcloud}"
mkdir -p "${WORK}/gcloud-config"
if [ -d "${_gcloud_src}" ]; then cp -r "${_gcloud_src}/." "${WORK}/gcloud-config/"; fi
export CLOUDSDK_CONFIG="${WORK}/gcloud-config"
gcloud config set project "${GCP_PROJECT_ID}" >/dev/null

# ---- PROFIL DE CAMPAGNE EPINGLE AVANT TOUTE MUTATION (P1 : la matrice est
# fixee independamment des sorties que le validateur jugera ; le nom
# effectif degrade en `custom` des qu'un axe canonique est surcharge).
{
  echo "profil=${EFFECTIVE_PROFILE}"
  echo "profil_canonique=${CAMPAIGN_PROFILE}"
  echo "profil_canonique_sha256=$(sha256sum "${PROFILE_SRC}" | awk '{print $1}')"
  echo "conf_specs=${CONF_SPECS}"
  echo "bench_specs=${BENCH_SPECS}"
  echo "queue_families=${QUEUE_FAMILIES}"
  echo "queue_n=${QUEUE_N}"
  echo "queue_seeds=${QUEUE_SEEDS}"
  echo "run_timeout=${RUN_TIMEOUT}"
  echo "threads=${THREADS_VM}"
  echo "v5_gate_min=${V5_GATE_MIN}"
  echo "v6_gate_min=${V6_GATE_MIN}"
} > "${PROFILE}.tmp"
mv "${PROFILE}.tmp" "${PROFILE}"
log "profil de campagne epingle : $(sha256sum "${PROFILE}" | awk '{print $1}')"

# ---- PREFLIGHT BUDGETAIRE (P1) : estimations CONSERVATRICES declarees ici,
# jamais ajustees apres mesure. Refus AVANT toute mutation si l'estimation
# depasse la fenetre utile.
budget_estimate() {
  python3 - "${CONF_SPECS}" "${BENCH_SPECS}" "${QUEUE_FAMILIES}" "${QUEUE_N}" "${QUEUE_SEEDS}" <<'PY'
import sys
conf = sys.argv[1].split()
bench = sys.argv[2].split()
queue_f, queue_n, queue_s = sys.argv[3].split(), sys.argv[4].split(), sys.argv[5].split()
# secondes par run, VM 48 fils, DECLARE AVANT MESURE (recus v5 G4 a 200k
# ~260s x marge 1,5-2 ; jamais ajuste apres coup) :
conf_ref = {8000: 30, 32000: 60, 50000: 100, 100000: 200, 200000: 450}
bench_ref = {8000: 40, 32000: 80, 100000: 220, 200000: 480}
queue_ref = {16000: 60, 64000: 150, 128000: 350, 256000: 800}
total = 0
for spec in conf:
    n = int(spec.split(":")[1])
    total += 2 * conf_ref.get(n, 900)  # reference v5 + juge v6
for spec in bench:
    n = int(spec.split(":")[1])
    total += 4 * bench_ref.get(n, 900)  # ABBA : quatre runs par paire
for n in queue_n:
    total += len(queue_f) * len(queue_s) * queue_ref.get(int(n), 1800)
print(total)
PY
}
BUILD_ESTIMATE_S=900
ESTIMATE_S="$(budget_estimate)"
WINDOW_S=$((MAX_RUN_SECONDS - RAPATRIEMENT_MARGE_S - BUILD_ESTIMATE_S))
log "preflight budgetaire : estimation ${ESTIMATE_S}s (+build ${BUILD_ESTIMATE_S}s) pour une fenetre de ${WINDOW_S}s"
if [ "${ESTIMATE_S}" -gt "${WINDOW_S}" ]; then
  echo "REFUS : la matrice declaree (${ESTIMATE_S}s estimes) ne tient pas dans la fenetre (${WINDOW_S}s) — reduire la matrice ou augmenter MAX_RUN_SECONDS" >&2
  exit 2
fi

# ---- 1. Borne de duree persistee sur l'instance ARRETEE (garde epinglee).
"${GUARDS_DIR}/set_max_run_duration_and_verify.sh" --yes \
  --max-run-duration-seconds "${MAX_RUN_SECONDS}" 2>&1 | tee -a "${LOG}"

# ---- 2. Cle OS Login ephemere, TTL borne par la duree de la VM.
export GCP_SSH_KEY_DIR="${WORK}/ssh"
mkdir -p "${GCP_SSH_KEY_DIR}"
chmod 700 "${GCP_SSH_KEY_DIR}"
export GCP_SSH_KEY_FILE="${GCP_SSH_KEY_DIR}/id_ed25519"
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-v6-session' -f "${GCP_SSH_KEY_FILE}"
chmod 600 "${GCP_SSH_KEY_FILE}"
# TTL OS Login DANS la fenetre acceptee par le preflight du garde (audit
# deuxieme tour : le garde refuse une duree restante > MAX_RUN_SECONDS +
# 660 s ; +600 s laisse 60 s de marge). Garde arithmetique locale : un TTL
# hors fenetre est refuse ICI, avant toute mutation.
SSH_TTL_MIN=$(((MAX_RUN_SECONDS + 600) / 60))
if [ $((SSH_TTL_MIN * 60)) -gt $((MAX_RUN_SECONDS + 660)) ]; then
  echo "REFUS : TTL OS Login (${SSH_TTL_MIN} min) hors de la fenetre du preflight (MAX_RUN_SECONDS + 660 s)" >&2
  exit 2
fi
GCP_SSH_KEY_EXPIRATION_UTC="$(python3 -c "from datetime import datetime,timedelta,timezone; print((datetime.now(timezone.utc)+timedelta(minutes=${SSH_TTL_MIN})).isoformat(timespec='seconds').replace('+00:00','Z'))")"
export GCP_SSH_KEY_EXPIRATION_UTC
gcloud compute os-login ssh-keys add --key-file="${GCP_SSH_KEY_FILE}.pub" --ttl="${SSH_TTL_MIN}m" \
  --project="${GCP_PROJECT_ID}" >/dev/null

# ---- 3. Demarrage garde (epingle), temoin de mutation durable, handoff
# atomique. Le retour est CAPTURE : quel qu'il soit, cleanup decide via la
# table temoin/handoff.
START_RC=0
set +e
"${GUARDS_DIR}/start_and_verify.sh" --yes \
  --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}" \
  --handoff-file "${HANDOFF}" \
  --lifecycle-state-file "${STATE_FILE}" 2>&1 | tee -a "${LOG}"
START_RC=${PIPESTATUS[0]}
set -e
if [ "${START_RC}" -ne 0 ]; then
  SESSION_RC="${START_RC}"
  exit "${START_RC}"
fi
GENERATION="$(parse_handoff)" || { SESSION_RC=72; exit 72; }
log "generation verrouillee : ${GENERATION}"

# ---- Echeance derivee du lastStartTimestamp CERTIFIE (P1), jamais de
# l'horloge locale au moment du demarrage.
GEN_EPOCH="$(python3 - "${GENERATION}" <<'PY'
from datetime import datetime
import sys
value = sys.argv[1]
if value.endswith("Z"):
    value = value[:-1] + "+00:00"
print(int(datetime.fromisoformat(value).timestamp()))
PY
)" || { SESSION_RC=73; exit 73; }
DEADLINE_EPOCH=$((GEN_EPOCH + MAX_RUN_SECONDS - RAPATRIEMENT_MARGE_S))
log "deadline_epoch=${DEADLINE_EPOCH} (generation ${GEN_EPOCH} + ${MAX_RUN_SECONDS} - ${RAPATRIEMENT_MARGE_S})"

# Controle de generation NON MUTANT (describe) : ferme la fenetre entre le
# start certifie et le premier SSH.
check_generation() {
  local seen
  seen="$(gcloud compute instances describe "${GCP_INSTANCE_NAME}" --project="${GCP_PROJECT_ID}" \
          --zone="${GCP_ZONE}" --format='value(lastStartTimestamp)' 2>>"${LOG}")" || return 1
  [ "${seen}" = "${GENERATION}" ]
}
check_generation || { log "REFUS : generation changee avant SSH"; SESSION_RC=74; exit 74; }

SSH=(gcloud compute ssh "${GCP_INSTANCE_NAME}" --project="${GCP_PROJECT_ID}"
     --zone="${GCP_ZONE}" --ssh-key-file="${GCP_SSH_KEY_FILE}"
     --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" --quiet --command)
SCP=(gcloud compute scp --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}"
     --quiet --ssh-key-file="${GCP_SSH_KEY_FILE}"
     --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}")

# Chemins distants propres a (pin, generation) — jamais reutilises (P1).
REMOTE_TAG="${SOURCE_COMMIT:0:12}_${GEN_EPOCH}"
REMOTE_DIR="v6camp_${REMOTE_TAG}"
REMOTE_BUNDLE="/tmp/v6bundle_${REMOTE_TAG}.tgz"

# ---- 4a. HANDSHAKE NON MUTANT (audit deuxieme tour) : un premier SSH qui
# ne fait QUE lire boot_id, encadre par DEUX controles de generation ; toute
# commande distante ulterieure REVERIFIE ce boot_id avant de muter quoi que
# ce soit sur la VM.
BOOT_ID="$(timeout "${SCP_STEP_TIMEOUT_S}" "${SSH[@]}" 'cat /proc/sys/kernel/random/boot_id' 2>>"${LOG}")" \
  || { log "REFUS : handshake boot_id impossible"; SESSION_RC=75; exit 75; }
[[ "${BOOT_ID}" =~ ^[0-9a-f-]{36}$ ]] || { log "REFUS : boot_id mal forme (${BOOT_ID})"; SESSION_RC=75; exit 75; }
check_generation || { log "REFUS : generation changee pendant le handshake"; SESSION_RC=74; exit 74; }
log "handshake : boot_id=${BOOT_ID} generation confirmee"

# ---- 4b. Envoi du BUNDLE pinne (lecture seule cote VM), encadre par deux
# controles de generation.
timeout "${SCP_STEP_TIMEOUT_S}" "${SCP[@]}" "${BUNDLE}" \
  "${GCP_INSTANCE_NAME}:${REMOTE_BUNDLE}" 2>&1 | tee -a "${LOG}"
check_generation || { log "REFUS : generation changee apres l'envoi du bundle"; SESSION_RC=74; exit 74; }

# ---- 5. Build v5 + v6, preconditions, REJEU des portes avec JOURNAL COMPLET
# et planchers (P1 : jamais un tail -4). Le boot_id du handshake est
# REVERIFIE dans la meme commande distante avant toute mutation.
BUILD_LOG="${WORK}/build_vm.log"
timeout "${SSH_STEP_TIMEOUT_S}" "${SSH[@]}" 'set -euo pipefail
  test "$(cat /proc/sys/kernel/random/boot_id)" = '"'${BOOT_ID}'"' || { echo "REFUS : boot_id different (redemarrage detecte)" >&2; exit 9; }
  export PATH=$HOME/.local/bin:$PATH
  test -x /usr/bin/time || { echo "REFUS : GNU time absent de la VM" >&2; exit 2; }
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/'"${REMOTE_DIR}"' && mkdir -p ~/'"${REMOTE_DIR}"' && cd ~/'"${REMOTE_DIR}"'
  echo "'"${SOURCE_PAYLOAD_SHA256}"'  '"${REMOTE_BUNDLE}"'" | sha256sum -c -
  tar xzf '"${REMOTE_BUNDLE}"'
  echo "coeurs=$(nproc)"; grep MemTotal /proc/meminfo; cmake --version | head -1
  cmake -S morsehgp3D_v5 -B build-v5 -DCMAKE_BUILD_TYPE=Release
  cmake --build build-v5 -j48 2>&1 | tail -3
  ctest --test-dir build-v5 -L gate -j24 --output-on-failure
  cmake -S morsehgp3D_v6 -B build-v6 -DCMAKE_BUILD_TYPE=Release
  cmake --build build-v6 -j48 2>&1 | tail -3
  ctest --test-dir build-v6 -L gate -j24 --output-on-failure
' > "${BUILD_LOG}" 2>&1 || { SESSION_RC=$?; log "build/portes VM en echec (rc=${SESSION_RC}, journal ${BUILD_LOG})"; exit "${SESSION_RC}"; }
cat "${BUILD_LOG}" >> "${LOG}"
# Planchers de portes : DEUX blocs 100%, totaux >= planchers (P1).
mapfile -t GATE_TOTALS < <(grep -oE '100% tests passed, 0 tests failed out of [0-9]+' "${BUILD_LOG}" | grep -oE '[0-9]+$')
if [ "${#GATE_TOTALS[@]}" -ne 2 ] || [ "${GATE_TOTALS[0]}" -lt "${V5_GATE_MIN}" ] || [ "${GATE_TOTALS[1]}" -lt "${V6_GATE_MIN}" ]; then
  log "REFUS : rejeu des portes non conforme (blocs=${#GATE_TOTALS[@]} totaux=${GATE_TOTALS[*]:-aucun}, planchers ${V5_GATE_MIN}/${V6_GATE_MIN})"
  SESSION_RC=76
  exit 76
fi
log "portes VM : v5=${GATE_TOTALS[0]} v6=${GATE_TOTALS[1]} (journaux complets dans ${BUILD_LOG})"

# ---- 6. LA CAMPAGNE. Generation recontrolee, boot_id verifie DANS la meme
# commande distante avant toute execution ; retour CAPTURE sans trap.
check_generation || { log "REFUS : generation changee avant la campagne"; SESSION_RC=74; exit 74; }
REMOTE_CAMPAIGN_RC=0
CAMPAIGN_TIMEOUT=$(( DEADLINE_EPOCH - $(date +%s) + 300 ))
if [ "${CAMPAIGN_TIMEOUT}" -lt 60 ]; then CAMPAIGN_TIMEOUT=60; fi
set +e
timeout "${CAMPAIGN_TIMEOUT}" "${SSH[@]}" "set -euo pipefail
  test \"\$(cat /proc/sys/kernel/random/boot_id)\" = '${BOOT_ID}' || { echo 'REFUS : boot_id different (redemarrage detecte)' >&2; exit 9; }
  export PATH=\$HOME/.local/bin:\$PATH
  cd ~/${REMOTE_DIR}
  THREADS=${THREADS_VM} DEADLINE_EPOCH=${DEADLINE_EPOCH} RUN_TIMEOUT='${RUN_TIMEOUT}' \
    CONF_SPECS='${CONF_SPECS}' \
    BENCH_SPECS='${BENCH_SPECS}' \
    QUEUE_FAMILIES='${QUEUE_FAMILIES}' QUEUE_N='${QUEUE_N}' QUEUE_SEEDS='${QUEUE_SEEDS}' \
    bash gcp-migration/v6_campaign_remote.sh ${SOURCE_COMMIT} ${SOURCE_PAYLOAD_SHA256} ${PROTOCOL_MANIFEST_SHA256}
" 2>&1 | tee -a "${LOG}"
REMOTE_CAMPAIGN_RC=${PIPESTATUS[0]}
set -e
printf 'remote_campaign_rc=%d\n' "${REMOTE_CAMPAIGN_RC}" | tee -a "${LOG}"

# ---- 7. RAPATRIEMENT TOUJOURS, reprises bornees, generation controlee.
mkdir -p "${WORK}/out"
SCP_RC=1
for attempt in 1 2 3; do
  check_generation || { log "generation changee pendant le rapatriement (tentative ${attempt})"; break; }
  set +e
  timeout "${SCP_STEP_TIMEOUT_S}" "${SCP[@]}" --recurse \
    "${GCP_INSTANCE_NAME}:~/${REMOTE_DIR}/out" "${WORK}/" 2>&1 | tee -a "${LOG}"
  rc=${PIPESTATUS[0]}
  set -e
  if [ "${rc}" -eq 0 ]; then
    # Controle de generation APRES le rapatriement reussi (audit deuxieme
    # tour : chaque SCP encadre avant ET apres).
    if check_generation; then
      SCP_RC=0
    else
      log "generation changee APRES le rapatriement (tentative ${attempt}) : resultat non attribuable"
    fi
    break
  fi
  log "scp tentative ${attempt} echouee (rc=${rc})"
  sleep $((5 * attempt))
done
printf 'scp_rc=%d\n' "${SCP_RC}" | tee -a "${LOG}"

# ---- 8. VALIDATION LOCALE par le validateur EPINGLE, profil epingle joint :
# seule autorite du statut de campagne.
set +e
python3 "${VALIDATOR}" "${WORK}/out" \
  "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}" \
  "${REMOTE_CAMPAIGN_RC}" "${SCP_RC}" "${PROFILE}" "${PROFILE_SRC}" 2>&1 | tee "${WORK}/validation.txt" | tee -a "${LOG}"
VALIDATE_RC=${PIPESTATUS[0]}
set -e
if [ "${VALIDATE_RC}" -ne 0 ]; then SESSION_RC=65; fi

echo "session terminee ; l arret certifie est declenche par le trap"
