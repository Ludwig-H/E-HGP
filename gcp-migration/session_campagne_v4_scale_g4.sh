#!/usr/bin/env bash
# Session G4 — CAMPAGNE D'ECHELLE v4 : les tailles contractuelles sur les
# deux profils, moteur CPU de reference. VERSION TRANSACTIONNELLE (audit
# « campagne G4 transactionnelle » apres 1a0640).
#
# Cette session utilise EXCLUSIVEMENT les scripts gardes du depot pour le
# demarrage et l'arret. Elle ne cree aucune VM, ne modifie aucune garde et
# certifie TERMINATED sur exactement la generation qu'elle a demarree.
#
# ---------------------------------------------------------------------------
# CE QU'ELLE MESURE, ET CE QU'ELLE NE REVENDIQUE PAS
#
# La v4 n'a pas de cible CUDA : la G4 sert de RESSOURCE CPU (48 vCPU,
# 180 Go) et de materiel contractuel des recus de cout. Aucun debit GPU
# revendique, aucun SLO ; aucun benchmark ne promeut public_status=exact —
# les invariants sont etablis par les portes CTest, REJOUEES sur la VM
# avant toute mesure. Hors regime juge le probe imprime desormais
# `juge=off desaccords=NA` : un zero machine ne passera jamais pour un
# accord verifie.
#
# DEUX CAMPAGNES SEPAREES (audit § 2) :
#   1. LATENCE ISOLEE (timing_scope=isolated_latency) : un seul run a la
#      fois, taskset sur cœur fixe, OMP_NUM_THREADS=1, pic RSS enregistre —
#      uniform smax=11 n=8000 (×2, mediane), 16000, 32000. Ce sont les
#      points comparables aux pentes locales 400/800/1600.
#   2. COUVERTURE CONTENDUE (timing_scope=contended_throughput) : la
#      matrice 4 familles × 3 tailles × 2 profils par VAGUES a concurrence
#      PILOTEE PAR LA MEMOIRE (audit § 3) : la concurrence de chaque taille
#      est min(cap_prudent, floor(0.75·RAM / pic_RSS_mesure_en_phase_1)) —
#      les temps de cette phase ne servent JAMAIS aux pentes de latence.
#
# TRANSACTIONNEL (audit § 1) : chaque run ecrit DEUX fichiers atomiques
# (payload .txt + .status avec code/duree/pic RSS/timing_scope), errexit
# desarme localement autour du run — un timeout, un OOM-kill ou un refus
# n'effacent jamais leur marqueur. Apres rapatriement, une VALIDATION
# LOCALE exige l'ensemble exact des fichiers, code=0 partout, la ligne de
# compteurs presente et aucune ligne REFUS/INVARIANT/PLANCHER/Killed/
# bad_alloc ; sinon le reçu porte campaign_status=partial_or_failed et la
# session sort NON NULLE — apres rapatriement, jamais avant. Le mot
# COMPLETE est reserve au cas valide.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
# Repli exact autorise : GCP_ZONE=europe-west4-ai1a
# GCP_INSTANCE_NAME=ehgp-blackwell-spot-ai1a (voir README).
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-21600}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-350}"

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
GCP_SSH_KEY_EXPIRATION_UTC="$(python3 -c 'from datetime import datetime,timedelta,timezone; print((datetime.now(timezone.utc)+timedelta(minutes=370)).isoformat(timespec="seconds").replace("+00:00","Z"))')"
export GCP_SSH_KEY_EXPIRATION_UTC
gcloud compute os-login ssh-keys add --key-file="${GCP_SSH_KEY_FILE}.pub" --ttl=370m \
  --project="${GCP_PROJECT_ID}" >/dev/null

# ---- 3. Demarrage garde. Le trap d'echec du script appelle deja l'arret.
./gcp-migration/start_and_verify.sh --yes \
  --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}" \
  --handoff-file "${HANDOFF}" 2>&1 | tee -a "${LOG}"

GENERATION="$(python3 -c "import json,sys; print(json.load(open('${HANDOFF}'))['last_start_timestamp'])")"
echo "generation verrouillee : ${GENERATION}" | tee -a "${LOG}"

