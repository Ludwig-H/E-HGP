#!/usr/bin/env bash
# Script DISTANT de la campagne v6 — execute sur la VM par la session gardee
# (session_campagne_v6_g4.sh) ou localement par le selftest transactionnel
# (selftest_campagne_v6.sh, faux pilotes). Il ne demarre ni n'arrete rien :
# la session locale detient les gardes.
#   $1 = source_commit, $2 = source_payload_sha256, $3 = protocol_manifest_sha256,
#   graves dans CHAQUE .status.
#
# TRANSACTIONNEL : chaque run ecrit deux fichiers atomiques (.txt = sortie du
# pilote, .status = code / duree / pic RSS par GNU time / portee / pin),
# errexit desarme autour du run. GNU time OBLIGATOIRE. Les TROIS plans sont
# ANNONCES avant le premier run (conf_plan.txt, bench_plan.txt,
# queue_plan.txt) ; le validateur recalcule chaque sequence depuis les
# parametres et exige un statut par run annonce.
#
# SIX PHASES (ordre : accord differentiel, fils, queue, bench — toutes les
# mesures CPU d'abord —, puis GPU (son build nvcc -j8 ne doit pas contaminer
# les murs CPU), puis la FRONTIERE EN DERNIER (sa pression memoire ne doit
# contaminer aucune mesure) ; les axes a sentinelle `aucun` donnent un plan
# runs=0 et une phase sautee) :
#   1. ACCORD DIFFERENTIEL v5 ≡ v6 sur les paires CONF_SPECS (fam:n — les
#      tailles MESUREES par le bench y figurent, pas seulement 50000) — la
#      v5 est un SUJET DIFFERENTIEL, jamais une autorite de conformite pour
#      la v6 : le pilote v5 (--digest) produit la REFERENCE sur la VM, puis
#      `mhgp6_conformity --expected=<cette reference>` juge l'objet v6
#      (digest_all + forets, ensemble exact des K). Un echec ARRETE la
#      campagne (exit 3, statuts conserves).
#   2. QUEUE STATIONNAIRE v6 (sonde E6 a l'echelle) : familles stationnaires
#      aux tailles etendues x graines, moteur v6 seul, sans --digest — le
#      grand-livre (tests_coeur, octaves_q4, issues par octave) est la
#      donnee. Un echec tronque la queue (grave) sans empecher le bench.
#   3. BENCH APPARIE v5/v6 (mesure, jamais un claim) : pour chaque
#      (famille, n), QUATRE runs en ordre CONTREBALANCE — config impaire
#      v5 v6 v6 v5, config paire v6 v5 v5 v6 (schema ABBA) — memes fils,
#      SANS --digest. Murs et RSS graves par GNU time ; compteurs IDENTIQUES
#      exiges entre les deux runs d'un meme moteur (determinisme) ; aucune
#      acceleration conclue.
#
# ECHEANCE : DEADLINE_EPOCH (epoch, marge de rapatriement comprise) — la
# campagne s'arrete AVANT un run qui ne pourrait pas finir a temps ; la
# troncature est gravee et le validateur juge partial.
set -euo pipefail

SOURCE_COMMIT="${1:?source_commit requis}"
SOURCE_PAYLOAD_SHA256="${2:?source_payload_sha256 requis}"
PROTOCOL_MANIFEST_SHA256="${3:?protocol_manifest_sha256 requis}"
V5_BIN="${V5_BIN:-./build-v5/mhgp5}"
V6_BIN="${V6_BIN:-./build-v6/mhgp6}"
CONF_BIN="${CONF_BIN:-./build-v6/mhgp6_conformity}"
OUT_DIR="${OUT_DIR:-out}"
RUN_TIMEOUT="${RUN_TIMEOUT:-2400}"
TIME_BIN="${TIME_BIN:-/usr/bin/time}"
# CONSTANTES EXACTES (revue en vol du septieme tour : un chemin absolu
# quelconque ne suffit pas) — les porteurs de bornes sont ces deux binaires
# et rien d'autre.
readonly TIMEOUT_BIN="/usr/bin/timeout"
readonly WRAPPER_BASH="/bin/bash"
# Grace unique TRANSMISE par le cycle de vie (jamais un -k local divergent).
GRACE_S="${GRACE_S:-30}"
THREADS="${THREADS:-$(nproc)}"
CONF_SPECS="${CONF_SPECS:-uniform:32000 terrain:32000 eight_clusters:32000 scanline_single_pass:32000 uniform:50000 terrain:50000 eight_clusters:50000 scanline_single_pass:50000 uniform:100000 eight_clusters:100000 uniform:200000 eight_clusters:200000}"
BENCH_SPECS="${BENCH_SPECS:-uniform:32000 terrain:32000 eight_clusters:32000 scanline_single_pass:32000 uniform:100000 eight_clusters:100000 uniform:200000 eight_clusters:200000}"
QUEUE_FAMILIES="${QUEUE_FAMILIES:-terrain_stationnaire scanline_stationnaire}"
QUEUE_N="${QUEUE_N:-64000 128000 256000}"
QUEUE_SEEDS="${QUEUE_SEEDS:-3 4 5}"
# Phase FILS (gain de parallelisme CPU) : paires fam:n:liste_de_fils (csv),
# repetees en ordre CONTREBALANCE (avant/arriere par repetition), moteur v6,
# sans digest — la BIT-IDENTITE entre fils est exigee par le validateur
# (doctrine : sorties identiques quel que soit le nombre de fils).
SWEEP_SPECS="${SWEEP_SPECS:-aucun}"
SWEEP_REPEATS="${SWEEP_REPEATS:-2}"
# Phase GPU (v5, seule ligne a cibles CUDA) : par famille, contrat 50k CPU
# puis --gpu puis adaptatif (--gpu-min-sites=256, familles denses) — digests
# CPU == GPU exiges ; temoin device + mutant + lanes en prealable.
GPU_SPECS="${GPU_SPECS:-aucun}"
GPU_BUILD_TIMEOUT="${GPU_BUILD_TIMEOUT:-3600}"
V5CPU_BIN="${V5CPU_BIN:-./build-v5/mhgp5}"
GPU_BIN="${GPU_BIN:-./build-v5-cuda/mhgp5_cuda}"
GPU_WITNESS_BIN="${GPU_WITNESS_BIN:-./build-v5-cuda/mhgp5_device_witness}"
GPU_Q3_GATE="${GPU_Q3_GATE:-./build-v5-cuda/mhgp5_q3_lane_device_gate}"
GPU_Q4_GATE="${GPU_Q4_GATE:-./build-v5-cuda/mhgp5_q4_lane_device_gate}"
# Phase FRONTIERE (contrats d'echelle) : v6 a 48 fils, tailles croissantes,
# RSS graves ; un echec (OOM, refus de capacite, timeout) est une DONNEE de
# frontiere, pas une faute de campagne — le code est enregistre et la phase
# continue sur les specs suivants.
FRONTIER_SPECS="${FRONTIER_SPECS:-aucun}"
FRONTIER_TIMEOUT="${FRONTIER_TIMEOUT:-3600}"
# Plafond de memoire VIRTUELLE (kB) par run de frontiere : l'instrument qui
# transforme un OOM muet (SIGKILL non attribuable) en std::bad_alloc TYPE.
# 0 = pas de plafond.
FRONTIER_ULIMIT_KB="${FRONTIER_ULIMIT_KB:-0}"
DEADLINE_EPOCH="${DEADLINE_EPOCH:-}"

