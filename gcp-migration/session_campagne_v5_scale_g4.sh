#!/usr/bin/env bash
# Session G4 — CAMPAGNE D'ECHELLE v5 : conformite v4 aux tailles d'interet
# (8000/16000/32000, quatre familles) PUIS contrat 50 000 points (mesure),
# moteur CPU de reference (aucune cible CUDA en v5). VERSION TRANSACTIONNELLE, PINNEE
# ET ROBUSTE AUX RUPTURES (audits « campagne transactionnelle »,
# « pin source et RSS », « rapatriement apres rupture SSH »).
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# GARANTIES DE PROTOCOLE :
#  - PIN DE SOURCE : refus si l'arbre morsehgp3D_v5 differe de HEAD
#    (index, worktree, non-suivis) ; le payload est produit par
#    `git archive` depuis le COMMIT (jamais l'etat du disque) ; le couple
#    (source_commit, source_tar_sha256) est journalise avant demarrage,
#    verifie sur la VM, grave dans CHAQUE .status et exige identique par
#    le validateur ;
#  - GNU time OBLIGATOIRE sur la VM (verifie au build, avant les pilotes) —
#    le repli VmHWM qui mesurait le wrapper est supprime ;
#  - deux campagnes (latence isolee / couverture contendue), concurrence
#    pilotee par la memoire, pilotes VALIDES avant toute vague (script
#    distant : gcp-migration/v5_campaign_remote.sh) ;
#  - RAPATRIEMENT TOUJOURS : le retour SSH de campagne est capture SANS
#    declencher le trap, puis le scp est tente trois fois — une rupture
#    SSH ne peut plus eteindre la VM avant la recolte des statuts ;
#  - la VALIDATION LOCALE (gcp-migration/validate_v5_campaign.py) seule
#    decide de campaign_status ; `complete` exige aussi remote_rc=0 et
#    scp_rc=0 ; sinon partial_or_failed et sortie NON NULLE — apres
#    rapatriement, jamais avant.
# Porte transactionnelle a faux probe : gcp-migration/selftest_campagne_v5.sh.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
# Repli exact autorise : GCP_ZONE=europe-west4-ai1a
# GCP_INSTANCE_NAME=ehgp-blackwell-spot-ai1a (voir README).
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-14400}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-230}"

WORK="$(mktemp -d /tmp/ehgp-v5scale-session.XXXXXXXX)"
HANDOFF="${WORK}/handoff.json"
LOG="${WORK}/session.log"
echo "session dans ${WORK}"

# ---- 0. PIN DU PROTOCOLE, avant toute mutation GCP (audits « pin
# source » puis « pin du protocole ») : le moteur ET le protocole qui
# choisit les runs et les juge (lanceur, runner, validateur) doivent
# correspondre a HEAD ; bundle et scripts epingles sont materialises
# depuis le COMMIT — le worktree n'intervient plus apres cette etape.
PIN_OUT="$(./gcp-migration/v5_campaign_pin.sh "${WORK}")"
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
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-v5scale-session' -f "${GCP_SSH_KEY_FILE}"
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

# ---- Arret certifie quoi qu'il arrive, sur EXACTEMENT cette generation.
# Le trap ne se declenche qu'APRES les tentatives de rapatriement : les
# blocs campagne et scp capturent leurs codes sans errexit.
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

# ---- 4. Envoi du BUNDLE pinne (moteur + runner + validateur, depuis le
# commit) — aucun script du worktree n'est transfere.
"${SCP[@]}" "${BUNDLE}" "${GCP_INSTANCE_NAME}:/tmp/v5bundle.tgz" \
  2>&1 | tee -a "${LOG}"

# ---- 5. Build Release + preconditions + REJEU INDEPENDANT des portes.
# Un echec ARRETE la session avant toute mesure : pas de campagne sur un
# build non conforme, une integrite de payload non verifiee ou sans GNU time.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  test -x /usr/bin/time || { echo "REFUS : GNU time absent de la VM" >&2; exit 2; }
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/v5scale && mkdir -p ~/v5scale && cd ~/v5scale
  echo "'"${SOURCE_PAYLOAD_SHA256}"'  /tmp/v5bundle.tgz" | sha256sum -c -
  tar xzf /tmp/v5bundle.tgz
  echo "coeurs=$(nproc)"; grep MemTotal /proc/meminfo; cmake --version | head -1
  cmake -S morsehgp3D_v5 -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j48
  ctest --test-dir build --output-on-failure -L gate -j24 2>&1 | tail -4
' 2>&1 | tee -a "${LOG}"

# ---- 6. LA CAMPAGNE. Le retour SSH est CAPTURE sans declencher le trap
# (audit « rupture SSH ») : quoi qu'il rende, le rapatriement suit.
REMOTE_CAMPAIGN_RC=0
set +e
"${SSH[@]}" "set -euo pipefail
  export PATH=\$HOME/.local/bin:\$PATH
  cd ~/v5scale
  THREADS=48 EXTRA_N='${EXTRA_N:-}' EXTRA_FAMILIES='${EXTRA_FAMILIES:-uniform eight_clusters}' bash gcp-migration/v5_campaign_remote.sh ${SOURCE_COMMIT} ${SOURCE_PAYLOAD_SHA256} ${PROTOCOL_MANIFEST_SHA256}
" 2>&1 | tee -a "${LOG}"
REMOTE_CAMPAIGN_RC=${PIPESTATUS[0]}
set -e
printf 'remote_campaign_rc=%d\n' "${REMOTE_CAMPAIGN_RC}" | tee -a "${LOG}"

# ---- 7. RAPATRIEMENT TOUJOURS, avec reprises bornees.
mkdir -p "${WORK}/out"
SCP_RC=1
for attempt in 1 2 3; do
  set +e
  "${SCP[@]}" --recurse "${GCP_INSTANCE_NAME}:~/v5scale/out" "${WORK}/" \
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
python3 "${WORK}/pinned/gcp-migration/validate_v5_campaign.py" "${WORK}/out" \
  "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}" \
  "${REMOTE_CAMPAIGN_RC}" "${SCP_RC}" 2>&1 | tee -a "${LOG}"
VALIDATE_RC=${PIPESTATUS[0]}
set -e
if [ "${VALIDATE_RC}" -ne 0 ]; then SESSION_RC=65; fi

echo "session terminee ; l arret certifie est declenche par le trap"
echo "recette : coller ${WORK}/out/*.txt et *.status dans un reçu, avec le pin"
