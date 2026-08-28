#!/usr/bin/env bash
# Script DISTANT de la campagne v5 — execute sur la VM par la session gardee
# (session_campagne_v5_scale_g4.sh) ou localement par le selftest
# transactionnel (selftest_campagne_v5.sh, faux pilote). Il ne demarre ni
# n'arrete rien : la session locale detient les gardes.
#   $1 = source_commit, $2 = source_payload_sha256, $3 = protocol_manifest_sha256,
#   graves dans CHAQUE .status (le reçu sait quel code et quel protocole
#   exacts ont produit ses compteurs).
#
# TRANSACTIONNEL : chaque run ecrit deux fichiers atomiques (.txt = sortie du
# pilote, .status = code / duree / pic RSS par GNU time / portee / pin),
# errexit desarme autour du run. GNU time OBLIGATOIRE (jamais un repli qui
# mesurerait le wrapper).
#
# DEUX PHASES :
#   1. CONFORMITE v4 ≡ v5 aux tailles d'interet (8000, 16000, 32000, quatre
#      familles) par `mhgp5_conformity_v4` contre le reçu calcule par la v4
#      (bundle) — chaque run doit imprimer `balls=egal all=egal` ; un echec
#      ARRETE la campagne (exit 3, statuts conserves) : pas de mesure a 50 k
#      sur un moteur non conforme ;
#   2. CONTRAT 50 000 POINTS (mesure, jamais un claim) : `mhgp5 --digest` a
#      n=50000 sur les quatre familles, THREADS fils (defaut nproc), digests
#      et RSS graves — ces digests deviennent la reference v5 a 50 k.
#
# PHASE OPTIONNELLE SCALE_THREADS (P0 de l'audit « rendement GPU et
# multi-CPU » du 28 aout 2026) : activee par SCALE_THREADS="1 2 4 8 16 24 32 48"
# (liste de fils). Pour chaque (famille, fils, inflight, digest) le binaire CPU
# est execute `--threads=<fils> --fold-inflight=<inflight> [--digest]`, en ordre
# CONTREBALANCE (liste des fils dans l'ordre donne aux repetitions impaires,
# inversee aux repetitions paires : schema ABBA par bloc (famille, inflight,
# digest)). Le plan complet est ANNONCE avant le premier run
# (scale_threads_plan.txt), la topologie gravee une fois (topologie.txt),
# chaque run grave .txt, .status (commande, fils, inflight, digest, repetition,
# sequence, pin) et la sortie complete de GNU time (.status.time). Le
# validateur exige la presence de tous les runs annonces, code 0, memes digests
# (digest=1) et memes compteurs de travail par famille ; il ne conclut JAMAIS
# sur une acceleration.
set -euo pipefail

SOURCE_COMMIT="${1:?source_commit requis}"
SOURCE_PAYLOAD_SHA256="${2:?source_payload_sha256 requis}"
PROTOCOL_MANIFEST_SHA256="${3:?protocol_manifest_sha256 requis}"
PROBE_BIN="${PROBE_BIN:-./build/mhgp5}"
CONFORMITY_BIN="${CONFORMITY_BIN:-./build/mhgp5_conformity_v4}"
RECEIPT="${RECEIPT:-morsehgp3D_v5/receipts/conformite_v4/digests_v4.txt}"
OUT_DIR="${OUT_DIR:-out}"
RUN_TIMEOUT="${RUN_TIMEOUT:-7200}"
TIME_BIN="${TIME_BIN:-/usr/bin/time}"
THREADS="${THREADS:-$(nproc)}"
FAMILIES="${FAMILIES:-uniform terrain eight_clusters scanline_single_pass}"
# Parametres de la phase SCALE_THREADS (vide = phase non executee).
SCALE_THREADS="${SCALE_THREADS:-}"
SCALE_FAMILIES="${SCALE_FAMILIES:-eight_clusters scanline_single_pass}"
SCALE_N="${SCALE_N:-50000}"
SCALE_INFLIGHT="${SCALE_INFLIGHT:-1 2 3}"
SCALE_DIGEST="${SCALE_DIGEST:-0 1}"
SCALE_REPEATS="${SCALE_REPEATS:-2}"
SCALE_RUN_TIMEOUT="${SCALE_RUN_TIMEOUT:-${RUN_TIMEOUT}}"