test -x "${TIME_BIN}" || {
  echo "REFUS : GNU time requis (${TIME_BIN}) pour une campagne a RSS mesure" >&2
  exit 2
}
# Les porteurs de bornes sont EPINGLES en chemins absolus (septieme tour) :
# jamais resolus depuis le PATH.
test -x "${TIMEOUT_BIN}" || { echo "REFUS : timeout epingle absent (${TIMEOUT_BIN})" >&2; exit 2; }
test -x "${WRAPPER_BASH}" || { echo "REFUS : bash epingle absent (${WRAPPER_BASH})" >&2; exit 2; }
[[ "${GRACE_S}" =~ ^[1-9][0-9]{0,2}$ ]] && [ "${GRACE_S}" -le 300 ] || { echo "REFUS : GRACE_S='${GRACE_S}' hors de [1, 300]" >&2; exit 2; }

# REFUS AVANT TOUT RUN d'un parametre mal forme.
refuse() { echo "REFUS : parametre — $1" >&2; exit 2; }
for v in ${QUEUE_N} ${QUEUE_SEEDS} ${RUN_TIMEOUT}; do
  [[ "${v}" =~ ^[1-9][0-9]*$ ]] || refuse "valeur '${v}' non entiere >= 1"
done
if [ -n "${DEADLINE_EPOCH}" ]; then
  [[ "${DEADLINE_EPOCH}" =~ ^[1-9][0-9]*$ ]] || refuse "echeance '${DEADLINE_EPOCH}' non entiere"
fi
for spec in ${CONF_SPECS} ${BENCH_SPECS}; do
  [ "${spec}" = "aucun" ] && continue
  [[ "${spec}" =~ ^[a-z][a-z0-9_]*:[1-9][0-9]*$ ]] || refuse "paire '${spec}' mal formee (fam:n)"
  case "${spec%%:*}" in uniform|terrain|eight_clusters|scanline_single_pass|scanline_overlap_multiecho) ;;
    *) refuse "famille partagee inconnue '${spec%%:*}'" ;; esac
done
for spec in ${GPU_SPECS}; do
  [ "${spec}" = "aucun" ] && continue
  [[ "${spec}" =~ ^[a-z][a-z0-9_]*:[1-9][0-9]*$ ]] || refuse "paire GPU '${spec}' mal formee (fam:n)"
  case "${spec%%:*}" in uniform|terrain|eight_clusters|scanline_single_pass|scanline_overlap_multiecho) ;;
    *) refuse "famille GPU inconnue '${spec%%:*}'" ;; esac
done
for spec in ${SWEEP_SPECS}; do
  [ "${spec}" = "aucun" ] && continue
  [[ "${spec}" =~ ^[a-z][a-z0-9_]*:[1-9][0-9]*:[1-9][0-9,]*$ ]] || refuse "triple FILS '${spec}' mal forme (fam:n:fils_csv)"
done
for spec in ${FRONTIER_SPECS}; do
  [ "${spec}" = "aucun" ] && continue
  [[ "${spec}" =~ ^[a-z][a-z0-9_]*:[1-9][0-9]*$ ]] || refuse "paire FRONTIERE '${spec}' mal formee (fam:n)"
done
[[ "${SWEEP_REPEATS}" =~ ^[1-9][0-9]*$ ]] || refuse "SWEEP_REPEATS non entier"
[[ "${FRONTIER_TIMEOUT}" =~ ^[1-9][0-9]*$ ]] || refuse "FRONTIER_TIMEOUT non entier"
[[ "${GPU_BUILD_TIMEOUT}" =~ ^[1-9][0-9]*$ ]] || refuse "GPU_BUILD_TIMEOUT non entier"
[[ "${FRONTIER_ULIMIT_KB}" =~ ^(0|[1-9][0-9]*)$ ]] || refuse "FRONTIER_ULIMIT_KB non entier"
for f in ${QUEUE_FAMILIES}; do
  case "${f}" in aucun|terrain_stationnaire|scanline_stationnaire|uniform|terrain|eight_clusters|scanline_single_pass|scanline_overlap_multiecho) ;;
    *) refuse "famille v6 inconnue '${f}'" ;; esac
