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
V5_GATE_MIN="${V5_GATE_MIN:-40}"
V6_GATE_MIN="${V6_GATE_MIN:-60}"
DURABLE_RECEIPT_DIR="${DURABLE_RECEIPT_DIR:-}"

# Parametres de campagne : valides AVANT toute interpolation SSH (P1 :
# aucune apostrophe ni caractere hors alphabet sur).
CONF_SPECS="${CONF_SPECS:-uniform:32000 terrain:32000 eight_clusters:32000 scanline_single_pass:32000 uniform:50000 terrain:50000 eight_clusters:50000 scanline_single_pass:50000 uniform:100000 eight_clusters:100000 uniform:200000 eight_clusters:200000}"
BENCH_FAMILIES="${BENCH_FAMILIES:-uniform terrain eight_clusters scanline_single_pass}"
BENCH_N="${BENCH_N:-32000 100000 200000}"
QUEUE_FAMILIES="${QUEUE_FAMILIES:-terrain_stationnaire scanline_stationnaire}"
QUEUE_N="${QUEUE_N:-64000 128000 256000}"
QUEUE_SEEDS="${QUEUE_SEEDS:-3 4 5}"
RUN_TIMEOUT="${RUN_TIMEOUT:-2400}"
THREADS_VM="${THREADS_VM:-48}"
_param_re='^[A-Za-z0-9_: -]*$'
for v in CONF_SPECS BENCH_FAMILIES BENCH_N QUEUE_FAMILIES QUEUE_N QUEUE_SEEDS RUN_TIMEOUT THREADS_VM; do
  [[ "${!v}" =~ ${_param_re} ]] || { echo "REFUS : parametre ${v} avec caractere hors alphabet sur" >&2; exit 2; }
done

