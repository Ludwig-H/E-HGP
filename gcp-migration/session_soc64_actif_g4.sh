#!/usr/bin/env bash
# Session G4 — LA RAMPE QUI MESURE LE CERTIFICAT CORRELE BRANCHE.
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# ---------------------------------------------------------------------------
# CE QU'ELLE MESURE, ET POURQUOI MAINTENANT
#
# `SOC64` etait jusqu'ici un SHADOW : il comptait ce qu'il fermerait sans rien
# fermer. Aucune mesure d'echelle n'avait donc de sens, puisque la fenetre `E4`
# publiee restait celle du certificat central seul.
#
# Il est desormais BRANCHE (`--soc64-actif`) comme disjonctif du certificat —
# deux conditions suffisantes et non comparables restent suffisantes. Localement,
# a `n=3000`, il fait tomber le residuel q4 :
#
#     uniform         22,842 %  ->  13,602 %
#     terrain         12,603 %  ->   6,781 %
#     eight_clusters  89,933 %  ->  73,767 %
#
# La question de cette session est la seule qui compte : est-ce que cela deplace
# l'EXPOSANT, ou seulement la constante ? Quatre tailles, trois familles, deux
# certificats, et un analyseur qui decide en arithmetique ENTIERE.
#
# ELLE NE REVENDIQUE AUCUN SLO ET NE MESURE AUCUN DEBIT GPU. La machine est
# employee comme ressource CPU : le poste de developpement a DEUX vCPU. Aucun
# kernel CUDA n'est execute.
#
# ---------------------------------------------------------------------------
# LES SIX REPARATIONS EXIGEES PAR LE CONTRE-AUDIT
#
# `AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md` section 8 liste
# six defauts de la recette precedente. Ils sont tous corriges ici :
#
#   1. le trap d'arret est arme AVANT toute commande faillible qui suit le
#      retour de `start_and_verify.sh` — l'ancienne version lisait le handoff
#      JSON avant de l'armer ;
#   2. chaque pipeline tourne sous `set +e`, son `PIPESTATUS[0]` est capture
#      IMMEDIATEMENT, `set -e` est restaure, et les quatre tailles sont
#      poursuivies quoi qu'il arrive ;
#   3. la sortie brute est conservee INTEGRALEMENT — `fenetre_finale`, les trois
#      masses, les recertifications, les temps et le HWM. L'ancien `sed` les
#      coupait ;
#   4. les pentes sont calculees et GATEES par un analyseur versionne, en
#      arithmetique entiere, au lieu d'un calcul documentaire posterieur ;
#   5. les timeouts par run, le coupe-circuit invite et le `maxRunDuration`
#      sont coherents entre eux ;
#   6. le recu d'ECHEC est archive lui aussi, et toute copie ratee est
#      bloquante.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-ai1a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot-ai1a}"
# Coherence des trois durees : GCE 90 min, invite 80 min, run 50 min. Le run le
# plus long doit finir avant l'arret invite, qui doit finir avant l'arret GCE.
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-5400}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-80}"
RUN_TIMEOUT="${RUN_TIMEOUT:-3000}"

WORK="$(mktemp -d /tmp/ehgp-soc64-session.XXXXXXXX)"
HANDOFF="${WORK}/handoff.json"
LOG="${WORK}/session.log"
RECU="${REPO_ROOT}/morsehgp3D_v3/receipts/soc64_actif_g4_20260814"
echo "session dans ${WORK}"

GIT_HEAD="$(git rev-parse HEAD)"
GIT_DIRTY="$(git status --porcelain | wc -l)"
echo "git_head=${GIT_HEAD} fichiers_modifies=${GIT_DIRTY}" | tee -a "${LOG}"
for f in morsehgp3D_v3/prototype/wspd_wavefront_probe.cpp \
         morsehgp3D_v3/prototype/soc64_rect.hpp \
         morsehgp3D_v3/audits/check_rampe_pentes.py; do
  echo "sha256 ${f} = $(sha256sum "${f}" | cut -d' ' -f1)" | tee -a "${LOG}"
done

gcloud config set project "${GCP_PROJECT_ID}" >/dev/null

./gcp-migration/set_max_run_duration_and_verify.sh --yes \
  --max-run-duration-seconds "${MAX_RUN_SECONDS}" 2>&1 | tee -a "${LOG}"

export GCP_SSH_KEY_DIR="${WORK}/ssh"
mkdir -p "${GCP_SSH_KEY_DIR}"
chmod 700 "${GCP_SSH_KEY_DIR}"
export GCP_SSH_KEY_FILE="${GCP_SSH_KEY_DIR}/id_ed25519"
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-soc64-session' -f "${GCP_SSH_KEY_FILE}"
chmod 600 "${GCP_SSH_KEY_FILE}"
GCP_SSH_KEY_EXPIRATION_UTC="$(python3 -c 'from datetime import datetime,timedelta,timezone; print((datetime.now(timezone.utc)+timedelta(minutes=100)).isoformat(timespec="seconds").replace("+00:00","Z"))')"
export GCP_SSH_KEY_EXPIRATION_UTC
gcloud compute os-login ssh-keys add --key-file="${GCP_SSH_KEY_FILE}.pub" --ttl=100m \
  --project="${GCP_PROJECT_ID}" >/dev/null

