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
# LA QUESTION MESUREE est celle de la note `QUESTIONS_CLAUDE_TUER_LA_VOIE` :
#
#     le residuel `PairId` devient-il `o(n^2)` ?
#
# Critere du depot : deux pentes log2 successives `>= 1,35` REFUSENT la voie,
# quelle que soit la fraction fermee. La sortie de cette session peut donc etre
# une refutation, et c'est son objet.
#
# Elle verifie en outre que la cible CUDA opt-in COMPILE, ce que le poste local
# ne peut pas faire faute de `nvcc`. C'est une verification de compilation, pas
# une mesure : l'ABI `run_anchor_point` avait ete cassee par un parametre
# `density_guard` non propage, et la reparation par suppression n'a jamais ete
# confrontee a un vrai compilateur CUDA.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-ai1a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot-ai1a}"
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-3600}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-45}"

WORK="$(mktemp -d /tmp/ehgp-residuel-session.XXXXXXXX)"
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
  ./gcp-migration/stop_and_verify.sh --yes \
    --expected-last-start-timestamp "${GENERATION}" 2>&1 | tee -a "${LOG}" || true
  echo "journal complet : ${LOG}"
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
"${SSH[@]}" 'set -e
  export PATH=$HOME/.local/bin:/usr/local/cuda/bin:$PATH
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/residuel && mkdir -p ~/residuel && cd ~/residuel
  tar xzf /tmp/v3.tgz
  echo "coeurs=$(nproc)"; cmake --version | head -1; nvcc --version | tail -2
  cmake -S morsehgp3D_v3 -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --target mhgp3v_directional_dominance_probe \
    mhgp3v_cell_credits_probe mhgp3v_conic_groups_probe mhgp3v_window_source_probe \
    mhgp3v_spindle_cone_probe -j48
' 2>&1 | tee -a "${LOG}"

# La cible CUDA exige un worktree propre cote produit ; ici seule la
# compilation de v3 nous interesse. Un echec est RAPPORTE, pas masque.
"${SSH[@]}" 'set -e
  export PATH=$HOME/.local/bin:/usr/local/cuda/bin:$PATH
  cd ~/residuel
  echo "=== compilation CUDA opt-in ==="
  cmake -S morsehgp3D_v3 -B build-cuda -DCMAKE_BUILD_TYPE=Release \
    -DMHGP3V_ENABLE_CUDA=ON -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc \
    -DCMAKE_CUDA_ARCHITECTURES=120-real 2>&1 | tail -5
  cmake --build build-cuda --target mhgp3v_anchor_device -j48 2>&1 | tail -20 \
    && echo "CUDA_COMPILE=OK" || echo "CUDA_COMPILE=ECHEC"
' 2>&1 | tee -a "${LOG}"

# ---- 6. Portes locales, reconstruites sur la VM. Rejeu independant.
"${SSH[@]}" 'set -e
  export PATH=$HOME/.local/bin:$PATH
  cd ~/residuel
  ctest --test-dir build --output-on-failure -j24 \
    -R "^mhgp3v_(cone|window|dominance|groupes|coeur|credits)_" 2>&1 | tail -25
' 2>&1 | tee -a "${LOG}"

# ---- 7. LA MESURE DECISIVE. Quatre familles en parallele, trois tailles.
#
# Le sujet est mono-thread : les quarante-huit coeurs servent a lancer les
# douze runs simultanement, pas a accelerer un run. Chaque ligne publie le
# residuel `PairId` en valeur ABSOLUE, seule quantite dont la pente decide.
"${SSH[@]}" 'set -e
  cd ~/residuel
  mkdir -p out
  for f in uniform eight_clusters terrain scanline_overlap_multiecho; do
    for n in 12500 25000 50000; do
      ( ./build/mhgp3v_directional_dominance_probe --points=$n --family=$f --seed=3 \
          > out/${f}_${n}.txt 2>&1 ; echo "fini $f $n" ) &
    done
  done
  wait
  echo "=== RESIDUEL PairId, VALEUR ABSOLUE ==="
  for f in uniform eight_clusters terrain scanline_overlap_multiecho; do
    for n in 12500 25000 50000; do
      r=$(grep "^lane q4" out/${f}_${n}.txt | grep -o "PairId_residuel=[0-9]*" | cut -d= -f2)
      c=$(grep "^lane q4" out/${f}_${n}.txt | grep -o "PairId_ferme=[0-9]*" | cut -d= -f2)
      echo "$f n=$n residuel=$r ferme=$c"
    done
  done
' 2>&1 | tee -a "${LOG}"

# ---- 8. Les pentes, calculees sur la VM pour eviter toute retranscription.
"${SSH[@]}" 'set -e
  cd ~/residuel
  python3 - <<PY
import math, re
for f in ["uniform","eight_clusters","terrain","scanline_overlap_multiecho"]:
    vals=[]
    for n in [12500,25000,50000]:
        t=open(f"out/{f}_{n}.txt").read()
        m=re.search(r"^lane q4.*?PairId_residuel=(\d+)", t, re.M)
        vals.append(int(m.group(1)) if m else -1)
    s=[]
    for i in range(1,len(vals)):
        s.append(math.log2(vals[i]/vals[i-1]) if vals[i-1]>0 and vals[i]>0 else float("nan"))
    verdict = "REFUSE" if all(x>=1.35 for x in s) else ("VERT" if all(x<=1.35 for x in s) else "MIXTE")
    print(f"{f:32s} residuels={vals} pentes={[round(x,3) for x in s]} -> {verdict}")
PY
' 2>&1 | tee -a "${LOG}"

echo "SESSION TERMINEE" | tee -a "${LOG}"
