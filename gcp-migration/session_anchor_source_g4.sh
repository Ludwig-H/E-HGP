#!/usr/bin/env bash
# Session G4 — source par ancre maximale : rampe 48 coeurs et parite device.
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# Ce qu'elle mesure :
#   - la suite de portes `mhgp3v_anchor_` reconstruite sur la VM ;
#   - la rampe contractuelle 12 500 / 25 000 / 50 000 sur `uniform` et
#     `terrain`, avec 48 threads, compteurs complets ;
#   - la parite hote/device et le temps de noyau CUDA a 50 000 points.
#
# Elle ne qualifie NI le SLO `warm_e2e`, NI le payload officiel : ni les dix
# forets, ni les verticales, ni les lots, ni le certificat minimal n'existent.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-ai1a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot-ai1a}"
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-3600}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-45}"

WORK="$(mktemp -d /tmp/ehgp-anchor-session.XXXXXXXX)"
HANDOFF="${WORK}/handoff.json"
LOG="${WORK}/session.log"
echo "session dans ${WORK}"

gcloud config set project "${GCP_PROJECT_ID}" >/dev/null

# ---- 1. Borne de duree persistee sur l'instance ARRETEE.
./gcp-migration/set_max_run_duration_and_verify.sh --yes \
  --max-run-duration-seconds "${MAX_RUN_SECONDS}" 2>&1 | tee -a "${LOG}"

# ---- 2. Cle OS Login ephemere, TTL borne par la duree de la VM.
export GCP_SSH_KEY_DIR="${WORK}/ssh"
mkdir -p "${GCP_SSH_KEY_DIR}"
chmod 700 "${GCP_SSH_KEY_DIR}"
export GCP_SSH_KEY_FILE="${GCP_SSH_KEY_DIR}/id_ed25519"
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-anchor-session' -f "${GCP_SSH_KEY_FILE}"
chmod 600 "${GCP_SSH_KEY_FILE}"
GCP_SSH_KEY_EXPIRATION_UTC="$(python3 -c 'from datetime import datetime,timedelta,timezone; print((datetime.now(timezone.utc)+timedelta(minutes=70)).isoformat(timespec="seconds").replace("+00:00","Z"))')"
export GCP_SSH_KEY_EXPIRATION_UTC
gcloud compute os-login ssh-keys add --key-file="${GCP_SSH_KEY_FILE}.pub" --ttl=70m \
  --project="${GCP_PROJECT_ID}" >/dev/null

# ---- 3. Demarrage garde. Le trap d'echec du script appelle deja l'arret.
./gcp-migration/start_and_verify.sh --yes \
  --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}" \
  --handoff-file "${HANDOFF}" 2>&1 | tee -a "${LOG}"

GENERATION="$(python3 -c "import json,sys; print(json.load(open('${HANDOFF}'))['last_start_timestamp'])")"
echo "generation verrouillee : ${GENERATION}" | tee -a "${LOG}"

# ---- Arret certifie quoi qu'il arrive, sur EXACTEMENT cette generation.
cleanup() {
  local rc=$?
  echo "--- arret certifie (rc=${rc}) ---" | tee -a "${LOG}"
  ./gcp-migration/stop_and_verify.sh --yes \
    --expected-last-start-timestamp "${GENERATION}" 2>&1 | tee -a "${LOG}" || true
  echo "journal complet : ${LOG}"
  exit "${rc}"
}
trap cleanup EXIT

SSH=(gcloud compute ssh "${GCP_INSTANCE_NAME}" --project="${GCP_PROJECT_ID}"
     --zone="${GCP_ZONE}" --ssh-key-file="${GCP_SSH_KEY_FILE}"
     --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" --quiet --command)

# ---- 4. Envoi des sources. v3 seul suffit : il embarque v2 par add_subdirectory.
TAR="${WORK}/v3.tgz"
tar czf "${TAR}" --exclude=build --exclude=.git morsehgp3D_v2 morsehgp3D_v3
gcloud compute scp "${TAR}" "${GCP_INSTANCE_NAME}:/tmp/v3.tgz" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
  --ssh-key-file="${GCP_SSH_KEY_FILE}" \
  --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" 2>&1 | tee -a "${LOG}"

# ---- 5. Build. cmake de l'image est trop vieux et nvcc n'est pas dans PATH.
"${SSH[@]}" 'set -e
  export PATH=$HOME/.local/bin:/usr/local/cuda/bin:$PATH
  command -v cmake >/dev/null && cmake --version | head -1 || true
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/anchor && mkdir -p ~/anchor && cd ~/anchor
  tar xzf /tmp/v3.tgz
  nproc; cmake --version | head -1; nvcc --version | tail -2
  cmake -S morsehgp3D_v3 -B build -DCMAKE_BUILD_TYPE=Release \
    -DMHGP3V_ENABLE_CUDA=ON -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc \
    -DCMAKE_CUDA_ARCHITECTURES=120-real
  cmake --build build --target mhgp3v_anchor_source mhgp3v_anchor_device -j48
' 2>&1 | tee -a "${LOG}"

# ---- 6. Portes locales, reconstruites sur la VM.
"${SSH[@]}" 'set -e
  export PATH=$HOME/.local/bin:$PATH
  cd ~/anchor
  ctest --test-dir build -R "^mhgp3v_anchor_" --output-on-failure -j8 2>&1 | tail -30
' 2>&1 | tee -a "${LOG}"

# ---- 7. Rampe contractuelle 48 coeurs, compteurs complets.
"${SSH[@]}" 'set -e
  cd ~/anchor
  for f in uniform terrain; do
    for n in 12500 25000 50000; do
      echo "=== $f n=$n threads=48 ==="
      ./build/mhgp3v_anchor_source --points=$n --family=$f --seed=1 --threads=48 --no-store
    done
  done
' 2>&1 | tee -a "${LOG}"

# ---- 8. Parite hote/device et temps de noyau.
"${SSH[@]}" 'set -e
  cd ~/anchor
  nvidia-smi --query-gpu=name,memory.total,driver_version --format=csv,noheader
  for n in 4000 12500 50000; do
    echo "=== device n=$n ==="
    ./build/mhgp3v_anchor_device --points=$n --family=uniform --seed=1 --threads=48 \
      --slots=65536 || echo "rc=$?"
  done
  echo "=== device terrain n=50000 ==="
  ./build/mhgp3v_anchor_device --points=50000 --family=terrain --seed=1 --threads=48 \
    --slots=65536 || echo "rc=$?"
' 2>&1 | tee -a "${LOG}"

echo "SESSION TERMINEE" | tee -a "${LOG}"
