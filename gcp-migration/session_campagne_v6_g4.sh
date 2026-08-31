#!/usr/bin/env bash
# Session G4 — CAMPAGNE v6 : conformite v5 ≡ v6 a n=50000 (reference v5
# calculee sur la VM), PUIS bench apparie v5/v6 (ABBA, sans digest, murs et
# RSS par GNU time), PUIS queue stationnaire v6 (sonde E6 a l'echelle).
# VERSION TRANSACTIONNELLE, PINNEE ET ROBUSTE AUX RUPTURES — meme doctrine
# que session_campagne_v5_scale_g4.sh (audits « campagne transactionnelle »,
# « pin source et RSS », « rapatriement apres rupture SSH »).
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# GARANTIES DE PROTOCOLE :
#  - PIN DE SOURCE ET DE PROTOCOLE (gcp-migration/v6_campaign_pin.sh) :
#    refus si morsehgp3D_v5, morsehgp3D_v6 (hors audits/) ou les scripts de
#    campagne different de HEAD ; bundle par `git archive` depuis le COMMIT ;
#    (source_commit, sha256, manifeste) graves dans CHAQUE .status et exiges
#    identiques par le validateur ;
#  - GNU time OBLIGATOIRE sur la VM (verifie avant tout build) ;
#  - REJEU INDEPENDANT des portes v5 ET v6 (ctest -L gate) avant toute
#    mesure : pas de campagne sur un build non conforme ;
#  - ECHEANCE : DEADLINE_EPOCH = demarrage + MAX_RUN_SECONDS - 1500 s (marge
#    de rapatriement) transmise au runner, qui tronque AVANT un run trop
#    tardif et grave la troncature ;
#  - RAPATRIEMENT TOUJOURS : le retour SSH de campagne est capture SANS
#    declencher le trap, puis le scp est tente trois fois ;
#  - la VALIDATION LOCALE (validate_v6_campaign.py EPINGLE) seule decide de
#    campaign_status ; `complete` exige aussi remote_rc=0 et scp_rc=0.
# Porte transactionnelle a faux pilotes : gcp-migration/selftest_campagne_v6.sh
# (a lancer A LA MAIN avant toute session payante).
set -euo pipefail

# EXECUTION DEPUIS UNE COPIE : bash lit un script paresseusement ; une edition
# du fichier pendant une session longue corrompt sa suite. La copie est prise
# AVANT toute action GCP.
if [ -z "${MHGP6_SESSION_SELF_COPY:-}" ]; then
  _self_copy="$(mktemp /tmp/ehgp-v6session-copy.XXXXXXXX.sh)"
  cp "$0" "${_self_copy}"
  MHGP6_SESSION_REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
  export MHGP6_SESSION_REPO_ROOT
  MHGP6_SESSION_SELF_COPY="${_self_copy}" MHGP6_SESSION_SOURCE="$0" exec bash "${_self_copy}" "$@"
fi
echo "session executee depuis la copie ${MHGP6_SESSION_SELF_COPY} (source : ${MHGP6_SESSION_SOURCE:-?}, racine ${MHGP6_SESSION_REPO_ROOT:-?})"

REPO_ROOT="${MHGP6_SESSION_REPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
# Repli exact autorise : GCP_ZONE=europe-west4-ai1a
# GCP_INSTANCE_NAME=ehgp-blackwell-spot-ai1a (voir README).
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-14400}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-230}"

WORK="$(mktemp -d /tmp/ehgp-v6session.XXXXXXXX)"
HANDOFF="${WORK}/handoff.json"
LOG="${WORK}/session.log"
echo "session dans ${WORK}"

# ---- 0. PIN DU PROTOCOLE, avant toute mutation GCP : moteurs v5 + v6 et
# protocole (lanceur, runner, validateur) materialises depuis le COMMIT — le
# worktree n'intervient plus apres cette etape.
PIN_OUT="$(./gcp-migration/v6_campaign_pin.sh "${WORK}")"
echo "${PIN_OUT}" | tee -a "${LOG}"
SOURCE_COMMIT="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^source_commit=//p')"
SOURCE_PAYLOAD_SHA256="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^source_payload_sha256=//p')"
PROTOCOL_MANIFEST_SHA256="$(printf '%s\n' "${PIN_OUT}" | sed -n 's/^protocol_manifest_sha256=//p')"
BUNDLE="${WORK}/bundle.tgz"

gcloud config set project "${GCP_PROJECT_ID}" >/dev/null

# ---- 1. Borne de duree persistee sur l'instance ARRETEE.
./gcp-migration/set_max_run_duration_and_verify.sh --yes \
  --max-run-duration-seconds "${MAX_RUN_SECONDS}" 2>&1 | tee -a "${LOG}"

# ---- 2. Cle OS Login ephemere, TTL borne par la duree de la VM.
export GCP_SSH_KEY_DIR="${WORK}/ssh"
mkdir -p "${GCP_SSH_KEY_DIR}"
chmod 700 "${GCP_SSH_KEY_DIR}"
export GCP_SSH_KEY_FILE="${GCP_SSH_KEY_DIR}/id_ed25519"
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-v6-session' -f "${GCP_SSH_KEY_FILE}"
chmod 600 "${GCP_SSH_KEY_FILE}"
GCP_SSH_KEY_EXPIRATION_UTC="$(python3 -c 'from datetime import datetime,timedelta,timezone; print((datetime.now(timezone.utc)+timedelta(minutes=250)).isoformat(timespec="seconds").replace("+00:00","Z"))')"
export GCP_SSH_KEY_EXPIRATION_UTC
gcloud compute os-login ssh-keys add --key-file="${GCP_SSH_KEY_FILE}.pub" --ttl=250m \
  --project="${GCP_PROJECT_ID}" >/dev/null