done
for axis in CONF_SPECS BENCH_SPECS QUEUE_FAMILIES QUEUE_N QUEUE_SEEDS SWEEP_SPECS GPU_SPECS FRONTIER_SPECS; do
  [ -n "${!axis// /}" ] || refuse "axe ${axis} vide"
  vals="$(printf '%s\n' ${!axis} | sort)"
  [ "$(printf '%s\n' "${vals}" | uniq -d | wc -l)" -eq 0 ] || refuse "axe ${axis} avec doublon"
done
mkdir -p "${OUT_DIR}"

# run_one NAME SCOPE CMD... : un run transactionnel (statut + sortie
# atomiques, GNU time -v, timeout borne). EXTRA_STATUS (cle=valeur) ajoute.
run_one() {
  local name="$1" scope="$2"; shift 2
  local out="${OUT_DIR}/${name}.txt" status="${OUT_DIR}/${name}.status"
  local rc=0 t0 t1 hwm=""
  t0=$(date +%s)
  # .txt et .status.time sont eux aussi publies ATOMIQUEMENT (audit
  # troisieme tour : seul .status l'etait).
  "${TIME_BIN}" -v -o "${status}.time.tmp" "${TIMEOUT_BIN}" -k "${GRACE_S}" "${RUN_TIMEOUT_ONE:-${RUN_TIMEOUT}}" "$@" >"${out}.tmp" 2>&1 </dev/null || rc=$?
  t1=$(date +%s)
  mv "${out}.tmp" "${out}"
  mv "${status}.time.tmp" "${status}.time"
  hwm=$(grep -oE 'Maximum resident set size[^0-9]*[0-9]+' "${status}.time" 2>/dev/null | grep -oE '[0-9]+$' || true)
  {
    printf 'code=%d\n' "${rc}"
    printf 'duree_s=%d\n' "$((t1 - t0))"
    printf 'peak_rss_kb=%s\n' "${hwm:-inconnu}"
    printf 'timing_scope=%s\n' "${scope}"
    printf 'threads=%s\n' "${THREADS_ONE:-${THREADS}}"
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

# Echeance : vrai (0) si le prochain run ne finirait pas a temps ; grave la
# troncature dans le fichier $3.
past_deadline() {
  local name="$1" phase="$2" file="$3" margin="${4:-${RUN_TIMEOUT}}"
  if [ -n "${DEADLINE_EPOCH}" ] && [ "$(( $(date +%s) + margin ))" -gt "${DEADLINE_EPOCH}" ]; then
    printf 'truncated_at=%s\nphase=%s\nreason=echeance (run ne finirait pas avant %s)\n' \
      "${name}" "${phase}" "${DEADLINE_EPOCH}" > "${OUT_DIR}/${file}"
    echo "=== TRONCATURE (${phase}) avant ${name} : echeance ===" >&2
    return 0
  fi
  return 1
}

# TOPOLOGIE, une fois par campagne.
{
  echo "nproc=$(nproc)"
  echo "affinite_runner=$(taskset -pc $$ 2>/dev/null | sed 's/.*: //' || echo inconnue)"
  echo "date_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "uname=$(uname -srm)"
  grep -E '^(MemTotal|MemAvailable)' /proc/meminfo 2>/dev/null || true
  echo "--- lscpu ---"
  lscpu 2>/dev/null || echo "lscpu indisponible"
} > "${OUT_DIR}/topologie.txt.tmp"
mv "${OUT_DIR}/topologie.txt.tmp" "${OUT_DIR}/topologie.txt"

# LES TROIS PLANS, annonces avant le premier run.
{
  echo "conf_plan=v2"
  echo "specs=$(printf '%s\n' ${CONF_SPECS} | tr '\n' ' ' | sed 's/ $//')"
  echo "threads=${THREADS}"
  seq_no=0
  for spec in ${CONF_SPECS}; do
    [ "${spec}" = "aucun" ] && continue
    fam="${spec%%:*}"; N="${spec##*:}"
    seq_no=$((seq_no + 1)); echo "seq=${seq_no} name=v5ref_${fam}_n${N} family=${fam} n=${N} kind=v5ref"
    seq_no=$((seq_no + 1)); echo "seq=${seq_no} name=conf_${fam}_n${N} family=${fam} n=${N} kind=conf"
  done
  echo "runs=${seq_no}"
} > "${OUT_DIR}/conf_plan.txt.tmp"
mv "${OUT_DIR}/conf_plan.txt.tmp" "${OUT_DIR}/conf_plan.txt"

{
  echo "bench_plan=v2"
  echo "specs=$(printf '%s\n' ${BENCH_SPECS} | tr '\n' ' ' | sed 's/ $//')"
  echo "threads=${THREADS}"
  echo "s=8 smax=11 seed=3"
  seq_no=0; cfg=0
  for spec in ${BENCH_SPECS}; do
    [ "${spec}" = "aucun" ] && continue
    fam="${spec%%:*}"; N="${spec##*:}"
    cfg=$((cfg + 1))
    if [ $((cfg % 2)) -eq 1 ]; then order="v5 v6 v6 v5"; else order="v6 v5 v5 v6"; fi
    pos=0; r5=0; r6=0
    for eng in ${order}; do
      pos=$((pos + 1)); seq_no=$((seq_no + 1))
      if [ "${eng}" = "v5" ]; then r5=$((r5 + 1)); r=${r5}; else r6=$((r6 + 1)); r=${r6}; fi
      echo "seq=${seq_no} name=bench_${fam}_n${N}_${eng}_r${r} family=${fam} n=${N} engine=${eng} pos=${pos} repeat=${r}"
    done
  done
  echo "runs=${seq_no}"
} > "${OUT_DIR}/bench_plan.txt.tmp"
mv "${OUT_DIR}/bench_plan.txt.tmp" "${OUT_DIR}/bench_plan.txt"

{
  echo "queue_plan=v1"
  echo "families=$(printf '%s\n' ${QUEUE_FAMILIES} | tr '\n' ' ' | sed 's/ $//')"
  echo "n_list=$(printf '%s\n' ${QUEUE_N} | tr '\n' ' ' | sed 's/ $//')"
  echo "seeds=$(printf '%s\n' ${QUEUE_SEEDS} | tr '\n' ' ' | sed 's/ $//')"
  echo "threads=${THREADS}"
  echo "s=8 smax=11"
  seq_no=0
  for fam in ${QUEUE_FAMILIES}; do
    [ "${fam}" = "aucun" ] && continue
    for N in ${QUEUE_N}; do
      for seed in ${QUEUE_SEEDS}; do
        seq_no=$((seq_no + 1))
        echo "seq=${seq_no} name=queue_${fam}_n${N}_s${seed} family=${fam} n=${N} seed=${seed}"
      done
    done
  done
  echo "runs=${seq_no}"
} > "${OUT_DIR}/queue_plan.txt.tmp"
mv "${OUT_DIR}/queue_plan.txt.tmp" "${OUT_DIR}/queue_plan.txt"
{
  echo "sweep_plan=v1"
  echo "specs=$(printf '%s\n' ${SWEEP_SPECS} | tr '\n' ' ' | sed 's/ $//')"
  echo "repeats=${SWEEP_REPEATS}"
  echo "s=8 smax=11 seed=3"
  seq_no=0
  for spec in ${SWEEP_SPECS}; do
    [ "${spec}" = "aucun" ] && continue
    fam="${spec%%:*}"; rest="${spec#*:}"; N="${rest%%:*}"; tl="${rest#*:}"
    fils_avant="$(printf '%s' "${tl}" | tr ',' ' ')"
    fils_arriere="$(printf '%s\n' ${fils_avant} | tac | tr '\n' ' ' | sed 's/ $//')"
    r=1
    while [ "${r}" -le "${SWEEP_REPEATS}" ]; do
      if [ $((r % 2)) -eq 1 ]; then fils="${fils_avant}"; else fils="${fils_arriere}"; fi
      pos=0
      for T in ${fils}; do
        pos=$((pos + 1)); seq_no=$((seq_no + 1))
        echo "seq=${seq_no} name=sweep_${fam}_n${N}_t${T}_r${r} family=${fam} n=${N} sweep_threads=${T} repeat=${r} pos=${pos}"
      done
      r=$((r + 1))
    done
  done
  echo "runs=${seq_no}"
} > "${OUT_DIR}/sweep_plan.txt.tmp"
mv "${OUT_DIR}/sweep_plan.txt.tmp" "${OUT_DIR}/sweep_plan.txt"

{
  echo "gpu_plan=v1"
  echo "specs=$(printf '%s\n' ${GPU_SPECS} | tr '\n' ' ' | sed 's/ $//')"
  echo "threads=${THREADS}"
  echo "build_timeout=${GPU_BUILD_TIMEOUT}"
  echo "s=8 smax=11 seed=3"
  seq_no=0
  if [ "${GPU_SPECS}" != "aucun" ]; then
    seq_no=1; echo "seq=1 name=gpu_witness kind=witness"
    seq_no=2; echo "seq=2 name=gpu_lane kind=lane"
    seq_no=3; echo "seq=3 name=gpu_mutant kind=mutant"
    for spec in ${GPU_SPECS}; do
      [ "${spec}" = "aucun" ] && continue
      fam="${spec%%:*}"; N="${spec##*:}"
      seq_no=$((seq_no + 1)); echo "seq=${seq_no} name=gpu_cpu_${fam}_n${N} family=${fam} n=${N} kind=cpu"
      seq_no=$((seq_no + 1)); echo "seq=${seq_no} name=gpu_dev_${fam}_n${N} family=${fam} n=${N} kind=dev"
      seq_no=$((seq_no + 1)); echo "seq=${seq_no} name=gpu_ad_${fam}_n${N} family=${fam} n=${N} kind=ad"
      seq_no=$((seq_no + 1)); echo "seq=${seq_no} name=gpu_idx_${fam}_n${N} family=${fam} n=${N} kind=idx"
    done
  fi
  echo "runs=${seq_no}"
} > "${OUT_DIR}/gpu_plan.txt.tmp"
mv "${OUT_DIR}/gpu_plan.txt.tmp" "${OUT_DIR}/gpu_plan.txt"

{
  echo "frontier_plan=v1"
  echo "specs=$(printf '%s\n' ${FRONTIER_SPECS} | tr '\n' ' ' | sed 's/ $//')"
  echo "threads=${THREADS}"
  echo "s=8 smax=11 seed=3"
  echo "timeout=${FRONTIER_TIMEOUT}"
  echo "ulimit_kb=${FRONTIER_ULIMIT_KB}"
  seq_no=0
  for spec in ${FRONTIER_SPECS}; do
    [ "${spec}" = "aucun" ] && continue
    fam="${spec%%:*}"; N="${spec##*:}"
    seq_no=$((seq_no + 1))
    echo "seq=${seq_no} name=front_${fam}_n${N} family=${fam} n=${N}"
  done
  echo "runs=${seq_no}"
} > "${OUT_DIR}/frontier_plan.txt.tmp"
mv "${OUT_DIR}/frontier_plan.txt.tmp" "${OUT_DIR}/frontier_plan.txt"
echo "=== plans annonces : conf=$(sed -n 's/^runs=//p' "${OUT_DIR}/conf_plan.txt") sweep=$(sed -n 's/^runs=//p' "${OUT_DIR}/sweep_plan.txt") gpu=$(sed -n 's/^runs=//p' "${OUT_DIR}/gpu_plan.txt") frontier=$(sed -n 's/^runs=//p' "${OUT_DIR}/frontier_plan.txt") bench=$(sed -n 's/^runs=//p' "${OUT_DIR}/bench_plan.txt") queue=$(sed -n 's/^runs=//p' "${OUT_DIR}/queue_plan.txt") runs ==="

# PHASE 1 — conformite v5 ≡ v6 sur les paires CONF_SPECS. Un echec ARRETE tout.
for spec in ${CONF_SPECS}; do
  [ "${spec}" = "aucun" ] && continue
  fam="${spec%%:*}"; CN="${spec##*:}"
  if past_deadline "v5ref_${fam}_n${CN}" conformite conf_tronquee.txt; then exit 3; fi
  run_one "v5ref_${fam}_n${CN}" conformity_ref \
    "${V5_BIN}" "--family=${fam}" "--n=${CN}" --s=8 --smax=11 --seed=3 "--threads=${THREADS}" --digest
  if ! grep -q '^code=0$' "${OUT_DIR}/v5ref_${fam}_n${CN}.status" \
     || ! grep -q '^digest_all=' "${OUT_DIR}/v5ref_${fam}_n${CN}.txt"; then
    echo "CONFORMITE NON ETABLIE : reference v5 ${fam} n=${CN} invalide — campagne refusee, statuts conserves" >&2
    exit 3
  fi
  if past_deadline "conf_${fam}_n${CN}" conformite conf_tronquee.txt; then exit 3; fi
  run_one "conf_${fam}_n${CN}" conformity \
    "${CONF_BIN}" "--family=${fam}" "--n=${CN}" "--threads=${THREADS}" \
    "--expected=${OUT_DIR}/v5ref_${fam}_n${CN}.txt"
  if ! grep -q '^code=0$' "${OUT_DIR}/conf_${fam}_n${CN}.status" \
     || ! grep -q 'identiques (objet)' "${OUT_DIR}/conf_${fam}_n${CN}.txt"; then
    echo "CONFORMITE NON ETABLIE : conf_${fam}_n${CN} — queue et bench refuses, statuts conserves" >&2
    exit 3
  fi
done


# PHASE FILS — gain de parallelisme CPU, moteur v6, ordre contrebalance
# (avant/arriere par repetition). SANS --digest ; le validateur exige
# l'INVARIANCE DU GRAND-LIVRE entre fils (compteurs, generation,
# cardinalites — pas la bit-identite de l'objet, qui reste prouvee par les
# portes a digest). Un run non nul tronque la phase (grave) sans empecher
# la suite.
while read -r line; do
  case "${line}" in seq=*) ;; *) continue ;; esac
  name="$(printf '%s\n' "${line}" | sed 's/.* name=\([^ ]*\).*/\1/')"
  fam="$(printf '%s\n' "${line}" | sed 's/.* family=\([^ ]*\).*/\1/')"
  N="$(printf '%s\n' "${line}" | sed 's/.* n=\([^ ]*\).*/\1/')"
  T="$(printf '%s\n' "${line}" | sed 's/.* sweep_threads=\([^ ]*\).*/\1/')"
  r="$(printf '%s\n' "${line}" | sed 's/.* repeat=\([^ ]*\).*/\1/')"
  pos="$(printf '%s\n' "${line}" | sed 's/.* pos=\([^ ]*\).*/\1/')"
  seq_no="$(printf '%s\n' "${line}" | sed 's/^seq=\([^ ]*\).*/\1/')"
  if past_deadline "${name}" sweep sweep_tronquee.txt; then break; fi
  THREADS_ONE="${T}" \
  EXTRA_STATUS="$(printf 'family=%s\nn=%s\nsweep_threads=%s\nrepeat=%s\npos=%s\nseq=%s' \
                  "${fam}" "${N}" "${T}" "${r}" "${pos}" "${seq_no}")" \
    run_one "${name}" sweep_fils \
    "${V6_BIN}" "--family=${fam}" "--n=${N}" --s=8 --smax=11 --seed=3 "--threads=${T}"
  last_code="$(sed -n 's/^code=//p' "${OUT_DIR}/${name}.status" | head -n 1)"
  if [ "${last_code:-1}" != "0" ]; then
    printf 'truncated_at=%s\nphase=sweep\nreason=premier run non nul (code=%s)\n' \
      "${name}" "${last_code:-?}" > "${OUT_DIR}/sweep_tronquee.txt"
    echo "=== PHASE FILS TRONQUEE a ${name} : code ${last_code:-?} ===" >&2
    break
  fi
