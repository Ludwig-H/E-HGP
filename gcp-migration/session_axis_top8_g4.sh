#!/usr/bin/env bash
# Session G4 — LA CAMPAGNE QUI TENTE DE REFUTER `FaceAxisExtremalCompletion-8`.
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# ---------------------------------------------------------------------------
# CE QU'ELLE MESURE, ET POURQUOI ELLE MERITE UNE MACHINE
#
# Le theoreme dit qu'a face aigue owner fixee, tout apex q4 de rang au plus `s`
# est parmi au plus `2(s+1)-2p` racines extremales, et que le census se
# reconstruit sans second balayage. C'est une affirmation UNIVERSELLE : elle ne
# se confirme pas, elle se REFUTE. La seule chose utile est donc de la
# confronter a une sweep exhaustive sur le plus de configurations possible.
#
# La sweep coute `O(n^5)` : le poste de developpement a DEUX vCPU et plafonne a
# `n=60`. La machine est employee ici comme ressource CPU a 48 coeurs pour
# monter a `n=300` sur SIX familles, plusieurs graines et quatre seuils.
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

WORK="$(mktemp -d /tmp/ehgp-axis8-session.XXXXXXXX)"
HANDOFF="${WORK}/handoff.json"
LOG="${WORK}/session.log"
RECU="${REPO_ROOT}/morsehgp3D_v3/receipts/axis_top8_g4_20260814"
echo "session dans ${WORK}"

GIT_HEAD="$(git rev-parse HEAD)"
GIT_DIRTY="$(git status --porcelain | wc -l)"
echo "git_head=${GIT_HEAD} fichiers_modifies=${GIT_DIRTY}" | tee -a "${LOG}"
for f in morsehgp3D_v3/prototype/axis_top8.hpp \
         morsehgp3D_v3/prototype/axis_top8_probe.cpp \
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
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-axis8-session' -f "${GCP_SSH_KEY_FILE}"
chmod 600 "${GCP_SSH_KEY_FILE}"
GCP_SSH_KEY_EXPIRATION_UTC="$(python3 -c 'from datetime import datetime,timedelta,timezone; print((datetime.now(timezone.utc)+timedelta(minutes=95)).isoformat(timespec="seconds").replace("+00:00","Z"))')"
export GCP_SSH_KEY_EXPIRATION_UTC
gcloud compute os-login ssh-keys add --key-file="${GCP_SSH_KEY_FILE}.pub" --ttl=95m \
  --project="${GCP_PROJECT_ID}" >/dev/null

# Le trap est arme AVANT le demarrage et lit la generation DANS le trap : un
# handoff illisible ne peut pas laisser une VM allumee.
GENERATION=""
cleanup() {
  local rc=$?
  echo "--- arret certifie (rc=${rc}) ---" | tee -a "${LOG}"
  local stop_rc=0
  if [ -n "${GENERATION}" ]; then
    ./gcp-migration/stop_and_verify.sh --yes \
      --expected-last-start-timestamp "${GENERATION}" 2>&1 | tee -a "${LOG}" || stop_rc=$?
  else
    ./gcp-migration/stop_and_verify.sh --yes 2>&1 | tee -a "${LOG}" || stop_rc=$?
  fi
  mkdir -p "${RECU}"
  cp "${LOG}" "${RECU}/transcript.txt" || { echo "[COPIE RATEE] transcript"; exit 71; }
  echo "journal complet : ${LOG} (copie dans ${RECU})"
  if [ "${stop_rc}" -ne 0 ]; then
    echo "[ARRET NON CERTIFIE] stop_and_verify a rendu ${stop_rc} — echec bloquant" | tee -a "${LOG}"
    exit 70
  fi
  exit "${rc}"
}
trap cleanup EXIT

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
  rm -rf ~/a8 && mkdir -p ~/a8 && cd ~/a8
  tar xzf /tmp/v3.tgz
  echo "coeurs=$(nproc)"; cmake --version | head -1; g++ --version | head -1
  cmake -S morsehgp3D_v3 -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
  cmake --build build --target mhgp3v_axis_top8_probe -j48
  sha256sum build/mhgp3v_axis_top8_probe
' 2>&1 | tee -a "${LOG}"

# ---- 2. Rejeu INDEPENDANT des vingt-trois portes sur la VM. Les fixtures et
# les mutants doivent repasser sur une autre machine, un autre compilateur et un
# autre ordonnancement avant que la campagne ait le moindre sens.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/a8
  ctest --test-dir build --output-on-failure -j24 -R "^mhgp3v_axis_top8" 2>&1 | tail -6
' 2>&1 | tee -a "${LOG}"