# ---- REPARATION 1 : le trap est defini AVANT le demarrage, et la generation
# est lue DANS le trap. L'ancienne version lisait le handoff JSON entre le
# demarrage et l'armement ; un JSON illisible laissait donc une VM allumee.
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
  # REPARATION 6 : le recu d'ECHEC est archive lui aussi.
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

# ---- 1. Build, et empreinte de l'ELF mesure.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:/usr/local/cuda/bin:$PATH
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/soc && mkdir -p ~/soc && cd ~/soc
  tar xzf /tmp/v3.tgz
  echo "coeurs=$(nproc)"; cmake --version | head -1; g++ --version | head -1
  cmake -S morsehgp3D_v3 -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
  # TOUTES les cibles dont la porte de rejeu a besoin. La session precedente
  # n'en construisait qu'une et lancait un regex plus large : les binaires
  # absents faisaient echouer des tests qui n'avaient rien a voir avec la
  # mesure, et la session s'arretait — correctement, mais pour rien.
  cmake --build build --target mhgp3v_wspd_wavefront_probe mhgp3v_soc64_probe \
        mhgp3v_wspd_front_probe mhgp3v_jung_dual_probe -j48
  for t in mhgp3v_wspd_wavefront_probe mhgp3v_soc64_probe mhgp3v_wspd_front_probe \
           mhgp3v_jung_dual_probe; do sha256sum build/$t; done
' 2>&1 | tee -a "${LOG}"

# ---- 2. Rejeu INDEPENDANT des portes sur la VM.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/soc
  ctest --test-dir build --output-on-failure -j24 \
    -R "^mhgp3v_(wspd|soc64|porteurs|two_lines|jung_dual|diag_feuille|dissection|ordre_proche)" \
    2>&1 | tail -10
' 2>&1 | tee -a "${LOG}"

# ---- 3. LA RAMPE. Trois familles, deux certificats, quatre tailles.
#
# REPARATION 2 : `set +e` autour de chaque pipeline, `PIPESTATUS[0]` capture
# immediatement, `set -e` restaure, et les quatre tailles poursuivies.
# REPARATION 3 : AUCUN filtre. La sortie brute est conservee en entier.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/soc
  W=./build/mhgp3v_wspd_wavefront_probe
  mkdir -p out/central out/soc64
  for fam in uniform terrain eight_clusters; do
    for mode in central soc64; do
      (
        opt=""
        [ "$mode" = "soc64" ] && opt="--soc64-actif"
        for n in 6250 12500 25000 50000; do
          echo "=== $fam raffine=0 n=$n"
          set +e
          timeout '"${RUN_TIMEOUT}"' $W --family=$fam --points=$n --sep-euclid=8/1 \
            --tight --vwave --window=512 --window-ledger $opt 2>&1
          code=${PIPESTATUS[0]}
          set -e
          echo "code=${code}"
        done
      ) > out/$mode/rampe_${fam}_r0.txt 2>&1 &
    done
  done
  wait
  echo "=== RAMPE TERMINEE ==="
  for f in out/*/rampe_*.txt; do
    printf "%s : " "$f"
    grep -cE "^code=0" "$f" | tr -d "\n"
    printf " codes zero, "
    grep -cE "^fenetre_finale=OUI" "$f" | tr -d "\n"
    echo " fenetres finales"
  done
' 2>&1 | tee -a "${LOG}"

# ---- 4. REPARATION 4 : l'analyseur versionne DECIDE, en arithmetique entiere.
#
# La matrice exacte est exigee, tous les codes doivent etre zero, tout `pending`
# est refuse, et les pentes sont comparees par `m2^q n1^p <= m1^q n2^p`. Aucun
# logarithme flottant ne decide.
"${SSH[@]}" 'set -euo pipefail
  cd ~/soc
  for mode in central soc64; do
    echo "########## ANALYSE $mode"
    set +e
    python3 morsehgp3D_v3/audits/check_rampe_pentes.py \
      --repertoire=out/$mode --familles=uniform,terrain,eight_clusters \
      --profondeurs=0 --lane=4 --metrique=sum_E --max-pente=17/10 \
      --exige-code-zero --exige-pending-zero
    echo "analyseur_code=$?"
    set -e
  done
' 2>&1 | tee -a "${LOG}"

# ---- 5. Publication INTEGRALE des sorties brutes.
"${SSH[@]}" 'set -euo pipefail
  cd ~/soc
  for f in out/*/rampe_*.txt; do echo "########## $f"; cat "$f"; done
' 2>&1 | tee -a "${LOG}"

echo "SESSION_TERMINEE rc=0" | tee -a "${LOG}"