done < "${OUT_DIR}/sweep_plan.txt"

# PHASE 2 — queue stationnaire v6 (la sonde discriminante d'abord), sans
# --digest. Un run non nul tronque la queue (grave) sans empecher le bench.
while read -r line; do
  case "${line}" in seq=*) ;; *) continue ;; esac
  name="$(printf '%s\n' "${line}" | sed 's/.* name=\([^ ]*\).*/\1/')"
  fam="$(printf '%s\n' "${line}" | sed 's/.* family=\([^ ]*\).*/\1/')"
  N="$(printf '%s\n' "${line}" | sed 's/.* n=\([^ ]*\).*/\1/')"
  seed="$(printf '%s\n' "${line}" | sed 's/.* seed=\([^ ]*\).*/\1/')"
  seq_no="$(printf '%s\n' "${line}" | sed 's/^seq=\([^ ]*\).*/\1/')"
  if past_deadline "${name}" queue queue_tronquee.txt; then break; fi
  EXTRA_STATUS="$(printf 'family=%s\nn=%s\nseed=%s\nseq=%s' "${fam}" "${N}" "${seed}" "${seq_no}")" \
    run_one "${name}" queue_stationnaire \
    "${V6_BIN}" "--family=${fam}" "--n=${N}" --s=8 --smax=11 "--seed=${seed}" "--threads=${THREADS}"
  last_code="$(sed -n 's/^code=//p' "${OUT_DIR}/${name}.status" | head -n 1)"
  if [ "${last_code:-1}" != "0" ]; then
    printf 'truncated_at=%s\nphase=queue\nreason=premier run non nul (code=%s)\n' \
      "${name}" "${last_code:-?}" > "${OUT_DIR}/queue_tronquee.txt"
    echo "=== QUEUE TRONQUEE a ${name} : code ${last_code:-?} ===" >&2
    break
  fi