HANDOFF="${WORK}/handoff.json"
WITNESS="${WORK}/temoin_mutation"
LOG="${WORK}/session.log"
PROFILE="${WORK}/profil_campagne.txt"
: > "${LOG}"

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
finalize_receipt() { # $1 = issue, $2 = stop_rc, $3 = rc — best effort, jamais bloquant
  [ -n "${DURABLE_RECEIPT_DIR}" ] || return 0
  {
    mkdir -p "${DURABLE_RECEIPT_DIR}"
    {
      printf 'schema=e-hgp.v6-session-receipt.v1\nissue=%s\nrc=%s\nstop_rc=%s\n' "$1" "$3" "${2:-na}"
      printf 'generation=%s\nprojet=%s zone=%s instance=%s\n' "${GENERATION:-inconnue}" \
        "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}"
      printf 'source_commit=%s\nsource_payload_sha256=%s\nprotocol_manifest_sha256=%s\n' \
        "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}"
      printf 'max_run_seconds=%s guest_shutdown_minutes=%s\n' "${MAX_RUN_SECONDS}" "${GUEST_SHUTDOWN_MINUTES}"
      printf 'date_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    } > "${DURABLE_RECEIPT_DIR}/RECU_SESSION.txt"
    cp -f "${LOG}" "${DURABLE_RECEIPT_DIR}/session.log" 2>/dev/null || true
    cp -f "${PROFILE}" "${DURABLE_RECEIPT_DIR}/profil_campagne.txt" 2>/dev/null || true
    ( cd "${DURABLE_RECEIPT_DIR}" && sha256sum RECU_SESSION.txt session.log 2>/dev/null > SHA256SUMS ) || true
  } 2>/dev/null || true
}
cleanup() {
  local rc=$?
  trap - EXIT HUP INT TERM
  set +e
  if [ "${SESSION_RC}" -ne 0 ]; then rc="${SESSION_RC}"; fi
  log_safe "--- arret certifie (rc=${rc}) ---"
  if [ -z "${GENERATION}" ] && [ -s "${HANDOFF}" ]; then
    GENERATION="$(parse_handoff 2>/dev/null)" || GENERATION=""
  fi
  if [ ! -e "${WITNESS}" ]; then
    # Aucun start GCE atteste : refus avant mutation — ni arret ni blocage.
    log_safe "aucun temoin de mutation : refus avant demarrage, aucun arret a certifier"
    finalize_receipt refus_avant_mutation "" "${rc}"
    exit "${rc}"
  fi
  if [ -z "${GENERATION}" ]; then
    {
      echo "[BLOCAGE] mutation de demarrage ATTESTEE mais generation illisible — passage de relais requis."
      echo "[BLOCAGE] projet=${GCP_PROJECT_ID} zone=${GCP_ZONE} instance=${GCP_INSTANCE_NAME}"
      echo "[BLOCAGE] dernier etat certifie avant mutation : TERMINATED ; temoin : ${WITNESS}"
      echo "[BLOCAGE] controle : gcloud compute instances describe ${GCP_INSTANCE_NAME} --project=${GCP_PROJECT_ID} --zone=${GCP_ZONE} --format='value(status,lastStartTimestamp)'"
    } >&2
    finalize_receipt blocage_generation_illisible "" 71
    exit 71
  fi
  # UNE tentative d'arret ciblee, insensible aux pannes de journal.
  local so="${LOG}"
  ( : >> "${so}" ) 2>/dev/null || so="$(mktemp 2>/dev/null || echo /dev/stderr)"
  STOP_ATTEMPTED=1
  local stop_rc=0
  "${GUARDS_DIR}/stop_and_verify.sh" --yes \
    --expected-last-start-timestamp "${GENERATION}" >> "${so}" 2>&1 || stop_rc=$?
  log_safe "stop_and_verify (generation ${GENERATION}) : rc=${stop_rc}"
  echo "journal complet : ${LOG}"
  echo "resultats rapatries : ${WORK}/out (si l'etape scp a ete atteinte)"
  finalize_receipt arret_tente "${stop_rc}" "${rc}"
  if [ "${stop_rc}" -ne 0 ]; then
    echo "[ARRET NON CERTIFIE] stop_and_verify a rendu ${stop_rc} — echec bloquant" >&2
    exit 70
  fi
  exit "${rc}"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

gcloud config set project "${GCP_PROJECT_ID}" >/dev/null

# ---- PROFIL DE CAMPAGNE EPINGLE AVANT TOUTE MUTATION (P1 : la matrice est
# fixee independamment des sorties que le validateur jugera).
{
  echo "profil=decision_v1"
  echo "conf_specs=${CONF_SPECS}"
  echo "bench_families=${BENCH_FAMILIES}"
  echo "bench_n=${BENCH_N}"
  echo "queue_families=${QUEUE_FAMILIES}"
  echo "queue_n=${QUEUE_N}"
  echo "queue_seeds=${QUEUE_SEEDS}"
  echo "threads=${THREADS_VM}"
} > "${PROFILE}.tmp"
mv "${PROFILE}.tmp" "${PROFILE}"
log "profil de campagne epingle : $(sha256sum "${PROFILE}" | awk '{print $1}')"

# ---- PREFLIGHT BUDGETAIRE (P1) : estimations CONSERVATRICES declarees ici,
# jamais ajustees apres mesure. Refus AVANT toute mutation si l'estimation
# depasse la fenetre utile.
budget_estimate() {
  python3 - "${CONF_SPECS}" "${BENCH_FAMILIES}" "${BENCH_N}" "${QUEUE_FAMILIES}" "${QUEUE_N}" "${QUEUE_SEEDS}" <<'PY'
import sys
conf = sys.argv[1].split()
bench_f, bench_n = sys.argv[2].split(), sys.argv[3].split()
queue_f, queue_n, queue_s = sys.argv[4].split(), sys.argv[5].split(), sys.argv[6].split()
# secondes par run, VM 48 fils, DECLARE AVANT MESURE (recus v5 G4 a 200k
# ~260s x marge 1,5-2 ; jamais ajuste apres coup) :
conf_ref = {32000: 60, 50000: 100, 100000: 200, 200000: 450}
bench = {32000: 80, 100000: 220, 200000: 480}
queue = {64000: 150, 128000: 350, 256000: 800}
total = 0
for spec in conf:
    n = int(spec.split(":")[1])
    total += 2 * conf_ref.get(n, 900)  # reference v5 + juge v6
for n in bench_n:
    total += 4 * len(bench_f) * bench.get(int(n), 900)
for n in queue_n:
    total += len(queue_f) * len(queue_s) * queue.get(int(n), 1800)
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
SSH_TTL_MIN=$((MAX_RUN_SECONDS / 60 + 20))
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
  --mutation-witness-file "${WITNESS}" 2>&1 | tee -a "${LOG}"
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

# ---- 4. Envoi du BUNDLE pinne.
timeout "${SCP_STEP_TIMEOUT_S}" "${SCP[@]}" "${BUNDLE}" \
  "${GCP_INSTANCE_NAME}:${REMOTE_BUNDLE}" 2>&1 | tee -a "${LOG}"

# ---- 5. Build v5 + v6, preconditions, REJEU des portes avec JOURNAL COMPLET
# et planchers (P1 : jamais un tail -4). Le boot_id est capture et reverifie
# par la campagne (handshake).
BUILD_LOG="${WORK}/build_vm.log"
timeout "${SSH_STEP_TIMEOUT_S}" "${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  echo "boot_id=$(cat /proc/sys/kernel/random/boot_id)"
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
BOOT_ID="$(sed -n 's/^boot_id=//p' "${BUILD_LOG}" | head -1)"
[ -n "${BOOT_ID}" ] || { log "REFUS : boot_id absent du journal de build"; SESSION_RC=75; exit 75; }
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
    BENCH_FAMILIES='${BENCH_FAMILIES}' BENCH_N='${BENCH_N}' \
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
    SCP_RC=0
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
  "${REMOTE_CAMPAIGN_RC}" "${SCP_RC}" "${PROFILE}" 2>&1 | tee -a "${LOG}"
VALIDATE_RC=${PIPESTATUS[0]}
set -e
if [ "${VALIDATE_RC}" -ne 0 ]; then SESSION_RC=65; fi

echo "session terminee ; l arret certifie est declenche par le trap"