test -x "${TIME_BIN}" || {
  echo "REFUS : GNU time requis (${TIME_BIN}) pour une campagne a RSS mesure" >&2
  exit 2
}
test -f "${RECEIPT}" || { echo "REFUS : reçu de conformite absent (${RECEIPT})" >&2; exit 2; }
# REFUS AVANT TOUT RUN d'un parametre SCALE_* mal forme : une liste vide ou
# non entiere ne doit pas se decouvrir apres des heures de conformite.
if [ -n "${SCALE_THREADS}" ]; then
  scale_refuse() { echo "REFUS : parametre SCALE_THREADS — $1" >&2; exit 2; }
  for t in ${SCALE_THREADS}; do
    [[ "${t}" =~ ^[1-9][0-9]*$ ]] || scale_refuse "fils '${t}' non entier >= 1"
  done
  for i in ${SCALE_INFLIGHT}; do
    [[ "${i}" =~ ^[1-9][0-9]*$ ]] || scale_refuse "inflight '${i}' non entier >= 1 (le domaine 0 n'est pas une mesure)"
  done
  for d in ${SCALE_DIGEST}; do
    [ "${d}" = "0" ] || [ "${d}" = "1" ] || scale_refuse "digest '${d}' hors {0,1}"
  done
  for f in ${SCALE_FAMILIES}; do
    [[ "${f}" =~ ^[a-z][a-z0-9_]*$ ]] || scale_refuse "famille '${f}' mal formee"
  done
  [[ "${SCALE_N}" =~ ^[1-9][0-9]*$ ]] || scale_refuse "n '${SCALE_N}' non entier"
  [[ "${SCALE_REPEATS}" =~ ^[1-9][0-9]*$ ]] || scale_refuse "repeats '${SCALE_REPEATS}' non entier >= 1"
  [[ "${SCALE_RUN_TIMEOUT}" =~ ^[1-9][0-9]*$ ]] || scale_refuse "timeout '${SCALE_RUN_TIMEOUT}' non entier"
  [ -n "${SCALE_FAMILIES// /}" ] || scale_refuse "liste de familles vide"
  [ -n "${SCALE_INFLIGHT// /}" ] || scale_refuse "liste inflight vide"
  [ -n "${SCALE_DIGEST// /}" ] || scale_refuse "liste digest vide"
  # Axes dupliques (un plan a doublons n'est pas une repetition contrebalancee) et familles inconnues (audit du 28 aout).
  for axis in SCALE_THREADS SCALE_INFLIGHT SCALE_DIGEST SCALE_FAMILIES; do
    vals="$(printf '%s\n' ${!axis} | sort)"
    [ "$(printf '%s\n' "${vals}" | uniq -d | wc -l)" -eq 0 ] || scale_refuse "axe ${axis} avec doublon"
  done
  for f in ${SCALE_FAMILIES}; do
    case "${f}" in uniform|terrain|eight_clusters|scanline_single_pass|scanline_overlap_multiecho) ;; *) scale_refuse "famille inconnue '${f}'" ;; esac
  done
  # Echeance globale (epoch, transmise par la session avec sa marge de rapatriement) : la phase s'arrete AVANT un run
  # qui ne pourrait pas finir avant l'echeance, et au premier run a code non nul ; la troncature est gravee et le
  # validateur la juge partielle.
  if [ -n "${SCALE_DEADLINE_EPOCH:-}" ]; then
    [[ "${SCALE_DEADLINE_EPOCH}" =~ ^[1-9][0-9]*$ ]] || scale_refuse "echeance '${SCALE_DEADLINE_EPOCH}' non entiere"
  fi
fi
mkdir -p "${OUT_DIR}"