done < "${OUT_DIR}/queue_plan.txt"

# PHASE 3 — bench apparie ABBA, sans --digest. Un run non nul tronque le
# bench (grave).
bench_ok=1
while read -r line; do
  case "${line}" in seq=*) ;; *) continue ;; esac
  name="$(printf '%s\n' "${line}" | sed 's/.* name=\([^ ]*\).*/\1/')"
  fam="$(printf '%s\n' "${line}" | sed 's/.* family=\([^ ]*\).*/\1/')"
  N="$(printf '%s\n' "${line}" | sed 's/.* n=\([^ ]*\).*/\1/')"
  eng="$(printf '%s\n' "${line}" | sed 's/.* engine=\([^ ]*\).*/\1/')"
  pos="$(printf '%s\n' "${line}" | sed 's/.* pos=\([^ ]*\).*/\1/')"
  r="$(printf '%s\n' "${line}" | sed 's/.* repeat=\([^ ]*\).*/\1/')"
  seq_no="$(printf '%s\n' "${line}" | sed 's/^seq=\([^ ]*\).*/\1/')"
  if [ "${bench_ok}" -ne 1 ]; then break; fi
  if past_deadline "${name}" bench bench_tronquee.txt; then bench_ok=0; break; fi
  bin="${V6_BIN}"; if [ "${eng}" = "v5" ]; then bin="${V5_BIN}"; fi
  EXTRA_STATUS="$(printf 'engine=%s\nfamily=%s\nn=%s\npos=%s\nrepeat=%s\nseq=%s' \
                  "${eng}" "${fam}" "${N}" "${pos}" "${r}" "${seq_no}")" \
    run_one "${name}" bench_paired \
    "${bin}" "--family=${fam}" "--n=${N}" --s=8 --smax=11 --seed=3 "--threads=${THREADS}"
  last_code="$(sed -n 's/^code=//p' "${OUT_DIR}/${name}.status" | head -n 1)"
  if [ "${last_code:-1}" != "0" ]; then
    printf 'truncated_at=%s\nphase=bench\nreason=premier run non nul (code=%s)\n' \
      "${name}" "${last_code:-?}" > "${OUT_DIR}/bench_tronquee.txt"
    echo "=== BENCH TRONQUE a ${name} : code ${last_code:-?} ===" >&2
    bench_ok=0
  fi
