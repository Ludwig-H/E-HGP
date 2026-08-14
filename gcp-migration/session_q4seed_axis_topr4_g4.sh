#!/usr/bin/env bash
# Session G4 — LA CAMPAGNE QUI TENTE DE REFUTER `Q4SeedAxisExtremalCompletion-r4`.
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# ---------------------------------------------------------------------------
# CE QU'ELLE MESURE, ET POURQUOI ELLE MERITE UNE MACHINE
#
# Le theoreme dit qu'a `Q4Seed3` owner aigu fixe, tout apex q4 de rang sous `r4`
# est parmi au plus `2(r4-p)` racines extremales, et que `I_B/U_B` se
# reconstruisent sans second balayage. C'est une affirmation UNIVERSELLE : elle ne
# se confirme pas, elle se REFUTE. La seule chose utile est donc de la
# confronter a une sweep exhaustive sur le plus de configurations possible.
#
# La sweep coute `O(n^5)` : le poste de developpement a DEUX vCPU et plafonne a
# `n=60`. La machine est employee ici comme ressource CPU a 48 coeurs.
#
# LA MATRICE EST GOUVERNEE PAR UN BUDGET, PAS PAR UNE LISTE. Le contre-audit
# `AUDIT_CONTRE_SESSION_AXIS_TOP8_G4_840A2E2_20260814.md` a montre que
# soixante-seize runs sequentiels ne tiennent dans aucune enveloppe : la somme
# des timeouts autorises depassait soixante-dix heures. On mesure donc une
# RAMPE causale — chaque taille n'est ouverte que si le debit observe prouve
# qu'elle finit avant le coupe-circuit — et on s'arrete au premier palier rouge.
# AUCUN kernel CUDA n'est execute et aucun debit GPU n'est mesure.
#
# ELLE NE REVENDIQUE NI SLO, NI DEBIT, NI STATUT PUBLIC. Un `manquants=0` sur
# toute la matrice ne prouve pas le theoreme — la preuve est dans l'audit ; il
# refute seulement les erreurs d'implementation que le raisonnement seul laisse
# passer, et j'en ai deja trouve une (le bout FERME de `J_f`).
#
# ---------------------------------------------------------------------------
# CE QUI FAIT ECHOUER LA SESSION
#
#   code 1  un apex shallow hors selection, un census faux, une borne cassee
#   code 3  un plancher de couverture viole (vert par vacuite)
#   code 4  un mutant survivant
# Toute sortie non nulle d'une phase est CONSERVEE et la session echoue ; elle
# n'est jamais filtree ni resumee avant archivage.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
# Coherence des trois durees : GCE 90 min, invite 75 min, run 55 min. Le run le
# plus long doit finir avant l'arret invite, qui doit finir avant l'arret GCE.
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-5400}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-75}"
RUN_TIMEOUT="${RUN_TIMEOUT:-3300}"

WORK="$(mktemp -d /tmp/ehgp-q4axis-session.XXXXXXXX)"
HANDOFF="${WORK}/handoff.json"
LOG="${WORK}/session.log"
RECU="${REPO_ROOT}/morsehgp3D_v3/receipts/q4seed_axis_topr4_g4_20260814"
echo "session dans ${WORK}"

GIT_HEAD="$(git rev-parse HEAD)"
GIT_DIRTY="$(git status --porcelain | wc -l)"
echo "git_head=${GIT_HEAD} fichiers_modifies=${GIT_DIRTY}" | tee -a "${LOG}"
for f in morsehgp3D_v3/prototype/q4seed_axis_topr4.hpp \
         morsehgp3D_v3/prototype/q4seed_axis_topr4_probe.cpp \
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
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-q4axis-session' -f "${GCP_SSH_KEY_FILE}"
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
  rm -rf ~/q4 && mkdir -p ~/q4 && cd ~/q4
  tar xzf /tmp/v3.tgz
  echo "coeurs=$(nproc)"; cmake --version | head -1; g++ --version | head -1
  cmake -S morsehgp3D_v3 -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
  cmake --build build --target mhgp3v_q4seed_axis_topr4_probe -j48
  sha256sum build/mhgp3v_q4seed_axis_topr4_probe
' 2>&1 | tee -a "${LOG}"

# ---- 2. Rejeu INDEPENDANT des vingt-trois portes sur la VM. Les fixtures et
# les mutants doivent repasser sur une autre machine, un autre compilateur et un
# autre ordonnancement avant que la campagne ait le moindre sens.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/q4
  ctest --test-dir build --output-on-failure -j24 -R "^mhgp3v_q4axis" 2>&1 | tail -6
' 2>&1 | tee -a "${LOG}"