# run_one NAME SCOPE CMD... : un run transactionnel. RUN_THREADS (defaut
# THREADS) est le nombre de fils grave ; EXTRA_STATUS (lignes cle=valeur,
# vide par defaut) est ajoute au statut ; RUN_TIMEOUT_ONE (defaut RUN_TIMEOUT)
# borne ce run. La commande exacte est gravee (commande=).
run_one() {
  local name="$1" scope="$2"; shift 2
  local out="${OUT_DIR}/${name}.txt" status="${OUT_DIR}/${name}.status"
  local rc=0 t0 t1 hwm=""
  t0=$(date +%s)
  "${TIME_BIN}" -v -o "${status}.time" timeout "${RUN_TIMEOUT_ONE:-${RUN_TIMEOUT}}" "$@" >"${out}" 2>&1 </dev/null || rc=$?
  t1=$(date +%s)
  hwm=$(grep -oE 'Maximum resident set size[^0-9]*[0-9]+' "${status}.time" 2>/dev/null | grep -oE '[0-9]+$' || true)
  {
    printf 'code=%d\n' "${rc}"
    printf 'duree_s=%d\n' "$((t1 - t0))"
    printf 'peak_rss_kb=%s\n' "${hwm:-inconnu}"
    printf 'timing_scope=%s\n' "${scope}"
    printf 'threads=%s\n' "${RUN_THREADS:-${THREADS}}"
    if [ -n "${EXTRA_STATUS:-}" ]; then printf '%s\n' "${EXTRA_STATUS}"; fi
    printf 'commande=%s\n' "$*"
    printf 'source_commit=%s\n' "${SOURCE_COMMIT}"
    printf 'source_payload_sha256=%s\n' "${SOURCE_PAYLOAD_SHA256}"
    printf 'protocol_manifest_sha256=%s\n' "${PROTOCOL_MANIFEST_SHA256}"
    printf 'finished=1\n'
  } > "${status}.tmp"
  mv "${status}.tmp" "${status}"
  echo "--- fini ${name} (code=${rc}, $((t1 - t0))s, rss=${hwm:-?}kB)"
  return 0
}

# PHASE 0 — TEMOIN DEVICE (docs/GPU.md, livraison 3) : build nvcc separe et
# execution du temoin ; son statut est grave comme un run (code, duree, RSS).
# NVCC_BIN vide => statut code=2 « nvcc absent » (jamais un vert de
# complaisance) ; le validateur exige code=0 sur ce run.
NVCC_BIN="${NVCC_BIN:-$(command -v nvcc 2>/dev/null || ls /usr/local/cuda*/bin/nvcc 2>/dev/null | head -1 || true)}"
if [ -n "${NVCC_BIN}" ] && [ "${SKIP_GPU_WITNESS:-0}" != "1" ]; then
  export PATH="$(dirname "${NVCC_BIN}"):${PATH}"
  run_one gpu_witness device_witness bash -c "set -e; echo nvcc=${NVCC_BIN}; uname -m; ${NVCC_BIN} --version 2>&1 | tail -2; nvidia-smi --query-gpu=name,driver_version --format=csv,noheader; cmake -S morsehgp3D_v5 -B build-cuda -DCMAKE_BUILD_TYPE=Release -DMHGP5_ENABLE_CUDA=ON -DCMAKE_CUDA_COMPILER=${NVCC_BIN} 2>&1 | tail -40; test \${PIPESTATUS[0]} -eq 0; cmake --build build-cuda --target mhgp5_device_witness -j8 2>&1 | grep -vE '^\[|^Scanning' | tail -60; test \${PIPESTATUS[0]} -eq 0; ./build-cuda/mhgp5_device_witness"
  # Refus IMMEDIAT des phases suivantes si le temoin n'est pas conforme
  # (P1 audit 9762daaf) : aucune conformite ni contrat sur une VM dont le
  # device n'est pas prouve ; le statut du temoin reste grave.
  if ! grep -q '^code=0$' "${OUT_DIR}/gpu_witness.status"; then
    echo "REFUS : temoin device non conforme (voir gpu_witness.txt) — phases 1 et 2 refusees"
    exit 3
  fi
  # Lane q3 DEVICE contre la production (docs/GPU.md, livraison 4) : trois
  # portes (uniform 1200, eight_clusters 1200 a 4 fils, uniform 8000 a 8 fils),
  # statut grave comme un run ; un echec ne refuse PAS les phases CPU (la lane
  # device n'est pas sur le chemin produit), mais le validateur l'exige a 0.
  run_one gpu_lane device_lane bash -c "set -e; cmake --build build-cuda --target mhgp5_q3_lane_device_gate mhgp5_q4_lane_device_gate mhgp5_cuda -j8 2>&1 | grep -vE '^\[|^Scanning' | tail -60; test \${PIPESTATUS[0]} -eq 0; ./build-cuda/mhgp5_q3_lane_device_gate --family=uniform --n=1200; ./build-cuda/mhgp5_q3_lane_device_gate --family=eight_clusters --n=1200 --threads=4; ./build-cuda/mhgp5_q3_lane_device_gate --family=uniform --n=8000 --threads=8 --min-candidates=100000; ./build-cuda/mhgp5_q4_lane_device_gate --family=uniform --n=1200; ./build-cuda/mhgp5_q4_lane_device_gate --family=eight_clusters --n=1200 --threads=4; ./build-cuda/mhgp5_q4_lane_device_gate --family=uniform --n=8000 --threads=8 --min-candidates=100000; ./build-cuda/mhgp5_q3_lane_device_gate --family=uniform --n=300 --coord=40 --min-candidates=200; ./build-cuda/mhgp5_q4_lane_device_gate --family=uniform --n=300 --coord=40 --min-candidates=200 --min-deep=20"
  # Mutant du temoin sur le DEVICE : code 4 exige (la seule inscription CTest
  # ne prouve pas qu'il a ete compile et tue sur la cible).
  run_one gpu_mutant device_mutant ./build-cuda/mhgp5_device_witness --inject=witness-no-warp-correction
  GPU_BIN="${GPU_BIN:-./build-cuda/mhgp5_cuda}"
