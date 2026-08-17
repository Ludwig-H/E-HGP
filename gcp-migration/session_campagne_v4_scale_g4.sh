#!/usr/bin/env bash
# Session G4 — CAMPAGNE D'ECHELLE v4 : les tailles contractuelles sur les
# deux profils, moteur CPU de reference.
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# ---------------------------------------------------------------------------
# CE QU'ELLE MESURE, ET CE QU'ELLE NE REVENDIQUE PAS
#
# La v4 n'a pas encore de cible CUDA : cette session emploie la G4 comme
# RESSOURCE DE CALCUL CPU (48 vCPU, 180 Go) et comme materiel contractuel de
# reference pour les recus de cout. Elle ne mesure AUCUN debit GPU et ne
# revendique aucun SLO ; le contrat <100 ms K=10 sur G4 vise le futur backend
# CUDA, pas ce moteur de reference. Aucun benchmark ne promeut
# public_status=exact : les sorties sont des COMPTEURS et des temps, les
# invariants restent etablis par les 89 portes CTest (rejouees ici sur la VM,
# rejeu independant du poste de developpement).
#
# LA MATRICE : les quatre familles contractuelles × les tailles d'interet
# n = 8000 / 16000 / 32000 (docs/TEST_PLAN § 3.1) × les deux profils
# (smax=11 = K_max 10 ; smax=6 = K_max 5). 24 runs mono-thread en parallele
# sur 48 cœurs. Chaque run publie les compteurs du pipeline filtre
# (candidats emis/tues par lane, ancres W4, boules uniques, clefs au census,
# evenements, fusions, nœuds) et les temps separes t_gen/t_tri/t_prefiltre/
# t_census/t_fold — les pentes 400/800/1600 du reçu
# ADDENDUM_PENTES_400_800_1600_20260817.md sont la baseline a prolonger.
#
# DUREE : extrapolation des pentes locales (~×2,6 par doublement) : le pire
# run (uniform n=32000 smax=11) ~1,5-2 h mono-thread, fold domine. La borne
# GCE est de 4 h avec shutdown invite a 235 min ; chaque run individuel est
# borne par `timeout 10800`. Une session interrompue par le coupe-circuit
# livre les runs termines (les fichiers out/ sont cat dans le journal au fil
# de l'eau par run, pas seulement a la fin).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
# Repli exact autorise : GCP_ZONE=europe-west4-ai1a
# GCP_INSTANCE_NAME=ehgp-blackwell-spot-ai1a (voir README).
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-14400}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-235}"

WORK="$(mktemp -d /tmp/ehgp-v4scale-session.XXXXXXXX)"
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
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-v4scale-session' -f "${GCP_SSH_KEY_FILE}"
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
cleanup() {
  local rc=$?
  echo "--- arret certifie (rc=${rc}) ---" | tee -a "${LOG}"
  # FAIL-CLOSED : un arret cible illisible doit BLOQUER, jamais etre avale.
  local stop_rc=0
  ./gcp-migration/stop_and_verify.sh --yes \
    --expected-last-start-timestamp "${GENERATION}" 2>&1 | tee -a "${LOG}" || stop_rc=$?
  echo "journal complet : ${LOG}"
  echo "resultats rapatries : ${WORK}/out (si l'etape 7 a ete atteinte)"
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

# ---- 4. Envoi des sources (v4 seul, l'arbre est autonome).
TAR="${WORK}/v4.tgz"
tar czf "${TAR}" --exclude=build --exclude=.git morsehgp3D_v4
gcloud compute scp "${TAR}" "${GCP_INSTANCE_NAME}:/tmp/v4.tgz" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
  --ssh-key-file="${GCP_SSH_KEY_FILE}" \
  --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" 2>&1 | tee -a "${LOG}"

# ---- 5. Build Release.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/v4scale && mkdir -p ~/v4scale && cd ~/v4scale
  tar xzf /tmp/v4.tgz
  echo "coeurs=$(nproc)"; cmake --version | head -1
  cmake -S morsehgp3D_v4 -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j48
' 2>&1 | tee -a "${LOG}"

# ---- 6. REJEU INDEPENDANT des 89 portes sur la VM. Un echec ARRETE la
# session avant toute mesure : pas de campagne sur un build non conforme.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/v4scale
  ctest --test-dir build --output-on-failure -j24 2>&1 | tail -6
' 2>&1 | tee -a "${LOG}"

# ---- 7. LA CAMPAGNE : 4 familles × 3 tailles × 2 profils, 24 runs
# mono-thread en parallele. Chaque run est borne ; un run qui depasse rend
# code=124 dans son fichier, jamais un blocage de session. Les floors
# min-balls/min-fusions gardent le vert-par-vacuite.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/v4scale
  P=./build/mhgp4_forest_probe
  mkdir -p out
  for fam in uniform terrain eight_clusters scanline_overlap_multiecho; do
    for n in 8000 16000 32000; do
      for smax in 11 6; do
        ( timeout 10800 $P --family=$fam --n=$n --s=8 --smax=$smax --seed=3 \
            --min-balls=10000 --min-fusions=1000 \
            > out/scale_${fam}_n${n}_smax${smax}.txt 2>&1
          echo "code=$?" >> out/scale_${fam}_n${n}_smax${smax}.txt
          echo "--- fini ${fam} n=${n} smax=${smax}"
          cat out/scale_${fam}_n${n}_smax${smax}.txt ) &
      done
    done
  done
  wait || true
  echo "=== CAMPAGNE COMPLETE ==="
  for f in out/scale_*.txt; do echo "--- $f"; cat "$f"; done
' 2>&1 | tee -a "${LOG}"

# ---- 8. Rapatriement durable des resultats.
mkdir -p "${WORK}/out"
gcloud compute scp --recurse "${GCP_INSTANCE_NAME}:~/v4scale/out" "${WORK}/" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
  --ssh-key-file="${GCP_SSH_KEY_FILE}" \
  --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" 2>&1 | tee -a "${LOG}"

echo "session terminee ; l arret certifie est declenche par le trap"
echo "recette : coller ${WORK}/out/*.txt dans un reçu"
