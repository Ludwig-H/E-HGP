#!/usr/bin/env bash
# Session G4 — LA RAMPE QUI DECIDE L'ETAPE 1 : `sum E_4` sous raffinement local.
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# ---------------------------------------------------------------------------
# CE QU'ELLE MESURE
#
# Le levier anisotrope est REFUTE sur rectangle : son gain mesure est de trois
# centiemes de point, parce que `Tabs` est le MINIMUM de `|T|` sur le bloc
# alors que le gain par paire vient des sites ou `|T|` est MAXIMAL. Le seul
# levier qui reste est le RAFFINEMENT LOCAL, et il paie : a `n=3 000`,
# `eight_clusters` passe de `89,93 %` a `57,75 %` de masse q4 ouverte pour un
# front multiplie par `3,3`.
#
# La question que cette session tranche est donc :
#
#     le raffinement local rend-il la pente de `sum E_4` inferieure a `1,35`
#     sur les CINQ familles, et a quel prix sur le front ?
#
# C'est la porte de l'etape 1 de la note de route. Une pente rouge REFUTE la
# route du certificat central pour les nuages en amas et impose de passer aux
# cages et fleurs.
#
# ELLE NE REVENDIQUE AUCUN SLO ET NE MESURE AUCUN DEBIT GPU. La machine est
# employee comme ressource CPU : le poste de developpement a DEUX vCPU, et la
# rampe complete y est infaisable. Aucun kernel n'est execute.
#
# ---------------------------------------------------------------------------
# FAIL-CLOSED, PARTOUT
#
# Le contre-audit `736f5bc` §10 a montre que la session precedente masquait
# trois familles sur quatre : `set -e` tuait le sous-shell AVANT le `echo
# code=$?`, puis `wait || true` effacait le statut. Ici chaque famille ecrit son
# code de retour par run, aucun `|| true` n'avale un statut, et la session
# REFUSE si une famille n'a pas publie ses quatre codes.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-ai1a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot-ai1a}"
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-3600}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-45}"

WORK="$(mktemp -d /tmp/ehgp-fenetre-session.XXXXXXXX)"
HANDOFF="${WORK}/handoff.json"
LOG="${WORK}/session.log"
echo "session dans ${WORK}"

# PROVENANCE PINCEE AVANT TOUT. Un recu sans commit ni empreinte n'est pas un
# recu — c'est le reproche que l'audit fait a la session precedente.
GIT_HEAD="$(git rev-parse HEAD)"
GIT_DIRTY="$(git status --porcelain | wc -l)"
echo "git_head=${GIT_HEAD} fichiers_modifies=${GIT_DIRTY}" | tee -a "${LOG}"

gcloud config set project "${GCP_PROJECT_ID}" >/dev/null

./gcp-migration/set_max_run_duration_and_verify.sh --yes \
  --max-run-duration-seconds "${MAX_RUN_SECONDS}" 2>&1 | tee -a "${LOG}"

export GCP_SSH_KEY_DIR="${WORK}/ssh"
mkdir -p "${GCP_SSH_KEY_DIR}"
chmod 700 "${GCP_SSH_KEY_DIR}"
export GCP_SSH_KEY_FILE="${GCP_SSH_KEY_DIR}/id_ed25519"
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-fenetre-session' -f "${GCP_SSH_KEY_FILE}"
chmod 600 "${GCP_SSH_KEY_FILE}"
GCP_SSH_KEY_EXPIRATION_UTC="$(python3 -c 'from datetime import datetime,timedelta,timezone; print((datetime.now(timezone.utc)+timedelta(minutes=70)).isoformat(timespec="seconds").replace("+00:00","Z"))')"
export GCP_SSH_KEY_EXPIRATION_UTC
gcloud compute os-login ssh-keys add --key-file="${GCP_SSH_KEY_FILE}.pub" --ttl=70m \
  --project="${GCP_PROJECT_ID}" >/dev/null

./gcp-migration/start_and_verify.sh --yes \
  --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}" \
  --handoff-file "${HANDOFF}" 2>&1 | tee -a "${LOG}"

GENERATION="$(python3 -c "import json,sys; print(json.load(open('${HANDOFF}'))['last_start_timestamp'])")"
echo "generation verrouillee : ${GENERATION}" | tee -a "${LOG}"