# ---- 3. LA RAMPE BUDGETEE. Chaque palier mesure son propre temps ; le palier
# suivant n'est ouvert que si le temps CUMULE plus le cout EXTRAPOLE du palier
# suivant tiennent dans le budget restant. L'extrapolation emploie l'exposant
# cinq declare par le probe. Un palier rouge arrete la rampe, il ne la saute pas.
"${SSH[@]}" "set -euo pipefail
  export PATH=\$HOME/.local/bin:\$PATH
  cd ~/q4
  A=./build/mhgp3v_q4seed_axis_topr4_probe
  mkdir -p out
  BUDGET=${RUN_TIMEOUT}
  DEBUT=\$(date +%s)
  FAM='uniform eight_clusters terrain scanline_single_pass scanline_overlap_multiecho two_lines'
  precedent=0
  for n in 80 120 160 220 300; do
    reste=\$(( BUDGET - (\$(date +%s) - DEBUT) ))
    if [ \"\$precedent\" -gt 0 ]; then
      # cout extrapole = precedent * (n/n_prec)^5, en arithmetique entiere
      prev_n=\$prev_n_val
      est=\$(( precedent * n * n * n * n * n / (prev_n * prev_n * prev_n * prev_n * prev_n) ))
      if [ \"\$est\" -gt \"\$(( reste * 8 / 10 ))\" ]; then
        echo \"=== PALIER n=\$n NON OUVERT : cout extrapole \${est}s > 80% du reste \${reste}s\"
        break
      fi
    fi
    t0=\$(date +%s)
    for f in \$FAM; do for g in 1 2 3; do
      echo \"=== n=\$n famille=\$f seed=\$g smax=11\"
      set +e
      timeout \$reste \$A --sweep --family=\$f --points=\$n --seed=\$g \\
        --threads=48 --smax=11 --min-seeds=0 --min-shallow=0 2>&1
      echo \"code=\\${PIPESTATUS[0]}\"
      set -e
    done; done
    precedent=\$(( \$(date +%s) - t0 ))
    prev_n_val=\$n
    echo \"=== PALIER n=\$n TERMINE en \${precedent}s\"
  done > out/rampe.txt 2>&1
  # L'exact-once global, borne a 200 points, sur les trois familles denses.
  for f in uniform eight_clusters terrain; do for n in 60 90 120; do
    echo \"=== exact-once famille=\$f n=\$n\"
    set +e
    timeout 900 \$A --exact-once --family=\$f --points=\$n --seed=1 \\
      --min-q4=1 --min-q3-morts=0 2>&1
    echo \"code=\\${PIPESTATUS[0]}\"
    set -e
  done; done > out/exact_once.txt 2>&1
  echo '=== RAMPE TERMINEE ==='
  wc -l out/*.txt
" 2>&1 | tee -a "${LOG}"

# ---- 4. LE VERDICT, EN ARITHMETIQUE ENTIERE ET SUR LA SORTIE BRUTE.
#
# La campagne n'est verte que si TOUS les codes sont nuls, si `manquants`,
# `bornes_cassees`, `census_faux` et `debordes` sont nuls PARTOUT, et si la
# couverture est non vide : au moins une famille doit produire des apex shallow,
# sinon le vert est vacuite. `two_lines` est la seule exception attendue a zero.
"${SSH[@]}" 'set -euo pipefail
  cd ~/q4
  python3 - <<PY
import re, sys, glob
lignes = []
for f in sorted(glob.glob("out/*.txt")):
    lignes += [(f, l.rstrip()) for l in open(f)]
codes = [int(l.split("=")[1]) for _, l in lignes if l.startswith("code=")]
juges = [l for _, l in lignes if l.startswith("q4seed_axis_topr4")]
def somme(cle):
    return sum(int(m.group(1)) for l in juges for m in [re.search(cle + r"=(\d+)", l)] if m)
mauvais = [c for c in codes if c != 0]
tot_shallow = somme("shallow")
tot_aigues = somme("aigues_owner")
print("runs=%d codes_non_nuls=%d juges=%d" % (len(codes), len(mauvais), len(juges)))
print("aigues_owner=%d shallow=%d manquants=%d bornes_cassees=%d census_faux=%d debordes=%d"
      % (tot_aigues, tot_shallow, somme("manquants"), somme("bornes_cassees"),
         somme("census_faux"), somme("debordes")))
vides = [l for l in juges if "aigues_owner=0 " in l and "two_lines" not in l]
print("familles_vides_hors_two_lines=%d" % len(vides))
ok = (not mauvais and somme("manquants") == 0 and somme("bornes_cassees") == 0
      and somme("census_faux") == 0 and somme("debordes") == 0
      and tot_shallow > 10000 and not vides and len(juges) == len(codes))
print("VERDICT=%s" % ("ACCORD" if ok else "REFUTATION_OU_PLANCHER"))
sys.exit(0 if ok else 1)
PY
' 2>&1 | tee -a "${LOG}"

# ---- 5. Reçu : la sortie brute revient, entiere.
mkdir -p "${RECU}"
for p in rampe exact_once; do
  gcloud compute scp "${GCP_INSTANCE_NAME}:~/q4/out/${p}.txt" "${RECU}/${p}.txt" \
    --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
    --ssh-key-file="${GCP_SSH_KEY_FILE}" \
    --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" 2>&1 | tee -a "${LOG}"
done
echo "campagne terminee" | tee -a "${LOG}"
