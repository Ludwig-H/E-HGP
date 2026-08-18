#!/usr/bin/env bash
# Session G4 — SCALE_THREADS v4 (audits bloquants 9223888 / b3a6eb4) :
# l'experience qui decide de l'architecture, dans une session dont le
# BUDGET TEMPOREL contient la somme des timeouts. Deux phases, UNE
# session chacune : PHASE=n32000 (defaut) ou PHASE=n64000.
#
# Memes garanties que session_campagne_v4_scale_g4.sh (pin de source et
# de protocole, GNU time, rapatriement toujours, validation locale seule
# autorite, arret certifie sur exactement la generation demarree), PLUS :
#
#  - PREFLIGHT DE BUDGET (audit b3a6eb4 § 2-3) : avant TOUTE mutation
#    GCP, required = build_margin + somme des timeouts sequentiels
#    (imprimee par le runner lui-meme : source de verite UNIQUE,
#    `--print-budget`) + marge de rapatriement, et l'on exige
#      MAX_RUN_SECONDS        >= required,
#      60*GUEST_SHUTDOWN_MIN  >  required,
#      60*SSH_KEY_TTL_MIN     >  60*GUEST_SHUTDOWN_MIN + 600,
#      MAX_RUN_SECONDS        <= 28800 (garde AGENTS.md : jamais > 8 h).
#    Un defaut = REFUS avant start_and_verify, code 2.
#  - DEADLINE AU RUNNER : le guest recoit une deadline epoch derivee de
#    l'arret invite ; il refuse de DEMARRER un run qui ne tient plus
#    (statut not_run_budget, campagne partielle honnete).
#  - APPARIEMENT PAR DIGEST : chaque run publie la signature canonique
#    (--digest) ; le validateur epingle exige l'egalite t1/t8/tmax.
# Porte transactionnelle : gcp-migration/selftest_scale_threads.sh.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
PHASE="${PHASE:-n32000}"
RUN_TIMEOUT="${RUN_TIMEOUT:-3600}"
RETRIEVE_MARGIN="${RETRIEVE_MARGIN:-900}"
BUILD_MARGIN="${BUILD_MARGIN:-1800}"
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-25200}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-400}"
SSH_KEY_TTL_MINUTES="${SSH_KEY_TTL_MINUTES:-420}"

# ---- 0a. PREFLIGHT DE BUDGET, avant toute action GCP. La somme des
# timeouts vient du RUNNER (la meme liste de runs que le guest executera,
# avec le meme RUN_TIMEOUT exporte — jamais une constante commentee).
export RUN_TIMEOUT RETRIEVE_MARGIN
SEQ_BUDGET="$(bash gcp-migration/v4_scale_threads_remote.sh --print-budget "${PHASE}")"
REQUIRED=$((BUILD_MARGIN + SEQ_BUDGET + RETRIEVE_MARGIN))
echo "budget : phase=${PHASE} somme_timeouts=${SEQ_BUDGET}s requis=${REQUIRED}s" \
     "max_run=${MAX_RUN_SECONDS}s guest=$((GUEST_SHUTDOWN_MINUTES * 60))s" \
     "ttl=$((SSH_KEY_TTL_MINUTES * 60))s"
if [ "${MAX_RUN_SECONDS}" -gt 28800 ]; then
  echo "REFUS : MAX_RUN_SECONDS > 8 h (garde AGENTS.md)" >&2
  exit 2
fi
if [ "${MAX_RUN_SECONDS}" -lt "${REQUIRED}" ]; then
  echo "REFUS : MAX_RUN_SECONDS (${MAX_RUN_SECONDS}) < budget requis (${REQUIRED})" >&2
  exit 2
fi
if [ $((GUEST_SHUTDOWN_MINUTES * 60)) -le "${REQUIRED}" ]; then
  echo "REFUS : arret invite ($((GUEST_SHUTDOWN_MINUTES * 60))s) <= budget requis (${REQUIRED})" >&2
  exit 2