else
  {
    printf 'code=2\nduree_s=0\npeak_rss_kb=0\ntiming_scope=device_witness\nthreads=%s\n' "${THREADS}"
    printf 'source_commit=%s\nsource_payload_sha256=%s\nprotocol_manifest_sha256=%s\nfinished=1\n' "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}"
  } > "${OUT_DIR}/gpu_witness.status"
  echo "REFUS : nvcc absent (temoin device non execute)" > "${OUT_DIR}/gpu_witness.txt"
  echo "REFUS : nvcc absent — phases 1 et 2 refusees"
  exit 3
fi

# PHASE 1 — conformite v4 aux tailles d'interet, un run a la fois.
for n in 8000 16000 32000; do
  for fam in ${FAMILIES}; do
    run_one "conf_${fam}_n${n}" conformity \
      "${CONFORMITY_BIN}" "--receipt=${RECEIPT}" "--family=${fam}" "--n=${n}" "--threads=${THREADS}"
  done
done
for n in 8000 16000 32000; do
  for fam in ${FAMILIES}; do
    st="${OUT_DIR}/conf_${fam}_n${n}.status"; tx="${OUT_DIR}/conf_${fam}_n${n}.txt"
    if ! grep -q '^code=0$' "${st}" || ! grep -q 'balls=egal all=egal' "${tx}"; then
      echo "CONFORMITE NON ETABLIE : conf_${fam}_n${n} — contrat 50 k refuse, statuts conserves" >&2
      exit 3
    fi
  done
done

# PHASE 2 — contrat 50 000 points (mesure), un run a la fois, tous les fils.
for fam in ${FAMILIES}; do
  run_one "contrat_${fam}_n50000" contract_50k \
    "${PROBE_BIN}" "--family=${fam}" --n=50000 --s=8 --smax=11 --seed=3 "--threads=${THREADS}" --digest
done
# PHASE 2bis (optionnelle, EXTRA_N="100000 200000") — contrats CPU aux tailles
# d'extension (dizaines/centaines de milliers de points), mesure seulement :
# le validateur exige code 0, compteurs et digest sur chaque run present, sans
# les rendre obligatoires.
for N in ${EXTRA_N:-}; do
  for fam in ${EXTRA_FAMILIES:-uniform eight_clusters}; do
    run_one "contrat_${fam}_n${N}" contract_extra \
      "${PROBE_BIN}" "--family=${fam}" "--n=${N}" --s=8 --smax=11 --seed=3 "--threads=${THREADS}" --digest
  done
done

