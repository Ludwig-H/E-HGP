#!/usr/bin/env bash
# Session G4 — LE NOYAU D'AXE SUR LE GPU : PARITE ET DEBIT.
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# ---------------------------------------------------------------------------
# CE QU'ELLE MESURE, ET SON PERIMETRE EXACT
#
# Le noyau d'axe divise les candidats par cinquante-neuf sans deplacer le mur de
# temps : la selection balaie encore tous les sites de chaque `Q4Seed3`. Ce
# balayage est donc le terme dominant, et c'est LUI que cette session met sur le
# GPU. Elle mesure deux choses :
#
#   1. la PARITE hote/device, champ par champ, verdict par verdict ;
#   2. le DEBIT du kernel seul, warmup non chronometre.
#
# ELLE NE MESURE NI LA CHAINE COMPLETE, NI UN `warm_e2e`, NI LE CONTRAT. Le lot
# est construit sur l'hote et transfere ; le transfert n'est pas soustrait mais
# n'est pas non plus inclus dans le debit du kernel. Un debit de kernel n'est pas
# un debit de bout en bout.
#
# C'est la PREMIERE execution CUDA de cette ligne. Le premier objectif est donc
# qu'elle compile et qu'elle soit d'accord avec l'hote ; le debit ne vaut que si
# la parite est exacte.
# ---------------------------------------------------------------------------
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
# LA ZONE EST UNE ZONE IA, ET CE N'EST PAS UN DETAIL. `verify_running_guard`
# n'autorise le repli « echeance calculee certifiee » — champ
# `terminationTimestamp` entierement absent — que si la zone correspond a
# `*-ai*`. La premiere version de cette recette visait `europe-west4-a`, une
# zone standard : le champ n'y est pas apparu en douze tentatives de cinq
# secondes et la session a echoue fermé, correctement. On vise donc la paire IA
# autorisee, celle de la recette qui fonctionne.
export GCP_ZONE="${GCP_ZONE:-europe-west4-ai1a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot-ai1a}"
# Coherence des trois durees : GCE 90 min, invite 75 min, run 55 min. Le run le
# plus long doit finir avant l'arret invite, qui doit finir avant l'arret GCE.
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-5400}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-75}"
RUN_TIMEOUT="${RUN_TIMEOUT:-3300}"

WORK="$(mktemp -d /tmp/ehgp-axis-cuda.XXXXXXXX)"
HANDOFF="${WORK}/handoff.json"
LOG="${WORK}/session.log"
RECU="${REPO_ROOT}/morsehgp3D_v3/receipts/axis_cuda_g4_20260815"
echo "session dans ${WORK}"

GIT_HEAD="$(git rev-parse HEAD)"
GIT_DIRTY="$(git status --porcelain | wc -l)"
echo "git_head=${GIT_HEAD} fichiers_modifies=${GIT_DIRTY}" | tee -a "${LOG}"
for f in morsehgp3D_v3/prototype/axis_device_job.hpp \
         morsehgp3D_v3/prototype/axis_device_kernel.cu \
         morsehgp3D_v3/prototype/axis_device_qualification.cpp \
         morsehgp3D_v3/prototype/q4seed_axis_topr4.hpp; do
  echo "sha256 ${f} = $(sha256sum "${f}" | cut -d' ' -f1)" | tee -a "${LOG}"
done

gcloud config set project "${GCP_PROJECT_ID}" >/dev/null

./gcp-migration/set_max_run_duration_and_verify.sh --yes \
  --max-run-duration-seconds "${MAX_RUN_SECONDS}" 2>&1 | tee -a "${LOG}"

export GCP_SSH_KEY_DIR="${WORK}/ssh"
mkdir -p "${GCP_SSH_KEY_DIR}"
chmod 700 "${GCP_SSH_KEY_DIR}"
export GCP_SSH_KEY_FILE="${GCP_SSH_KEY_DIR}/id_ed25519"
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-axis-cuda' -f "${GCP_SSH_KEY_FILE}"
chmod 600 "${GCP_SSH_KEY_FILE}"
GCP_SSH_KEY_EXPIRATION_UTC="$(python3 -c 'from datetime import datetime,timedelta,timezone; print((datetime.now(timezone.utc)+timedelta(minutes=95)).isoformat(timespec="seconds").replace("+00:00","Z"))')"
export GCP_SSH_KEY_EXPIRATION_UTC
gcloud compute os-login ssh-keys add --key-file="${GCP_SSH_KEY_FILE}.pub" --ttl=95m \
  --project="${GCP_PROJECT_ID}" >/dev/null