fi
if [ $((SSH_KEY_TTL_MINUTES * 60)) -le $((GUEST_SHUTDOWN_MINUTES * 60 + 600)) ]; then
  echo "REFUS : TTL de la cle SSH trop court pour l'arret invite + marge" >&2
  exit 2
fi

WORK="$(mktemp -d /tmp/ehgp-v4thr-session.XXXXXXXX)"
HANDOFF="${WORK}/handoff.json"
LOG="${WORK}/session.log"
echo "session scale_threads ${PHASE} dans ${WORK}"

# ---- 0b. PIN DU PROTOCOLE, avant toute mutation GCP.
PIN_OUT="$(./gcp-migration/v4_scale_threads_pin.sh "${WORK}")"
echo "${PIN_OUT}" | tee -a "${LOG}"
SOURCE_COMMIT="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^source_commit=//p')"
SOURCE_PAYLOAD_SHA256="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^source_payload_sha256=//p')"
PROTOCOL_MANIFEST_SHA256="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^protocol_manifest_sha256=//p')"
BUNDLE="${WORK}/bundle.tgz"

gcloud config set project "${GCP_PROJECT_ID}" >/dev/null

# ---- 1. Borne de duree persistee sur l'instance ARRETEE.
./gcp-migration/set_max_run_duration_and_verify.sh --yes \
  --max-run-duration-seconds "${MAX_RUN_SECONDS}" 2>&1 | tee -a "${LOG}"

# ---- 2. Cle OS Login ephemere, TTL borne par le preflight.
export GCP_SSH_KEY_DIR="${WORK}/ssh"
mkdir -p "${GCP_SSH_KEY_DIR}"
chmod 700 "${GCP_SSH_KEY_DIR}"
export GCP_SSH_KEY_FILE="${GCP_SSH_KEY_DIR}/id_ed25519"
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-v4thr-session' -f "${GCP_SSH_KEY_FILE}"
chmod 600 "${GCP_SSH_KEY_FILE}"
GCP_SSH_KEY_EXPIRATION_UTC="$(python3 -c "from datetime import datetime,timedelta,timezone; print((datetime.now(timezone.utc)+timedelta(minutes=${SSH_KEY_TTL_MINUTES})).isoformat(timespec='seconds').replace('+00:00','Z'))")"
export GCP_SSH_KEY_EXPIRATION_UTC
gcloud compute os-login ssh-keys add --key-file="${GCP_SSH_KEY_FILE}.pub" \
  --ttl="${SSH_KEY_TTL_MINUTES}m" --project="${GCP_PROJECT_ID}" >/dev/null

# ---- 3. Demarrage garde + DEADLINE partagee avec le runner : l'arret
# invite est l'horizon dur ; le runner garde sa propre marge.
SESSION_START_EPOCH="$(date +%s)"
DEADLINE_EPOCH=$((SESSION_START_EPOCH + GUEST_SHUTDOWN_MINUTES * 60))
./gcp-migration/start_and_verify.sh --yes \
  --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}" \
  --handoff-file "${HANDOFF}" 2>&1 | tee -a "${LOG}"

GENERATION="$(python3 -c "import json,sys; print(json.load(open('${HANDOFF}'))['last_start_timestamp'])")"
echo "generation verrouillee : ${GENERATION} ; deadline=${DEADLINE_EPOCH}" | tee -a "${LOG}"

SESSION_RC=0
cleanup() {
  local rc=$?
  if [ "${SESSION_RC}" -ne 0 ]; then rc="${SESSION_RC}"; fi
  echo "--- arret certifie (rc=${rc}) ---" | tee -a "${LOG}"
  local stop_rc=0
  ./gcp-migration/stop_and_verify.sh --yes \
    --expected-last-start-timestamp "${GENERATION}" 2>&1 | tee -a "${LOG}" || stop_rc=$?
  echo "journal complet : ${LOG}"
  echo "resultats rapatries : ${WORK}/out (si l'etape scp a ete atteinte)"
  if [ "${stop_rc}" -ne 0 ]; then
    echo "[ARRET NON CERTIFIE] stop_and_verify a rendu ${stop_rc} — echec bloquant" | tee -a "${LOG}"
    exit 70
  fi
  exit "${rc}"
}
trap cleanup EXIT