# PHASE 3 — contrats 50 000 points sur le DEVICE (mhgp5_cuda --gpu, lanes q3
# et q4 device) : memes familles, memes options ; le validateur exige
# digest_all IDENTIQUE au contrat CPU de la meme famille (egalite de bout en
# bout a 50 k) et grave les temps (mesure, jamais un claim).
# PRECONDITIONS de phase : lane device a code 0 et mutant du temoin tue (code 4).
if [ -x "${GPU_BIN:-/nonexistent}" ] && grep -q '^code=0$' "${OUT_DIR}/gpu_lane.status" 2>/dev/null && grep -q '^code=4$' "${OUT_DIR}/gpu_mutant.status" 2>/dev/null; then
  for fam in ${FAMILIES}; do
    run_one "contrat_gpu_${fam}_n50000" contract_50k_gpu \
      "${GPU_BIN}" --gpu "--family=${fam}" --n=50000 --s=8 --smax=11 --seed=3 "--threads=${THREADS}" --digest
  done
  # Executeur ADAPTATIF (ancres de covers >= 256 sites au device, le reste a
  # l'hote) sur les deux familles denses : meme digest exige.
  for fam in eight_clusters scanline_single_pass; do
    run_one "contrat_gpuad_${fam}_n50000" contract_50k_gpu_adaptive \
      "${GPU_BIN}" --gpu --gpu-min-sites=256 "--family=${fam}" --n=50000 --s=8 --smax=11 --seed=3 "--threads=${THREADS}" --digest
  done
else
  echo "REFUS : pilote CUDA absent (${GPU_BIN:-}) ou lane device / mutant non conformes — contrats device non executes"
fi