# ---------------------------------------------------------------------------
# LA FERMETURE CIBLEE, REPAREE.
#
# Contre-audit `AUDIT_CONTRE_SESSION_AXIS_TOP8_G4_840A2E2_20260814.md`, P0
# securite : l'ancienne branche de secours appelait `stop_and_verify.sh --yes`
# SANS `--expected-last-start-timestamp` des que `GENERATION` etait vide. Elle
# pouvait donc arreter une session PREEXISTANTE OU CONCURRENTE sur la cible par
# defaut — exactement ce que la regle imperative interdit.
#
# Trois etats, et un seul autorise l'arret :
#   START_ATTEMPTED=0 -> aucun arret, jamais. Rien n'a ete demarre.
#   START_ATTEMPTED=1 et generation connue -> arret VERSIONNE de cette
#     generation, et d'elle seule.
#   START_ATTEMPTED=1 et generation inconnue -> BLOCAGE. On signale projet,
#     zone, nom, dernier etat connu et la commande de controle. On n'appelle
#     jamais l'arret non versionne.
#
# Le transcript est copie APRES la decision finale : l'ancienne version le
# copiait avant d'ajouter le fait bloquant, donc le recu pouvait l'omettre.
# ---------------------------------------------------------------------------
START_ATTEMPTED=0
GENERATION=""
cleanup() {
  local rc=$?
  echo "--- fermeture ciblee (rc=${rc}) ---" | tee -a "${LOG}"
  local stop_rc=0
  local bloque=0
  if [ "${START_ATTEMPTED}" -eq 0 ]; then
    echo "[OK] aucun demarrage tente : aucun arret n'est appele." | tee -a "${LOG}"
  else
    # La generation est lue et VALIDEE ici, dans le trap, jamais avant.
    if [ -z "${GENERATION}" ] && [ -r "${HANDOFF}" ]; then
      GENERATION="$(python3 -c "
import json,sys
try:
    v = json.load(open('${HANDOFF}'))['last_start_timestamp']
except Exception:
    sys.exit(0)
print(v if isinstance(v, str) and v.strip() else '')
" 2>/dev/null || true)"
    fi
    if [ -n "${GENERATION}" ]; then
      ./gcp-migration/stop_and_verify.sh --yes \
        --expected-last-start-timestamp "${GENERATION}" 2>&1 | tee -a "${LOG}" || stop_rc=$?
    else
      bloque=1
      {
        echo "[BLOCAGE] demarrage tente mais generation INCONNUE :"
        echo "  projet=${GCP_PROJECT_ID} zone=${GCP_ZONE} instance=${GCP_INSTANCE_NAME}"
        echo "  dernier etat connu : voir ci-dessus dans ce journal"
        echo "  aucun arret non versionne n'est appele — il pourrait viser une"
        echo "  session concurrente. Controle manuel :"
        echo "    gcloud compute instances describe ${GCP_INSTANCE_NAME} \\"
        echo "      --zone=${GCP_ZONE} --project=${GCP_PROJECT_ID} \\"
        echo "      --format='value(status,lastStartTimestamp)'"
        echo "    ./gcp-migration/stop_and_verify.sh --yes \\"
        echo "      --expected-last-start-timestamp <horodatage lu ci-dessus>"
      } | tee -a "${LOG}"
    fi
  fi
  mkdir -p "${RECU}"
  cp "${LOG}" "${RECU}/transcript.txt" || { echo "[COPIE RATEE] transcript"; exit 71; }
  echo "journal complet : ${LOG} (copie dans ${RECU})"
  if [ "${bloque}" -ne 0 ]; then exit 72; fi
  if [ "${stop_rc}" -ne 0 ]; then
    echo "[ARRET NON CERTIFIE] stop_and_verify a rendu ${stop_rc} — echec bloquant" | tee -a "${LOG}"
    exit 70
  fi
  exit "${rc}"
}
trap cleanup EXIT

START_ATTEMPTED=1
./gcp-migration/start_and_verify.sh --yes \
  --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}" \
  --handoff-file "${HANDOFF}" 2>&1 | tee -a "${LOG}"

GENERATION="$(python3 -c "import json,sys; print(json.load(open('${HANDOFF}'))['last_start_timestamp'])")"
echo "generation verrouillee : ${GENERATION}" | tee -a "${LOG}"