# ---- Arret certifie quoi qu'il arrive, sur EXACTEMENT cette generation.
SESSION_RC=0
cleanup() {
  local rc=$?
  if [ "${SESSION_RC}" -ne 0 ]; then rc="${SESSION_RC}"; fi
  echo "--- arret certifie (rc=${rc}) ---" | tee -a "${LOG}"
  # FAIL-CLOSED : un arret cible illisible doit BLOQUER, jamais etre avale.
  local stop_rc=0
  ./gcp-migration/stop_and_verify.sh --yes \
    --expected-last-start-timestamp "${GENERATION}" 2>&1 | tee -a "${LOG}" || stop_rc=$?
  echo "journal complet : ${LOG}"
  echo "resultats rapatries : ${WORK}/out (si l'etape scp a ete atteinte)"
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

# ---- 5. Build Release + REJEU INDEPENDANT des portes. Un echec ARRETE la
# session avant toute mesure : pas de campagne sur un build non conforme.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/v4scale && mkdir -p ~/v4scale && cd ~/v4scale
  tar xzf /tmp/v4.tgz
  echo "coeurs=$(nproc)"; grep MemTotal /proc/meminfo; cmake --version | head -1
  git -C '"${REPO_ROOT}"' rev-parse HEAD 2>/dev/null || true
  cmake -S morsehgp3D_v4 -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j48
  ctest --test-dir build --output-on-failure -j24 2>&1 | tail -4
' 2>&1 | tee -a "${LOG}"

# ---- 6. LES DEUX CAMPAGNES, transactionnelles.
"${SSH[@]}" 'set -euo pipefail
  export PATH=$HOME/.local/bin:$PATH
  cd ~/v4scale
  P=./build/mhgp4_forest_probe
  mkdir -p out
  export OMP_NUM_THREADS=1

  # run_one : DEUX fichiers atomiques par run ; errexit desarme autour du
  # run — timeout/OOM/refus n ecrasent jamais leur marqueur (audit § 1).
  run_one() {
    local name="$1" scope="$2" cpu="$3"; shift 3
    local out="out/${name}.txt" status="out/${name}.status"
    local rc=0 t0 t1 hwm=""
    t0=$(date +%s)
    if command -v /usr/bin/time >/dev/null 2>&1; then
      /usr/bin/time -v -o "${status}.time" \
        timeout 10800 taskset -c "${cpu}" "$P" "$@" >"${out}" 2>&1 || rc=$?
      hwm=$(grep -oE "Maximum resident set size[^0-9]*[0-9]+" "${status}.time" | grep -oE "[0-9]+$" || true)
    else
      timeout 10800 taskset -c "${cpu}" "$P" "$@" >"${out}" 2>&1 & local pp=$!
      while kill -0 ${pp} 2>/dev/null; do
        local h; h=$(grep VmHWM /proc/${pp}/status 2>/dev/null | awk "{print \$2}") || true
        [ -n "${h}" ] && hwm=${h}
        sleep 3
      done
      wait ${pp} || rc=$?
    fi
    t1=$(date +%s)
    {
      printf "code=%d\n" "${rc}"
      printf "duree_s=%d\n" "$((t1 - t0))"
      printf "peak_rss_kb=%s\n" "${hwm:-inconnu}"
      printf "timing_scope=%s\n" "${scope}"
      printf "finished=1\n"
    } > "${status}.tmp"
    mv "${status}.tmp" "${status}"
    echo "--- fini ${name} (code=${rc}, $((t1 - t0))s, rss=${hwm:-?}kB)"
    return 0
  }

  # PHASE 1 — LATENCE ISOLEE (un run a la fois, cœur fixe). Les pics RSS
  # mesures ici PILOTENT la concurrence de la phase 2.
  run_one lat_uniform_n8000_smax11_rep1 isolated_latency 2 \
    --family=uniform --n=8000 --s=8 --smax=11 --seed=3 --min-balls=10000 --min-fusions=1000
  run_one lat_uniform_n8000_smax11_rep2 isolated_latency 2 \
    --family=uniform --n=8000 --s=8 --smax=11 --seed=3 --min-balls=10000 --min-fusions=1000
  run_one lat_uniform_n16000_smax11 isolated_latency 2 \
    --family=uniform --n=16000 --s=8 --smax=11 --seed=3 --min-balls=10000 --min-fusions=1000
  run_one lat_uniform_n32000_smax11 isolated_latency 2 \
    --family=uniform --n=32000 --s=8 --smax=11 --seed=3 --min-balls=10000 --min-fusions=1000

  # Concurrence par taille : min(cap prudent, 0.75*RAM/pic) — audit § 3.
  conc_for() {
    local status="$1" cap="$2"
    local kb mem
    kb=$(grep -oE "^peak_rss_kb=[0-9]+" "${status}" | grep -oE "[0-9]+$" || true)
    mem=$(grep MemTotal /proc/meminfo | awk "{print \$2}")
    if [ -z "${kb}" ]; then echo 1; return; fi
    local c=$(( (mem * 3 / 4) / kb ))
    [ "${c}" -lt 1 ] && c=1
    [ "${c}" -gt "${cap}" ] && c=${cap}
    echo "${c}"
  }
  C8=$(conc_for out/lat_uniform_n8000_smax11_rep1.status 8)
  C16=$(conc_for out/lat_uniform_n16000_smax11.status 4)
  C32=$(conc_for out/lat_uniform_n32000_smax11.status 2)
  echo "concurrences pilotees par la memoire : n8000=${C8} n16000=${C16} n32000=${C32}"

  # PHASE 2 — COUVERTURE CONTENDUE, par vagues de taille croissante ; les
  # gros runs ne demarrent jamais tous au meme instant.
  wave() {
    local n="$1" conc="$2"
    local i=0 cpu
    for fam in uniform terrain eight_clusters scanline_overlap_multiecho; do
      for smax in 11 6; do
        cpu=$(( 4 + (i % conc) * 2 ))
        run_one "cov_${fam}_n${n}_smax${smax}" contended_throughput "${cpu}" \
          --family=${fam} --n=${n} --s=8 --smax=${smax} --seed=3 \
          --min-balls=10000 --min-fusions=1000 &
        i=$((i + 1))
        if [ $((i % conc)) -eq 0 ]; then wait; fi
      done
    done
    wait
  }
  wave 8000 "${C8}"
  wave 16000 "${C16}"
  wave 32000 "${C32}"
  echo "=== fin des runs (la validation decide du statut, pas cette ligne) ==="
' 2>&1 | tee -a "${LOG}"

# ---- 7. Rapatriement TOUJOURS, puis VALIDATION LOCALE (audit § 1) : le
# statut de campagne est calcule ici, jamais declare par le lanceur.
mkdir -p "${WORK}/out"
gcloud compute scp --recurse "${GCP_INSTANCE_NAME}:~/v4scale/out" "${WORK}/" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --quiet \
  --ssh-key-file="${GCP_SSH_KEY_FILE}" \
  --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" 2>&1 | tee -a "${LOG}"

python3 - "${WORK}/out" <<'PYEOF' 2>&1 | tee -a "${LOG}" || SESSION_RC=65
import os, re, sys

out = sys.argv[1]
expected = []
for rep in (1, 2):
    expected.append(f"lat_uniform_n8000_smax11_rep{rep}")
expected += ["lat_uniform_n16000_smax11", "lat_uniform_n32000_smax11"]
for fam in ("uniform", "terrain", "eight_clusters", "scanline_overlap_multiecho"):
    for n in (8000, 16000, 32000):
        for smax in (11, 6):
            expected.append(f"cov_{fam}_n{n}_smax{smax}")

forbidden = re.compile(r"REFUS|INVARIANT|PLANCHER|Killed|bad_alloc")
counters = re.compile(r"boules_uniques=\d+.*evenements=\d+.*juge=off desaccords=NA")
bad = []
for name in expected:
    txt = os.path.join(out, name + ".txt")
    status = os.path.join(out, name + ".status")
    if not os.path.exists(status):
        bad.append(f"{name}: .status ABSENT")
        continue
    st = open(status).read()
    if "finished=1" not in st:
        bad.append(f"{name}: status incomplet")
    m = re.search(r"code=(\d+)", st)
    if not m or m.group(1) != "0":
        bad.append(f"{name}: code={m.group(1) if m else '?'}")
    if not os.path.exists(txt):
        bad.append(f"{name}: .txt ABSENT")
        continue
    body = open(txt, errors="replace").read()
    if forbidden.search(body):
        bad.append(f"{name}: motif interdit ({forbidden.search(body).group(0)})")
    if not counters.search(body):
        bad.append(f"{name}: ligne de compteurs absente ou juge ambigu")
for f in sorted(os.listdir(out)):
    if f.endswith(".txt") and f[:-4] not in expected:
        bad.append(f"{f}: fichier inattendu")
if bad:
    print("campaign_status=partial_or_failed")
    for b in bad:
        print("  -", b)
    sys.exit(1)
print(f"campaign_status=complete ({len(expected)} runs valides)")
print("=== CAMPAGNE COMPLETE ===")
PYEOF

echo "session terminee ; l arret certifie est declenche par le trap"
echo "recette : coller ${WORK}/out/*.txt et *.status dans un reçu"
