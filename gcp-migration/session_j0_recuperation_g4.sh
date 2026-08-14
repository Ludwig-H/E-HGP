#!/usr/bin/env bash
# Session G4 — RECUPERATION du brut J0 reste sur le disque.
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# ---------------------------------------------------------------------------
# CE QU'ELLE MESURE, ET POURQUOI ELLE MERITE UNE MACHINE
#
# Un seul chiffre gouverne la faisabilite du contrat : combien de supports
# l'objet contient-il a 12 500, 25 000 et 50 000 points, sur les DEUX familles
# obligatoires, et aux DEUX profils de rang de l'echelle de repli — `smax=11`
# pour `K=10`, `smax=6` pour `K=5` ?
#
# Localement, sur deux vCPU, `eight_clusters` a `n=1000` coute deja 280 s. La
# machine est employee ici comme ressource CPU a 48 coeurs. AUCUN kernel CUDA
# n'est execute et aucun debit GPU n'est mesure.
#
# ---------------------------------------------------------------------------
# LA MATRICE EST UNE RAMPE BUDGETEE, PAS UNE LISTE
#
# Le contre-audit de la recette precedente a montre que des runs sequentiels
# sans enveloppe ne tiennent dans aucun coupe-circuit. Chaque palier mesure donc
# son temps, et le suivant n'est ouvert que si le cout EXTRAPOLE tient dans
# quatre cinquiemes du budget restant. Les pistes sont ordonnees de la moins
# chere a la plus chere, pour maximiser l'information acquise avant epuisement.
#
# ---------------------------------------------------------------------------
# CE QU'ELLE NE REVENDIQUE PAS
#
# La coupure `--dmax-espacements` n'est PAS le rayon certifie par calottes. La
# sonde publie le diametre reellement atteint et refuse a `0,75 dmax`. Ce qui
# sort d'ici est un LEDGER DE CANDIDATS sous coupure declaree, jamais un nombre
# de supports certifie, jamais un debit, jamais un SLO.
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
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-1800}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-20}"
RUN_TIMEOUT="${RUN_TIMEOUT:-300}"

WORK="$(mktemp -d /tmp/ehgp-j0-recup.XXXXXXXX)"
HANDOFF="${WORK}/handoff.json"
LOG="${WORK}/session.log"
RECU="${REPO_ROOT}/morsehgp3D_v3/receipts/j0_lane_source_g4_20260814"
echo "session dans ${WORK}"

GIT_HEAD="$(git rev-parse HEAD)"
GIT_DIRTY="$(git status --porcelain | wc -l)"
echo "git_head=${GIT_HEAD} fichiers_modifies=${GIT_DIRTY}" | tee -a "${LOG}"
for f in morsehgp3D_v3/prototype/lane_source_scale_probe.cpp \
         morsehgp3D_v3/prototype/q4seed_axis_topr4.hpp \
         morsehgp3D_v3/CMakeLists.txt; do
  echo "sha256 ${f} = $(sha256sum "${f}" | cut -d' ' -f1)" | tee -a "${LOG}"
done

gcloud config set project "${GCP_PROJECT_ID}" >/dev/null

./gcp-migration/set_max_run_duration_and_verify.sh --yes \
  --max-run-duration-seconds "${MAX_RUN_SECONDS}" 2>&1 | tee -a "${LOG}"

export GCP_SSH_KEY_DIR="${WORK}/ssh"
mkdir -p "${GCP_SSH_KEY_DIR}"
chmod 700 "${GCP_SSH_KEY_DIR}"
export GCP_SSH_KEY_FILE="${GCP_SSH_KEY_DIR}/id_ed25519"
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-j0-recup' -f "${GCP_SSH_KEY_FILE}"
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

# ---- 1. Le brut de la rampe precedente est reste sur le disque de demarrage,
# qui persiste a l'arret. On ne recalcule RIEN : on rapatrie, puis on s'arrete.
"${SSH[@]}" 'set -euo pipefail
  ls -la ~/j0/out/ 2>/dev/null || { echo "AUCUN BRUT SUR LE DISQUE"; exit 1; }
  wc -l ~/j0/out/*.txt
' 2>&1 | tee -a "${LOG}"

mkdir -p "${RECU}"
gcloud compute scp "${GCP_INSTANCE_NAME}:~/j0/out/rampe_j0.txt" "${RECU}/rampe_j0.txt" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
  --ssh-key-file="${GCP_SSH_KEY_FILE}" \
  --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" 2>&1 | tee -a "${LOG}"
wc -l "${RECU}/rampe_j0.txt" | tee -a "${LOG}"
echo "recuperation terminee" | tee -a "${LOG}"
