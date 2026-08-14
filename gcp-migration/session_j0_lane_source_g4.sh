#!/usr/bin/env bash
# Session G4 — J0 : LA TAILLE DE L'OBJET AUX ECHELLES DU CONTRAT.
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
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-5400}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-75}"
RUN_TIMEOUT="${RUN_TIMEOUT:-3300}"

WORK="$(mktemp -d /tmp/ehgp-j0-session.XXXXXXXX)"
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
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-j0-session' -f "${GCP_SSH_KEY_FILE}"
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

# ---- 1. Build, et empreinte de l'ELF reellement mesure.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/j0 && mkdir -p ~/j0 && cd ~/j0
  tar xzf /tmp/v3.tgz
  echo "coeurs=$(nproc)"; cmake --version | head -1; g++ --version | head -1
  cmake -S morsehgp3D_v3 -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
  cmake --build build --target mhgp3v_lane_source_scale_probe \\
        mhgp3v_q4seed_axis_topr4_probe -j48
  sha256sum build/mhgp3v_lane_source_scale_probe build/mhgp3v_q4seed_axis_topr4_probe
' 2>&1 | tee -a "${LOG}"

# ---- 2. Rejeu INDEPENDANT des vingt-trois portes sur la VM. Les fixtures et
# les mutants doivent repasser sur une autre machine, un autre compilateur et un
# autre ordonnancement avant que la campagne ait le moindre sens.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/j0
  ctest --test-dir build --output-on-failure -j24 -R "^mhgp3v_(q4axis|lane_source)" 2>&1 | tail -6
' 2>&1 | tee -a "${LOG}"

# ---- 3. LA RAMPE J0. Six pistes ordonnees du moins cher au plus cher, trois
# tailles chacune, avec ouverture conditionnelle du palier suivant.
"${SSH[@]}" "set -euo pipefail
  export PATH=\$HOME/.local/bin:\$PATH
  cd ~/j0
  L=./build/mhgp3v_lane_source_scale_probe
  mkdir -p out
  BUDGET=${RUN_TIMEOUT}
  DEBUT=\$(date +%s)
  # famille:smax:espacements — du moins cher au plus cher.
  PISTES='terrain:6:10 terrain:11:10 uniform:6:10 uniform:11:10 eight_clusters:6:14 eight_clusters:11:14'
  for piste in \$PISTES; do
    fam=\$(echo \$piste | cut -d: -f1)
    sm=\$(echo \$piste | cut -d: -f2)
    esp=\$(echo \$piste | cut -d: -f3)
    prec=0; prevn=0
    for n in 12500 25000 50000; do
      reste=\$(( BUDGET - (\$(date +%s) - DEBUT) ))
      if [ \"\$reste\" -lt 60 ]; then echo \"=== BUDGET EPUISE avant \$fam smax=\$sm n=\$n\"; break; fi
      if [ \"\$prec\" -gt 0 ]; then
        est=\$(( prec * n * n / (prevn * prevn) ))
        if [ \"\$est\" -gt \"\$(( reste * 8 / 10 ))\" ]; then
          echo \"=== PALIER NON OUVERT \$fam smax=\$sm n=\$n : estime \${est}s > 80% de \${reste}s\"
          break
        fi
      fi
      echo \"=== famille=\$fam smax=\$sm n=\$n espacements=\$esp\"
      t0=\$(date +%s)
      set +e
      timeout \$reste \$L --family=\$fam --points=\$n --seed=1 --smax=\$sm \\
        --threads=48 --dmax-espacements=\$esp 2>&1
      code=\${PIPESTATUS[0]}
      set -e
      dt=\$(( \$(date +%s) - t0 ))
      echo \"code=\$code secondes=\$dt\"
      if [ \"\$code\" -ne 0 ]; then echo \"=== PISTE \$fam smax=\$sm ARRETEE au code \$code\"; break; fi
      prec=\$dt; prevn=\$n
    done
  done > out/rampe_j0.txt 2>&1
  echo '=== RAMPE J0 TERMINEE ==='
  grep -c '^code=0' out/rampe_j0.txt || true
  wc -l out/rampe_j0.txt
" 2>&1 | tee -a "${LOG}"

# ---- 4. LE VERDICT. J0 ne juge pas une performance : il juge que la mesure
# est LISIBLE. Tout code non nul, toute coupure qui mord, ou moins de deux
# tailles par famille obligatoire rendent la session rouge.
"${SSH[@]}" 'set -euo pipefail
  cd ~/j0
  python3 - <<PY
import re, sys
lignes = [l.rstrip() for l in open("out/rampe_j0.txt")]
codes = [int(l.split("=")[1].split()[0]) for l in lignes if l.startswith("code=")]
mesures = [l for l in lignes if l.startswith("lane_source :")]
coupures = [float(m.group(1)) for l in lignes for m in [re.search(r"rapport=([0-9.]+)", l)] if m]
par_famille = {}
for l in lignes:
    m = re.search(r"famille=(\w+) smax=(\d+) n=(\d+)", l)
    if m and l.startswith("==="):
        par_famille.setdefault((m.group(1), m.group(2)), []).append(int(m.group(3)))
obligatoires = [k for k in par_famille if k[0] in ("uniform", "eight_clusters")]
mauvais = [c for c in codes if c != 0]
pire_coupure = max(coupures) if coupures else 0.0
assez = all(len(par_famille[k]) >= 2 for k in obligatoires) and len(obligatoires) >= 4
print("runs=%d codes_non_nuls=%d mesures=%d pire_coupure=%.3f pistes_obligatoires=%d"
      % (len(codes), len(mauvais), len(mesures), pire_coupure, len(obligatoires)))
for k in sorted(par_famille):
    print("  %s smax=%s tailles=%s" % (k[0], k[1], par_famille[k]))
ok = (not mauvais) and pire_coupure <= 0.75 and assez and len(mesures) == len(codes)
print("VERDICT=%s" % ("LISIBLE" if ok else "INCOMPLET_OU_TRONQUE"))
sys.exit(0 if ok else 1)
PY
' 2>&1 | tee -a "${LOG}"

# ---- 5. Reçu : la sortie brute revient, entiere.
mkdir -p "${RECU}"
for p in rampe_j0; do
  gcloud compute scp "${GCP_INSTANCE_NAME}:~/j0/out/${p}.txt" "${RECU}/${p}.txt" \
    --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
    --ssh-key-file="${GCP_SSH_KEY_FILE}" \
    --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" 2>&1 | tee -a "${LOG}"
done
echo "campagne terminee" | tee -a "${LOG}"