SSH=(gcloud compute ssh "${GCP_INSTANCE_NAME}" --project="${GCP_PROJECT_ID}"
     --zone="${GCP_ZONE}" --ssh-key-file="${GCP_SSH_KEY_FILE}"
     --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" --quiet --command)
SCP=(gcloud compute scp --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}"
     --quiet --ssh-key-file="${GCP_SSH_KEY_FILE}"
     --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}")

# ---- 4. Bundle pinne.
"${SCP[@]}" "${BUNDLE}" "${GCP_INSTANCE_NAME}:/tmp/v4thrbundle.tgz" \
  2>&1 | tee -a "${LOG}"

# ---- 5. Build Release + preconditions + rejeu des portes.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  test -x /usr/bin/time || { echo "REFUS : GNU time absent de la VM" >&2; exit 2; }
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/v4thr && mkdir -p ~/v4thr && cd ~/v4thr
  echo "'"${SOURCE_PAYLOAD_SHA256}"'  /tmp/v4thrbundle.tgz" | sha256sum -c -
  tar xzf /tmp/v4thrbundle.tgz
  echo "coeurs=$(nproc)"; grep MemTotal /proc/meminfo; cmake --version | head -1
  cmake -S morsehgp3D_v4 -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j48
  ctest --test-dir build --output-on-failure -j24 2>&1 | tail -4
' 2>&1 | tee -a "${LOG}"

# ---- 6. LA CAMPAGNE (retour capture, rapatriement toujours).
REMOTE_RC=0
set +e
"${SSH[@]}" "set -euo pipefail
  export PATH=\$HOME/.local/bin:\$PATH
  cd ~/v4thr
  RUN_TIMEOUT=${RUN_TIMEOUT} RETRIEVE_MARGIN=${RETRIEVE_MARGIN} \
  bash gcp-migration/v4_scale_threads_remote.sh ${SOURCE_COMMIT} ${SOURCE_PAYLOAD_SHA256} ${PROTOCOL_MANIFEST_SHA256} ${PHASE} ${DEADLINE_EPOCH}
" 2>&1 | tee -a "${LOG}"
REMOTE_RC=${PIPESTATUS[0]}
set -e
printf 'remote_rc=%d\n' "${REMOTE_RC}" | tee -a "${LOG}"

# ---- 7. RAPATRIEMENT TOUJOURS.
mkdir -p "${WORK}/out"
SCP_RC=1
for attempt in 1 2 3; do
  set +e
  "${SCP[@]}" --recurse "${GCP_INSTANCE_NAME}:~/v4thr/out" "${WORK}/" \
    2>&1 | tee -a "${LOG}"
  rc=${PIPESTATUS[0]}
  set -e
  if [ "${rc}" -eq 0 ]; then
    SCP_RC=0
    break
  fi
  echo "scp tentative ${attempt} echouee (rc=${rc})" | tee -a "${LOG}"
  sleep $((5 * attempt))
done
printf 'scp_rc=%d\n' "${SCP_RC}" | tee -a "${LOG}"

# ---- 8. VALIDATION LOCALE par le validateur EPINGLE.
set +e
python3 "${WORK}/pinned/gcp-migration/validate_v4_scale_threads.py" \
  "${WORK}/out" "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" \
  "${PROTOCOL_MANIFEST_SHA256}" "${REMOTE_RC}" "${SCP_RC}" "${PHASE}" \
  2>&1 | tee -a "${LOG}"
VALIDATE_RC=${PIPESTATUS[0]}
set -e
if [ "${VALIDATE_RC}" -ne 0 ]; then SESSION_RC=65; fi

echo "session terminee ; l arret certifie est declenche par le trap"
echo "recette : coller ${WORK}/out/*.txt et *.status dans un reçu, avec le pin"