done < "${OUT_DIR}/bench_plan.txt"

# PHASE GPU — v5, seule ligne a cibles CUDA (protocole herite de
# v5_campaign_remote.sh) : temoin device (build nvcc separe), lanes q3/q4
# device contre la production, mutant temoin (code 4 exige), puis par paire
# GPU_SPECS trois contrats a --digest : CPU (reference), --gpu, adaptatif
# --gpu-min-sites=256. Le validateur exige les digests IDENTIQUES entre les
# trois. Toute non-conformite tronque la phase (grave) sans empecher la
# suite ; l acceleration se lit dans les murs, jamais conclue ici.
if [ "${GPU_SPECS}" != "aucun" ]; then
  gpu_ok=1
  NVCC_BIN="${NVCC_BIN:-$(command -v nvcc 2>/dev/null || ls /usr/local/cuda*/bin/nvcc 2>/dev/null | head -1 || true)}"
  if [ -z "${NVCC_BIN}" ]; then
    printf 'code=2\nduree_s=0\npeak_rss_kb=0\ntiming_scope=device_witness\nthreads=%s\ncommande=nvcc-absent\nsource_commit=%s\nsource_payload_sha256=%s\nprotocol_manifest_sha256=%s\nfinished=1\n' \
      "${THREADS}" "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}" > "${OUT_DIR}/gpu_witness.status"
    echo "REFUS : nvcc absent (temoin device non execute)" > "${OUT_DIR}/gpu_witness.txt"
    printf 'truncated_at=gpu_witness\nphase=gpu\nreason=nvcc absent\n' > "${OUT_DIR}/gpu_tronquee.txt"
    gpu_ok=0
  fi
  if [ "${gpu_ok}" -eq 1 ]; then
    if past_deadline gpu_witness gpu gpu_tronquee.txt "${GPU_BUILD_TIMEOUT}"; then gpu_ok=0; fi
  fi
  if [ "${gpu_ok}" -eq 1 ]; then
    LINEAGE_STATUS="$(printf 'engine=v5\nlineage=historical_baseline\nauthority=non_authoritative')"
    RUN_TIMEOUT_ONE="${GPU_BUILD_TIMEOUT}" EXTRA_STATUS="${LINEAGE_STATUS}" run_one gpu_witness device_witness "${WRAPPER_BASH}" -c "set -e; echo nvcc=${NVCC_BIN}; uname -m; ${NVCC_BIN} --version 2>&1 | tail -2; nvidia-smi --query-gpu=name,driver_version --format=csv,noheader; ${GPU_CMAKE_BIN:-cmake} -S morsehgp3D_v5 -B build-v5-cuda -DCMAKE_BUILD_TYPE=Release -DMHGP5_ENABLE_CUDA=ON -DCMAKE_CUDA_COMPILER=${NVCC_BIN} 2>&1 | tail -40; test \${PIPESTATUS[0]} -eq 0; ${GPU_CMAKE_BIN:-cmake} --build build-v5-cuda --target mhgp5_device_witness -j8 2>&1 | grep -vE '^\[|^Scanning' | tail -60; test \${PIPESTATUS[0]} -eq 0; ${GPU_WITNESS_BIN}"
    if ! grep -q '^code=0$' "${OUT_DIR}/gpu_witness.status"; then
      printf 'truncated_at=gpu_witness\nphase=gpu\nreason=temoin device non conforme\n' > "${OUT_DIR}/gpu_tronquee.txt"
      echo "=== PHASE GPU TRONQUEE : temoin device non conforme (voir gpu_witness.txt) ===" >&2
      gpu_ok=0
    fi
  fi
  if [ "${gpu_ok}" -eq 1 ]; then
    if past_deadline gpu_lane gpu gpu_tronquee.txt "${GPU_BUILD_TIMEOUT}"; then gpu_ok=0; fi
  fi
  if [ "${gpu_ok}" -eq 1 ]; then
    RUN_TIMEOUT_ONE="${GPU_BUILD_TIMEOUT}" EXTRA_STATUS="${LINEAGE_STATUS}" run_one gpu_lane device_lane "${WRAPPER_BASH}" -c "set -e; ${GPU_CMAKE_BIN:-cmake} --build build-v5-cuda --target mhgp5_q3_lane_device_gate mhgp5_q4_lane_device_gate mhgp5_cuda -j8 2>&1 | grep -vE '^\[|^Scanning' | tail -60; test \${PIPESTATUS[0]} -eq 0; ${GPU_Q3_GATE} --family=uniform --n=1200; ${GPU_Q3_GATE} --family=eight_clusters --n=1200 --threads=4; ${GPU_Q3_GATE} --family=uniform --n=8000 --threads=8 --min-candidates=100000; ${GPU_Q4_GATE} --family=uniform --n=1200; ${GPU_Q4_GATE} --family=eight_clusters --n=1200 --threads=4; ${GPU_Q4_GATE} --family=uniform --n=8000 --threads=8 --min-candidates=100000; ${GPU_Q3_GATE} --family=uniform --n=300 --coord=40 --min-candidates=200; ${GPU_Q4_GATE} --family=uniform --n=300 --coord=40 --min-candidates=200 --min-deep=20; ${GPU_Q3_GATE} --family=uniform --n=1200 --wire=index; ${GPU_Q3_GATE} --family=eight_clusters --n=1200 --threads=4 --wire=index; ${GPU_Q3_GATE} --family=uniform --n=300 --coord=40 --min-candidates=200 --wire=index; ${GPU_Q4_GATE} --family=uniform --n=1200 --wire=index; ${GPU_Q4_GATE} --family=eight_clusters --n=1200 --threads=4 --wire=index; ${GPU_Q4_GATE} --family=uniform --n=300 --coord=40 --min-candidates=200 --min-deep=20 --wire=index"
    if past_deadline gpu_mutant gpu gpu_tronquee.txt; then gpu_ok=0; fi
  fi
  if [ "${gpu_ok}" -eq 1 ]; then
    EXTRA_STATUS="${LINEAGE_STATUS}" run_one gpu_mutant device_mutant "${GPU_WITNESS_BIN}" --inject=witness-no-warp-correction
    if ! grep -q '^code=0$' "${OUT_DIR}/gpu_lane.status" || ! grep -q '^code=4$' "${OUT_DIR}/gpu_mutant.status"; then
      printf 'truncated_at=gpu_lane\nphase=gpu\nreason=lane device ou mutant non conformes\n' > "${OUT_DIR}/gpu_tronquee.txt"
      echo "=== PHASE GPU TRONQUEE : lane device ou mutant non conformes ===" >&2
      gpu_ok=0
    fi
  fi
  if [ "${gpu_ok}" -eq 1 ]; then
    for spec in ${GPU_SPECS}; do
      [ "${spec}" = "aucun" ] && continue
      fam="${spec%%:*}"; GN="${spec##*:}"
      if past_deadline "gpu_cpu_${fam}_n${GN}" gpu gpu_tronquee.txt; then gpu_ok=0; break; fi
      EXTRA_STATUS="$(printf 'family=%s\nn=%s\nkind=cpu\nengine=v5\nlineage=historical_baseline\nauthority=non_authoritative' "${fam}" "${GN}")" \
        run_one "gpu_cpu_${fam}_n${GN}" gpu_contract_cpu \
        "${V5CPU_BIN}" "--family=${fam}" "--n=${GN}" --s=8 --smax=11 --seed=3 "--threads=${THREADS}" --digest
      if past_deadline "gpu_dev_${fam}_n${GN}" gpu gpu_tronquee.txt; then gpu_ok=0; break; fi
      EXTRA_STATUS="$(printf 'family=%s\nn=%s\nkind=dev\nengine=v5\nlineage=historical_baseline\nauthority=non_authoritative' "${fam}" "${GN}")" \
        run_one "gpu_dev_${fam}_n${GN}" gpu_contract_dev \
        "${GPU_BIN}" --gpu "--family=${fam}" "--n=${GN}" --s=8 --smax=11 --seed=3 "--threads=${THREADS}" --digest
      if past_deadline "gpu_ad_${fam}_n${GN}" gpu gpu_tronquee.txt; then gpu_ok=0; break; fi
      EXTRA_STATUS="$(printf 'family=%s\nn=%s\nkind=ad\nengine=v5\nlineage=historical_baseline\nauthority=non_authoritative' "${fam}" "${GN}")" \
        run_one "gpu_ad_${fam}_n${GN}" gpu_contract_ad \
        "${GPU_BIN}" --gpu --gpu-min-sites=256 "--family=${fam}" "--n=${GN}" --s=8 --smax=11 --seed=3 "--threads=${THREADS}" --digest
      if past_deadline "gpu_idx_${fam}_n${GN}" gpu gpu_tronquee.txt; then gpu_ok=0; break; fi
      EXTRA_STATUS="$(printf 'family=%s\nn=%s\nkind=idx\nengine=v5\nlineage=historical_baseline\nauthority=non_authoritative' "${fam}" "${GN}")" \
        run_one "gpu_idx_${fam}_n${GN}" gpu_contract_idx \
        "${GPU_BIN}" --gpu --gpu-wire=index "--family=${fam}" "--n=${GN}" --s=8 --smax=11 --seed=3 "--threads=${THREADS}" --digest
      for k in cpu dev ad idx; do
        if ! grep -q '^code=0$' "${OUT_DIR}/gpu_${k}_${fam}_n${GN}.status"; then
          printf 'truncated_at=gpu_%s_%s_n%s\nphase=gpu\nreason=contrat non nul\n' \
            "${k}" "${fam}" "${GN}" > "${OUT_DIR}/gpu_tronquee.txt"
          echo "=== PHASE GPU TRONQUEE a gpu_${k}_${fam}_n${GN} ===" >&2
          gpu_ok=0
        fi
      done
      [ "${gpu_ok}" -eq 1 ] || break
    done
  fi
