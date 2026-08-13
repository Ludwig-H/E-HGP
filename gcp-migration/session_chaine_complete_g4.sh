#!/usr/bin/env bash
# Session G4 — la mesure qui peut REFUTER la voie par intervalles.
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# ---------------------------------------------------------------------------
# CE QU'ELLE MESURE, ET POURQUOI ELLE N'EST PAS UNE QUALIFICATION DEVICE
#
# Le contre-audit maintient G4 en NO-GO pour QUALIFIER une ordonnance sur
# device : deux pentes vertes, un cap d'octets, le lowering deux limbes et un
# residuel authentifie sont exiges avant cela, et rien de tout ceci n'existe.
# Cette session ne fait donc AUCUNE mesure de debit GPU et ne revendique aucun
# SLO.
#
# Elle emploie la machine comme une RESSOURCE DE CALCUL CPU, pour une seule
# raison : la mesure decisive est un diagnostic borne en `n(n-1)` a
# `n=50 000`, soit environ `1,2e11` operations par famille, et le poste de
# developpement a deux vCPU. Quarante-huit coeurs permettent de lancer les
# quatre familles contractuelles en parallele.
#
# LA QUESTION MESUREE est celle de la note `NOTE_CLAUDE_DESCENTE_JOINTE` :
#
#     la CHAINE COMPLETE tient-elle les deux pentes sur sa SORTIE ?
#
# C'est la premiere session qui mesure un objet PRODUIT — les supports emis avec
# leur rang et leur coquille — et non un compteur intermediaire. Elle publie
# aussi le high-water `kept`, c'est-a-dire la fenetre par ancre, pour la
# rapprocher de la mesure independante `somme_a |N_q(a)|` du front WSPD.
#
# Critere du depot : deux pentes log2 consecutives `>= 1,35` REFUSENT. La
# sortie peut donc etre une refutation, et c'est son objet.
#
# Elle verifie en outre que la cible CUDA opt-in COMPILE, ce que le poste local
# ne peut pas faire faute de `nvcc`.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-ai1a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot-ai1a}"
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-3600}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-45}"

WORK="$(mktemp -d /tmp/ehgp-chaine-session.XXXXXXXX)"
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
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-residuel-session' -f "${GCP_SSH_KEY_FILE}"
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
  # FAIL-CLOSED (audit `96be8e0`, section 10). Un arret cible illisible doit
  # BLOQUER, jamais etre avale par `|| true` : c'est le passage de relais.
  local stop_rc=0
  ./gcp-migration/stop_and_verify.sh --yes \
    --expected-last-start-timestamp "${GENERATION}" 2>&1 | tee -a "${LOG}" || stop_rc=$?
  echo "journal complet : ${LOG}"
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

# ---- 4. Envoi des sources.
TAR="${WORK}/v3.tgz"
tar czf "${TAR}" --exclude=build --exclude=.git morsehgp3D_v2 morsehgp3D_v3
gcloud compute scp "${TAR}" "${GCP_INSTANCE_NAME}:/tmp/v3.tgz" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
  --ssh-key-file="${GCP_SSH_KEY_FILE}" \
  --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" 2>&1 | tee -a "${LOG}"

# ---- 5. Build CPU, puis VERIFICATION DE COMPILATION CUDA.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:/usr/local/cuda/bin:$PATH
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/chaine && mkdir -p ~/chaine && cd ~/chaine
  tar xzf /tmp/v3.tgz
  echo "coeurs=$(nproc)"; cmake --version | head -1; nvcc --version | tail -2
  cmake -S morsehgp3D_v3 -B build -DCMAKE_BUILD_TYPE=Release
  # TOUTES les cibles dont les portes sont rejouees, sinon ctest rend `Not Run`
  # et `set -e` coupe la session AVANT la mesure.
  cmake --build build --target mhgp3v_anchor_source mhgp3v_wspd_wavefront_probe \
        mhgp3v_wspd_front_probe mhgp3v_rect_front_probe -j48
' 2>&1 | tee -a "${LOG}"

# La cible CUDA exige un worktree propre cote produit ; ici seule la
# compilation de v3 nous interesse. Un echec est RAPPORTE, pas masque.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:/usr/local/cuda/bin:$PATH
  cd ~/chaine
  echo "=== compilation CUDA opt-in ==="
  cmake -S morsehgp3D_v3 -B build-cuda -DCMAKE_BUILD_TYPE=Release \
    -DMHGP3V_ENABLE_CUDA=ON -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc \
    -DCMAKE_CUDA_ARCHITECTURES=120-real 2>&1 | tail -5
  cmake --build build-cuda --target mhgp3v_anchor_device -j48 2>&1 | tail -20 \
    && echo "CUDA_COMPILE=OK" || echo "CUDA_COMPILE=ECHEC"
' 2>&1 | tee -a "${LOG}"

# ---- 6. Portes locales, reconstruites sur la VM. Rejeu independant.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/chaine
  ctest --test-dir build --output-on-failure -j24 \
    -R "^mhgp3v_(rect_front|wspd)_" 2>&1 | tail -30
' 2>&1 | tee -a "${LOG}"

# ---- 7. LA CHAINE COMPLETE. Quatre familles, rampe longue, moteur de
# reference multi-thread. Chaque run publie supports par lane, high-water,
# identite des cles et temps mur.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/chaine
  P=./build/mhgp3v_anchor_source
  mkdir -p out
  for fam in uniform terrain eight_clusters scanline_overlap_multiecho; do
    ( for n in 6250 12500 25000 50000; do
        echo "=== $fam n=$n"
        timeout 900 $P --family=$fam --points=$n --smax=11 --threads=12 2>&1 | tail -6
      done > out/chaine_${fam}.txt 2>&1; echo "code=$?" >> out/chaine_${fam}.txt ) &
  done
  wait || true
  echo "=== CHAINE COMPLETE ==="
  for f in out/chaine_*.txt; do echo "--- $f"; cat "$f"; done
' 2>&1 | tee -a "${LOG}"

# ---- 8. La fenetre par ancre, mesuree independamment sur le front WSPD, pour
# etre rapprochee du high-water `kept` de la chaine.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/chaine
  W=./build/mhgp3v_wspd_wavefront_probe
  for fam in uniform terrain eight_clusters scanline_overlap_multiecho; do
    ( $W --family=$fam --sep-euclid=3/1 --points=6250,12500,25000,50000 --tight          --vwave --window=256 > out/fenetre_${fam}.txt 2>&1; echo "code=$?" >> out/fenetre_${fam}.txt ) &
  done
  wait || true
  echo "=== FENETRE PAR ANCRE ==="
  for f in out/fenetre_*.txt; do echo "--- $f"; cat "$f"; done
' 2>&1 | tee -a "${LOG}"

echo "session terminee ; l arret certifie est declenche par le trap"