# ---- 3. LA CAMPAGNE. Six familles, trois tailles, plusieurs graines, quatre
# seuils. La sortie brute est conservee EN ENTIER ; aucun filtre n'intervient
# avant l'archivage.
"${SSH[@]}" "set -euo pipefail
  export PATH=\$HOME/.local/bin:\$PATH
  cd ~/a8
  A=./build/mhgp3v_axis_top8_probe
  mkdir -p out
  FAM='uniform eight_clusters terrain scanline_single_pass scanline_overlap_multiecho two_lines'
  # Phase A : diversite maximale a petite taille, cinq graines.
  for f in \$FAM; do for g in 1 2 3 4 5; do
    echo \"=== A famille=\$f n=120 seed=\$g seuil=7\"
    set +e
    timeout ${RUN_TIMEOUT} \$A --sweep --family=\$f --points=120 --seed=\$g \\
      --threads=48 --seuil=7 --min-faces=0 --min-shallow=0 2>&1
    echo \"code=\${PIPESTATUS[0]}\"
    set -e
  done; done > out/phaseA.txt 2>&1
  # Phase B : trois graines a n=200.
  for f in \$FAM; do for g in 1 2 3; do
    echo \"=== B famille=\$f n=200 seed=\$g seuil=7\"
    set +e
    timeout ${RUN_TIMEOUT} \$A --sweep --family=\$f --points=200 --seed=\$g \\
      --threads=48 --seuil=7 --min-faces=0 --min-shallow=0 2>&1
    echo \"code=\${PIPESTATUS[0]}\"
    set -e
  done; done > out/phaseB.txt 2>&1
  # Phase C : deux graines a n=300, la plus grande taille exhaustive tenable.
  for f in \$FAM; do for g in 1 2; do
    echo \"=== C famille=\$f n=300 seed=\$g seuil=7\"
    set +e
    timeout ${RUN_TIMEOUT} \$A --sweep --family=\$f --points=300 --seed=\$g \\
      --threads=48 --seuil=7 --min-faces=0 --min-shallow=0 2>&1
    echo \"code=\${PIPESTATUS[0]}\"
    set -e
  done; done > out/phaseC.txt 2>&1
  # Phase D : la borne doit suivre 2(seuil+1)-2p, pas seulement seize.
  for f in uniform eight_clusters terrain scanline_single_pass; do for s in 3 5 9 11; do
    echo \"=== D famille=\$f n=200 seed=1 seuil=\$s\"
    set +e
    timeout ${RUN_TIMEOUT} \$A --sweep --family=\$f --points=200 --seed=1 \\
      --threads=48 --seuil=\$s --min-faces=0 --min-shallow=0 2>&1
    echo \"code=\${PIPESTATUS[0]}\"
    set -e
  done; done > out/phaseD.txt 2>&1
  echo '=== CAMPAGNE TERMINEE ==='
  wc -l out/phase*.txt
" 2>&1 | tee -a "${LOG}"

# ---- 4. LE VERDICT, EN ARITHMETIQUE ENTIERE ET SUR LA SORTIE BRUTE.
#
# La campagne n'est verte que si TOUS les codes sont nuls, si `manquants`,
# `bornes_cassees`, `census_faux` et `debordes` sont nuls PARTOUT, et si la
# couverture est non vide : au moins une famille doit produire des apex shallow,
# sinon le vert est vacuite. `two_lines` est la seule exception attendue a zero.
"${SSH[@]}" 'set -euo pipefail
  cd ~/a8
  python3 - <<PY
import re, sys, glob
lignes = []
for f in sorted(glob.glob("out/phase*.txt")):
    lignes += [(f, l.rstrip()) for l in open(f)]
codes = [int(l.split("=")[1]) for _, l in lignes if l.startswith("code=")]
juges = [l for _, l in lignes if l.startswith("axis_top8_juge")]
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
      and tot_shallow > 100000 and not vides and len(juges) == len(codes))
print("VERDICT=%s" % ("ACCORD" if ok else "REFUTATION_OU_PLANCHER"))
sys.exit(0 if ok else 1)
PY
' 2>&1 | tee -a "${LOG}"

# ---- 5. Reçu : la sortie brute revient, entiere.
mkdir -p "${RECU}"
for p in phaseA phaseB phaseC phaseD; do
  gcloud compute scp "${GCP_INSTANCE_NAME}:~/a8/out/${p}.txt" "${RECU}/${p}.txt" \
    --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
    --ssh-key-file="${GCP_SSH_KEY_FILE}" \
    --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" 2>&1 | tee -a "${LOG}"
done
echo "campagne terminee" | tee -a "${LOG}"