SSH=(gcloud compute ssh "${GCP_INSTANCE_NAME}" --project="${GCP_PROJECT_ID}"
     --zone="${GCP_ZONE}" --ssh-key-file="${GCP_SSH_KEY_FILE}"
     --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" --quiet --command)

TAR="${WORK}/v3.tgz"
tar czf "${TAR}" --exclude=build --exclude=.git morsehgp3D_v2 morsehgp3D_v3
TAR_SHA="$(sha256sum "${TAR}" | cut -d' ' -f1)"
echo "tar_sha256=${TAR_SHA}" | tee -a "${LOG}"
gcloud compute scp "${TAR}" "${GCP_INSTANCE_NAME}:/tmp/v3.tgz" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
  --ssh-key-file="${GCP_SSH_KEY_FILE}" \
  --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" 2>&1 | tee -a "${LOG}"

# ---- 1. Build CUDA. La toolchain est celle de l'image Deep Learning ; on la
# publie avant tout, parce qu'un debit sans version de nvcc n'est pas une mesure.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:/usr/local/cuda/bin:$PATH
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:/usr/local/cuda/bin:$PATH
  nvidia-smi --query-gpu=name,memory.total,driver_version --format=csv,noheader
  nvcc --version | tail -2
  rm -rf ~/ax && mkdir -p ~/ax && cd ~/ax
  tar xzf /tmp/v3.tgz
  echo "coeurs=$(nproc)"
  cmake -S morsehgp3D_v3 -B build -DCMAKE_BUILD_TYPE=Release \
        -DMHGP3V_ENABLE_CUDA=ON 2>&1 | tail -5
  cmake --build build --target mhgp3v_axis_device -j48 2>&1 | tail -25
  sha256sum build/mhgp3v_axis_device
' 2>&1 | tee -a "${LOG}"

# ---- 2. PARITE ET DEBIT. Trois tailles, deux profils de rang, deux familles.
# Toute sortie non nulle est conservee et fait echouer la session.
"${SSH[@]}" "set -euo pipefail
  export PATH=\$HOME/.local/bin:/usr/local/cuda/bin:\$PATH
  cd ~/ax
  A=./build/mhgp3v_axis_device
  mkdir -p out
  for fam in uniform eight_clusters; do
    for sm in 6 11; do
      for n in 1500 3000 6000; do
        echo \"=== famille=\$fam smax=\$sm n=\$n\"
        set +e
        timeout 900 \$A --family=\$fam --points=\$n --seed=1 --smax=\$sm \\
          --max-seeds=3000000 --max-sites=600000000 2>&1
        echo \"code=\\${PIPESTATUS[0]}\"
        set -e
      done
    done
  done > out/axis_cuda.txt 2>&1
  echo '=== MESURE TERMINEE ==='
  wc -l out/axis_cuda.txt
" 2>&1 | tee -a "${LOG}"

# ---- 3. Recu AVANT verdict : le brut revient meme si le verdict est rouge.
mkdir -p "${RECU}"
gcloud compute scp "${GCP_INSTANCE_NAME}:~/ax/out/axis_cuda.txt" "${RECU}/axis_cuda.txt" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
  --ssh-key-file="${GCP_SSH_KEY_FILE}" \
  --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" 2>&1 | tee -a "${LOG}"

# ---- 4. LE VERDICT : la parite d'abord, le debit ensuite. Un seul ecart
# hote/device rend la session rouge, quel que soit le debit obtenu.
"${SSH[@]}" 'set -euo pipefail
  cd ~/ax
  python3 - <<PY
import re, sys
lignes = [l.rstrip() for l in open("out/axis_cuda.txt")]
codes = [int(l.split("=")[1]) for l in lignes if l.startswith("code=")]
par = [l for l in lignes if "parite :" in l]
ecarts = sum(int(m.group(1)) for l in par for m in [re.search(r"ecarts=(\d+)", l)] if m)
acc = [float(m.group(1)) for l in lignes for m in [re.search(r"acceleration=([0-9.]+)x", l)] if m]
dev = [float(m.group(1)) for l in lignes for m in [re.search(r"device : [0-9.]+ ms, [0-9.]+ Mseeds/s, ([0-9.]+) Msites/s", l)] if m]
mauvais = [c for c in codes if c != 0]
print("runs=%d codes_non_nuls=%d mesures_parite=%d ecarts_total=%d" % (len(codes), len(mauvais), len(par), ecarts))
if acc: print("acceleration min=%.1fx max=%.1fx" % (min(acc), max(acc)))
if dev: print("device Msites/s min=%.1f max=%.1f" % (min(dev), max(dev)))
ok = (not mauvais) and ecarts == 0 and len(par) == len(codes) and len(par) > 0
print("VERDICT=%s" % ("PARITE_EXACTE" if ok else "DESACCORD_OU_INCOMPLET"))
sys.exit(0 if ok else 1)
PY
' 2>&1 | tee -a "${LOG}"

echo "campagne terminee" | tee -a "${LOG}"