fi


# PHASE FRONTIERE — contrats d echelle v6 : tailles croissantes, RSS graves,
# executee EN DERNIER (cinquieme tour : pression memoire, timeout ou OOM ne
# doivent pas contaminer le bench). Issues TYPEES par le validateur (0 =
# contrat complet, 124 = timeout, code non nul a motif de capacite = la
# donnee ; tout le reste invalide la phase) ; FRONTIER_ULIMIT_KB > 0 plafonne
# la memoire virtuelle pour transformer un OOM muet en std::bad_alloc type.
# La phase CONTINUE apres un refus de capacite ; seule l echeance tronque.
while read -r line; do
  case "${line}" in seq=*) ;; *) continue ;; esac
  name="$(printf '%s\n' "${line}" | sed 's/.* name=\([^ ]*\).*/\1/')"
  fam="$(printf '%s\n' "${line}" | sed 's/.* family=\([^ ]*\).*/\1/')"
  N="$(printf '%s\n' "${line}" | sed 's/.* n=\([^ ]*\).*/\1/')"
  seq_no="$(printf '%s\n' "${line}" | sed 's/^seq=\([^ ]*\).*/\1/')"
  if past_deadline "${name}" frontiere frontier_tronquee.txt "$((FRONTIER_TIMEOUT + 60))"; then break; fi
  front_cmd=("${V6_BIN}" "--family=${fam}" "--n=${N}" --s=8 --smax=11 --seed=3 "--threads=${THREADS}")
  limit_kind="none"; limit_kb=0
  if [ "${FRONTIER_ULIMIT_KB}" -gt 0 ]; then
    front_cmd=("${WRAPPER_BASH}" -c 'ulimit -v "$1" && shift && exec "$@"' _ "${FRONTIER_ULIMIT_KB}" "${front_cmd[@]}")
    limit_kind="rlimit_as"; limit_kb="${FRONTIER_ULIMIT_KB}"
  fi
  RUN_TIMEOUT_ONE="${FRONTIER_TIMEOUT}" \
  EXTRA_STATUS="$(printf 'family=%s\nn=%s\nseq=%s\nlimit_kind=%s\nlimit_kb=%s' \
                  "${fam}" "${N}" "${seq_no}" "${limit_kind}" "${limit_kb}")" \
    run_one "${name}" frontiere "${front_cmd[@]}"
done < "${OUT_DIR}/frontier_plan.txt"

# MANIFESTE DISTANT : sha256 de chaque artefact produit, grave en dernier —
# le validateur recoupe apres rapatriement (corruption scp tuee).
( cd "${OUT_DIR}" && find . -maxdepth 1 -type f ! -name 'MANIFESTE_DISTANT.txt*' -printf '%P\n' | sort \
  | xargs -d '\n' sha256sum > MANIFESTE_DISTANT.txt.tmp && mv MANIFESTE_DISTANT.txt.tmp MANIFESTE_DISTANT.txt )
echo "=== fin des runs (la validation locale decide du statut, jamais cette ligne) ==="