# ---- 3. Demarrage garde. Le trap d'echec du script appelle deja l'arret.
./gcp-migration/start_and_verify.sh --yes \
  --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}" \
  --handoff-file "${HANDOFF}" 2>&1 | tee -a "${LOG}"

GENERATION="$(python3 -c "import json,sys; print(json.load(open('${HANDOFF}'))['last_start_timestamp'])")"
echo "generation verrouillee : ${GENERATION}" | tee -a "${LOG}"

# Echeance de campagne : le runner tronque AVANT un run qui depasserait.
DEADLINE_EPOCH="$(( $(date +%s) + MAX_RUN_SECONDS - 1500 ))"
echo "deadline_epoch=${DEADLINE_EPOCH}" | tee -a "${LOG}"

# ---- Arret certifie quoi qu'il arrive, sur EXACTEMENT cette generation.
# Le trap ne se declenche qu'APRES les tentatives de rapatriement.
SESSION_RC=0
cleanup() {
  local rc=$?
  if [ "${SESSION_RC}" -ne 0 ]; then rc="${SESSION_RC}"; fi
  echo "--- arret certifie (rc=${rc}) ---" | tee -a "${LOG}"
  # FAIL-CLOSED : un arret cible illisible doit BLOQUER, jamais etre avale.
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

# ---- 4. Envoi du BUNDLE pinne (moteurs + runner + validateur, depuis le
# commit) — aucun script du worktree n'est transfere.
"${SCP[@]}" "${BUNDLE}" "${GCP_INSTANCE_NAME}:/tmp/v6bundle.tgz" \
  2>&1 | tee -a "${LOG}"

# ---- 5. Build Release v5 + v6, preconditions, REJEU INDEPENDANT des portes
# des DEUX moteurs. Un echec ARRETE la session avant toute mesure.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  test -x /usr/bin/time || { echo "REFUS : GNU time absent de la VM" >&2; exit 2; }
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/v6camp && mkdir -p ~/v6camp && cd ~/v6camp
  echo "'"${SOURCE_PAYLOAD_SHA256}"'  /tmp/v6bundle.tgz" | sha256sum -c -
  tar xzf /tmp/v6bundle.tgz
  echo "coeurs=$(nproc)"; grep MemTotal /proc/meminfo; cmake --version | head -1
  cmake -S morsehgp3D_v5 -B build-v5 -DCMAKE_BUILD_TYPE=Release
  cmake --build build-v5 -j48
  ctest --test-dir build-v5 --output-on-failure -L gate -j24 2>&1 | tail -4
  cmake -S morsehgp3D_v6 -B build-v6 -DCMAKE_BUILD_TYPE=Release
  cmake --build build-v6 -j48
  ctest --test-dir build-v6 --output-on-failure -L gate -j24 2>&1 | tail -4
' 2>&1 | tee -a "${LOG}"

# ---- 6. LA CAMPAGNE. Le retour SSH est CAPTURE sans declencher le trap
# (audit « rupture SSH ») : quoi qu'il rende, le rapatriement suit.
REMOTE_CAMPAIGN_RC=0
set +e
"${SSH[@]}" "set -euo pipefail
  export PATH=\$HOME/.local/bin:\$PATH
  cd ~/v6camp
  THREADS=48 DEADLINE_EPOCH=${DEADLINE_EPOCH} \
    CONF_FAMILIES='${CONF_FAMILIES:-}' CONF_N='${CONF_N:-}' \
    BENCH_FAMILIES='${BENCH_FAMILIES:-}' BENCH_N='${BENCH_N:-}' \
    QUEUE_FAMILIES='${QUEUE_FAMILIES:-}' QUEUE_N='${QUEUE_N:-}' QUEUE_SEEDS='${QUEUE_SEEDS:-}' \
    RUN_TIMEOUT='${RUN_TIMEOUT:-}' \
    bash gcp-migration/v6_campaign_remote.sh ${SOURCE_COMMIT} ${SOURCE_PAYLOAD_SHA256} ${PROTOCOL_MANIFEST_SHA256}
" 2>&1 | tee -a "${LOG}"
REMOTE_CAMPAIGN_RC=${PIPESTATUS[0]}
set -e
printf 'remote_campaign_rc=%d\n' "${REMOTE_CAMPAIGN_RC}" | tee -a "${LOG}"

# ---- 7. RAPATRIEMENT TOUJOURS, avec reprises bornees.
mkdir -p "${WORK}/out"
SCP_RC=1
for attempt in 1 2 3; do
  set +e
  "${SCP[@]}" --recurse "${GCP_INSTANCE_NAME}:~/v6camp/out" "${WORK}/" \
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

# ---- 8. VALIDATION LOCALE par le validateur EPINGLE (jamais le worktree) :
# seule autorite du statut de campagne.
set +e
python3 "${WORK}/pinned/gcp-migration/validate_v6_campaign.py" "${WORK}/out" \
  "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}" \
  "${REMOTE_CAMPAIGN_RC}" "${SCP_RC}" 2>&1 | tee -a "${LOG}"
VALIDATE_RC=${PIPESTATUS[0]}
set -e
if [ "${VALIDATE_RC}" -ne 0 ]; then SESSION_RC=65; fi

echo "session terminee ; l arret certifie est declenche par le trap"