cleanup() {
  local rc=$?
  echo "--- arret certifie (rc=${rc}) ---" | tee -a "${LOG}"
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
  rm -rf ~/fen && mkdir -p ~/fen && cd ~/fen
  tar xzf /tmp/v3.tgz
  echo "coeurs=$(nproc)"; cmake --version | head -1; g++ --version | head -1
  cmake -S morsehgp3D_v3 -B build -DCMAKE_BUILD_TYPE=Release >/dev/null
  # LA CIBLE DE LA RAMPE EST BLOQUANTE ; la tranche verticale est AUXILIAIRE et
  # son echec est RAPPORTE sans tuer la mesure. Le compilateur de la VM est plus
  # strict que celui du poste — il a deja refuse un tableau non initialise que
  # `gcc 13` acceptait — et cela ne doit pas couter une session.
  cmake --build build --target mhgp3v_wspd_wavefront_probe -j48
  sha256sum build/mhgp3v_wspd_wavefront_probe
  if cmake --build build --target mhgp3v_ball_event_probe -j48 > /tmp/be.log 2>&1; then
    echo "BALL_EVENT_BUILD=OK"; sha256sum build/mhgp3v_ball_event_probe
  else
    echo "BALL_EVENT_BUILD=ECHEC"; tail -20 /tmp/be.log
  fi
' 2>&1 | tee -a "${LOG}"

# ---- 2. Rejeu INDEPENDANT de toutes les portes, sur la VM.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/fen
  ctest --test-dir build --output-on-failure -j24 \
    -R "^mhgp3v_wspd_wavefront" 2>&1 | tail -12
  ctest --test-dir build --output-on-failure -j24 -R "^mhgp3v_ball_event" 2>&1 | tail -6 \
    || echo "BALL_EVENT_CTEST=ECHEC (auxiliaire, non bloquant)"
' 2>&1 | tee -a "${LOG}"

# ---- 3. LA RAMPE. Cinq familles, deux profondeurs de raffinement, quatre
# tailles. Chaque run ecrit SON code ; aucun statut n'est avale.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/fen
  W=./build/mhgp3v_wspd_wavefront_probe
  mkdir -p out
  for fam in uniform terrain eight_clusters scanline_single_pass scanline_overlap_multiecho; do
    for raf in 0 4; do
      (
        # PAS de `set -e` ici : un run qui echoue doit ECRIRE son code, pas
        # tuer le sous-shell avant de l ecrire. C est la faute exacte que le
        # contre-audit `736f5bc` a relevee.
        opt=""
        [ "$raf" -gt 0 ] && opt="--raffine=$raf"
        for n in 6250 12500 25000 50000; do
          echo "=== $fam raffine=$raf n=$n"
          # `$?` APRES UN PIPELINE rend le statut du DERNIER maillon, donc de
          # `sed`, jamais celui de la sonde. C est exactement la faute qui a
          # masque trois familles dans la session precedente.
          timeout 1500 $W --family=$fam --points=$n --sep-euclid=8/1 --tight \
            --vwave --window=512 --window-ledger --max-slope=9 $opt 2>&1 | \
            grep -E "^n=|^fenetre q|^pente|^OK|REFUS" | sed "s/ | arbre.*//"
          echo "code=${PIPESTATUS[0]}"
        done
      ) > out/rampe_${fam}_r${raf}.txt 2>&1 &
    done
  done
  wait
  echo "=== RAMPE TERMINEE ==="
  for f in out/rampe_*.txt; do echo "--- $f"; grep -cE "^code=" "$f" | sed "s/^/codes_publies=/"; done
' 2>&1 | tee -a "${LOG}"

# ---- 4. Publication complete, et REFUS si une famille n'a pas ses quatre codes.
"${SSH[@]}" 'set -euo pipefail
  cd ~/fen
  for f in out/rampe_*.txt; do echo "########## $f"; cat "$f"; done
  echo "=== CONTROLE DE COMPLETUDE ==="
  manque=0
  for f in out/rampe_*.txt; do
    c=$(grep -cE "^code=" "$f" || true)
    [ "$c" -eq 4 ] || { echo "INCOMPLET: $f publie $c codes sur 4"; manque=1; }
  done
  [ "$manque" -eq 0 ] || { echo "REFUS: rampe incomplete"; exit 3; }
  echo "COMPLETUDE=OK"
' 2>&1 | tee -a "${LOG}"

echo "SESSION_TERMINEE rc=0" | tee -a "${LOG}"
cp "${LOG}" "${REPO_ROOT}/morsehgp3D_v3/receipts/session_fenetre_g4.log" || true