# PHASE 4 (optionnelle, SCALE_THREADS non vide) — campagne SCALE_THREADS
# CONTREBALANCEE : threads x fold_inflight x digest, repetee, moteur CPU
# seulement (le pilote produit, jamais --gpu). Mesure, jamais un claim :
# aucune acceleration n'est conclue ici ni par le validateur.
if [ -n "${SCALE_THREADS}" ]; then
  # Liste des fils dans l'ordre donne, puis inversee (repetitions paires).
  SCALE_THREADS_FWD="$(printf '%s\n' ${SCALE_THREADS} | tr '\n' ' ' | sed 's/ $//')"
  SCALE_THREADS_REV="$(printf '%s\n' ${SCALE_THREADS} | tac | tr '\n' ' ' | sed 's/ $//')"
  # TOPOLOGIE, une fois par campagne : coeurs, lscpu, affinite du runner,
  # memoire, noyau. Le processus mesure herite de cette affinite.
  {
    echo "nproc=$(nproc)"
    echo "affinite_runner=$(taskset -pc $$ 2>/dev/null | sed 's/.*: //' || echo inconnue)"
    echo "date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "uname=$(uname -srm)"
    grep -E '^(MemTotal|MemAvailable)' /proc/meminfo 2>/dev/null || true
    echo "--- lscpu ---"
    lscpu 2>/dev/null || echo "lscpu indisponible"
    echo "--- affinite (taskset -pc) ---"
    taskset -pc $$ 2>/dev/null || echo "taskset indisponible"
  } > "${OUT_DIR}/topologie.txt.tmp"
  mv "${OUT_DIR}/topologie.txt.tmp" "${OUT_DIR}/topologie.txt"
  # PLAN ANNONCE avant le premier run : parametres et sequence exacte. Le
  # validateur recalcule la sequence contrebalancee depuis les parametres et
  # exige un statut par run annonce (jamais un sous-ensemble).
  SCALE_PLAN="${OUT_DIR}/scale_threads_plan.txt"
  {
    echo "scale_threads_plan=v1"
    echo "threads_list=${SCALE_THREADS_FWD}"
    echo "families=$(printf '%s\n' ${SCALE_FAMILIES} | tr '\n' ' ' | sed 's/ $//')"
    echo "n=${SCALE_N}"
    echo "inflight_list=$(printf '%s\n' ${SCALE_INFLIGHT} | tr '\n' ' ' | sed 's/ $//')"
    echo "digest_list=$(printf '%s\n' ${SCALE_DIGEST} | tr '\n' ' ' | sed 's/ $//')"
    echo "repeats=${SCALE_REPEATS}"
    echo "run_timeout_s=${SCALE_RUN_TIMEOUT}"
    echo "s=8 smax=11 seed=3"
    seq_no=0
    for r in $(seq 1 "${SCALE_REPEATS}"); do
      if [ $((r % 2)) -eq 1 ]; then order="${SCALE_THREADS_FWD}"; else order="${SCALE_THREADS_REV}"; fi
      for fam in ${SCALE_FAMILIES}; do
        for infl in ${SCALE_INFLIGHT}; do
          for dig in ${SCALE_DIGEST}; do
            for t in ${order}; do
              seq_no=$((seq_no + 1))
              echo "seq=${seq_no} name=scale_${fam}_n${SCALE_N}_t${t}_f${infl}_d${dig}_r${r} family=${fam} threads=${t} inflight=${infl} digest=${dig} repeat=${r}"
            done
          done
        done
      done
    done
    echo "runs=${seq_no}"
  } > "${SCALE_PLAN}.tmp"
  mv "${SCALE_PLAN}.tmp" "${SCALE_PLAN}"
  SCALE_RUNS="$(sed -n 's/^runs=//p' "${SCALE_PLAN}")"
  echo "=== SCALE_THREADS : ${SCALE_RUNS} runs annonces (budget maximal $((SCALE_RUNS * SCALE_RUN_TIMEOUT)) s au timeout ${SCALE_RUN_TIMEOUT} s/run) ==="
  while read -r line; do
    case "${line}" in seq=*) ;; *) continue ;; esac
    name="$(printf '%s\n' "${line}" | sed 's/.* name=\([^ ]*\).*/\1/')"
    fam="$(printf '%s\n' "${line}" | sed 's/.* family=\([^ ]*\).*/\1/')"
    t="$(printf '%s\n' "${line}" | sed 's/.* threads=\([^ ]*\).*/\1/')"
    infl="$(printf '%s\n' "${line}" | sed 's/.* inflight=\([^ ]*\).*/\1/')"
    dig="$(printf '%s\n' "${line}" | sed 's/.* digest=\([^ ]*\).*/\1/')"
    r="$(printf '%s\n' "${line}" | sed 's/.* repeat=\([^ ]*\).*/\1/')"
    seq_no="$(printf '%s\n' "${line}" | sed 's/^seq=\([^ ]*\).*/\1/')"
    dig_arg=()
    if [ "${dig}" = "1" ]; then dig_arg=(--digest); fi
    if [ -n "${SCALE_DEADLINE_EPOCH:-}" ] && [ "$(( $(date +%s) + SCALE_RUN_TIMEOUT ))" -gt "${SCALE_DEADLINE_EPOCH}" ]; then
      printf 'truncated_at_seq=%s\nreason=echeance (run %s ne finirait pas avant %s)\n' "${seq_no}" "${name}" "${SCALE_DEADLINE_EPOCH}" > "${OUT_DIR}/scale_threads_truncated.txt"
      echo "=== SCALE_THREADS TRONQUEE a seq=${seq_no} : echeance ===" >&2
      break
    fi
    RUN_THREADS="${t}" RUN_TIMEOUT_ONE="${SCALE_RUN_TIMEOUT}" \
    EXTRA_STATUS="$(printf 'fold_inflight=%s\ndigest=%s\nfamily=%s\nn=%s\nrepeat=%s\nseq=%s' "${infl}" "${dig}" "${fam}" "${SCALE_N}" "${r}" "${seq_no}")" \
      run_one "${name}" scale_threads \
      "${PROBE_BIN}" "--family=${fam}" "--n=${SCALE_N}" --s=8 --smax=11 --seed=3 "--threads=${t}" "--fold-inflight=${infl}" ${dig_arg[@]+"${dig_arg[@]}"}
    last_code="$(sed -n 's/^code=//p' "${OUT_DIR}/${name}.status" 2>/dev/null | head -n 1)"
    if [ "${last_code:-1}" != "0" ]; then
      printf 'truncated_at_seq=%s\nreason=premier run non nul (%s code=%s)\n' "${seq_no}" "${name}" "${last_code:-?}" > "${OUT_DIR}/scale_threads_truncated.txt"
      echo "=== SCALE_THREADS ARRETEE a seq=${seq_no} : code ${last_code:-?} ===" >&2
      break
    fi
  done < "${SCALE_PLAN}"
fi
echo "=== fin des runs (la validation locale decide du statut, jamais cette ligne) ==="
