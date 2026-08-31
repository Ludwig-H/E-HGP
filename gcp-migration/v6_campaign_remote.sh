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
# TROIS PHASES :
#   1. CONFORMITE v5 ≡ v6 a n=50000 (taille de contrat) : pour chaque famille
#      partagee, le pilote v5 (--digest) produit la REFERENCE sur la VM, puis
#      `mhgp6_conformity --expected=<cette reference>` juge l'objet v6
#      (digest_all + dix forets). Un echec ARRETE la campagne (exit 3,
#      statuts conserves) : pas de mesure sur un moteur non conforme.
#   2. BENCH APPARIE v5/v6 (mesure, jamais un claim) : pour chaque
#      (famille, n), QUATRE runs en ordre CONTREBALANCE — config impaire
#      v5 v6 v6 v5, config paire v6 v5 v5 v6 (schema ABBA) — memes fils,
#      SANS --digest (le mur compare les moteurs, pas le hachage). Murs et
#      RSS graves par GNU time ; le validateur exige des compteurs
#      IDENTIQUES entre les deux runs d'un meme moteur (determinisme) et ne
#      conclut JAMAIS sur une acceleration.
#   3. QUEUE STATIONNAIRE v6 (sonde E6 a l'echelle) : familles stationnaires
#      aux tailles etendues x graines, moteur v6 seul, sans --digest — le
#      grand-livre (tests_coeur, octaves_q4, ...) est la donnee.
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
THREADS="${THREADS:-$(nproc)}"
CONF_FAMILIES="${CONF_FAMILIES:-uniform terrain eight_clusters scanline_single_pass}"
CONF_N="${CONF_N:-50000}"
BENCH_FAMILIES="${BENCH_FAMILIES:-uniform terrain eight_clusters scanline_single_pass}"
BENCH_N="${BENCH_N:-32000 100000 200000}"
QUEUE_FAMILIES="${QUEUE_FAMILIES:-terrain_stationnaire scanline_stationnaire}"
QUEUE_N="${QUEUE_N:-64000 128000 256000}"
QUEUE_SEEDS="${QUEUE_SEEDS:-3 4 5}"
DEADLINE_EPOCH="${DEADLINE_EPOCH:-}"

test -x "${TIME_BIN}" || {
  echo "REFUS : GNU time requis (${TIME_BIN}) pour une campagne a RSS mesure" >&2
  exit 2
}

# REFUS AVANT TOUT RUN d'un parametre mal forme.
refuse() { echo "REFUS : parametre — $1" >&2; exit 2; }
for v in ${CONF_N} ${BENCH_N} ${QUEUE_N} ${QUEUE_SEEDS} ${RUN_TIMEOUT}; do
  [[ "${v}" =~ ^[1-9][0-9]*$ ]] || refuse "valeur '${v}' non entiere >= 1"
done
if [ -n "${DEADLINE_EPOCH}" ]; then
  [[ "${DEADLINE_EPOCH}" =~ ^[1-9][0-9]*$ ]] || refuse "echeance '${DEADLINE_EPOCH}' non entiere"
fi
for f in ${CONF_FAMILIES} ${BENCH_FAMILIES}; do
  case "${f}" in uniform|terrain|eight_clusters|scanline_single_pass|scanline_overlap_multiecho) ;;
    *) refuse "famille partagee inconnue '${f}'" ;; esac
done
for f in ${QUEUE_FAMILIES}; do
  case "${f}" in terrain_stationnaire|scanline_stationnaire|uniform|terrain|eight_clusters|scanline_single_pass|scanline_overlap_multiecho) ;;
    *) refuse "famille v6 inconnue '${f}'" ;; esac
done
for axis in CONF_FAMILIES BENCH_FAMILIES BENCH_N QUEUE_FAMILIES QUEUE_N QUEUE_SEEDS; do
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
  "${TIME_BIN}" -v -o "${status}.time" timeout "${RUN_TIMEOUT_ONE:-${RUN_TIMEOUT}}" "$@" >"${out}" 2>&1 </dev/null || rc=$?
  t1=$(date +%s)
  hwm=$(grep -oE 'Maximum resident set size[^0-9]*[0-9]+' "${status}.time" 2>/dev/null | grep -oE '[0-9]+$' || true)
  {
    printf 'code=%d\n' "${rc}"
    printf 'duree_s=%d\n' "$((t1 - t0))"
    printf 'peak_rss_kb=%s\n' "${hwm:-inconnu}"
    printf 'timing_scope=%s\n' "${scope}"
    printf 'threads=%s\n' "${THREADS}"
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
  local name="$1" phase="$2" file="$3"
  if [ -n "${DEADLINE_EPOCH}" ] && [ "$(( $(date +%s) + RUN_TIMEOUT ))" -gt "${DEADLINE_EPOCH}" ]; then
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
  echo "conf_plan=v1"
  echo "families=$(printf '%s\n' ${CONF_FAMILIES} | tr '\n' ' ' | sed 's/ $//')"
  echo "n=${CONF_N}"
  echo "threads=${THREADS}"
  seq_no=0
  for fam in ${CONF_FAMILIES}; do
    seq_no=$((seq_no + 1)); echo "seq=${seq_no} name=v5ref_${fam}_n${CONF_N} family=${fam} kind=v5ref"
    seq_no=$((seq_no + 1)); echo "seq=${seq_no} name=conf_${fam}_n${CONF_N} family=${fam} kind=conf"
  done
  echo "runs=${seq_no}"
} > "${OUT_DIR}/conf_plan.txt.tmp"
mv "${OUT_DIR}/conf_plan.txt.tmp" "${OUT_DIR}/conf_plan.txt"

{
  echo "bench_plan=v1"
  echo "families=$(printf '%s\n' ${BENCH_FAMILIES} | tr '\n' ' ' | sed 's/ $//')"
  echo "n_list=$(printf '%s\n' ${BENCH_N} | tr '\n' ' ' | sed 's/ $//')"
  echo "threads=${THREADS}"
  echo "s=8 smax=11 seed=3"
  seq_no=0; cfg=0
  for N in ${BENCH_N}; do
    for fam in ${BENCH_FAMILIES}; do
      cfg=$((cfg + 1))
      if [ $((cfg % 2)) -eq 1 ]; then order="v5 v6 v6 v5"; else order="v6 v5 v5 v6"; fi
      pos=0; r5=0; r6=0
      for eng in ${order}; do
        pos=$((pos + 1)); seq_no=$((seq_no + 1))
        if [ "${eng}" = "v5" ]; then r5=$((r5 + 1)); r=${r5}; else r6=$((r6 + 1)); r=${r6}; fi
        echo "seq=${seq_no} name=bench_${fam}_n${N}_${eng}_r${r} family=${fam} n=${N} engine=${eng} pos=${pos} repeat=${r}"
      done
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
echo "=== plans annonces : conf=$(sed -n 's/^runs=//p' "${OUT_DIR}/conf_plan.txt") bench=$(sed -n 's/^runs=//p' "${OUT_DIR}/bench_plan.txt") queue=$(sed -n 's/^runs=//p' "${OUT_DIR}/queue_plan.txt") runs ==="

# PHASE 1 — conformite v5 ≡ v6 a la taille de contrat. Un echec ARRETE tout.
for fam in ${CONF_FAMILIES}; do
  if past_deadline "v5ref_${fam}_n${CONF_N}" conformite conf_tronquee.txt; then exit 3; fi
  run_one "v5ref_${fam}_n${CONF_N}" conformity_ref \
    "${V5_BIN}" "--family=${fam}" "--n=${CONF_N}" --s=8 --smax=11 --seed=3 "--threads=${THREADS}" --digest
  if ! grep -q '^code=0$' "${OUT_DIR}/v5ref_${fam}_n${CONF_N}.status" \
     || ! grep -q '^digest_all=' "${OUT_DIR}/v5ref_${fam}_n${CONF_N}.txt"; then
    echo "CONFORMITE NON ETABLIE : reference v5 ${fam} n=${CONF_N} invalide — campagne refusee, statuts conserves" >&2
    exit 3
  fi
  if past_deadline "conf_${fam}_n${CONF_N}" conformite conf_tronquee.txt; then exit 3; fi
  run_one "conf_${fam}_n${CONF_N}" conformity \
    "${CONF_BIN}" "--family=${fam}" "--n=${CONF_N}" "--threads=${THREADS}" \
    "--expected=${OUT_DIR}/v5ref_${fam}_n${CONF_N}.txt"
  if ! grep -q '^code=0$' "${OUT_DIR}/conf_${fam}_n${CONF_N}.status" \
     || ! grep -q 'identiques (objet)' "${OUT_DIR}/conf_${fam}_n${CONF_N}.txt"; then
    echo "CONFORMITE NON ETABLIE : conf_${fam}_n${CONF_N} — bench et queue refuses, statuts conserves" >&2
    exit 3
  fi
done

# PHASE 2 — bench apparie ABBA, sans --digest. Un run non nul tronque le
# bench (grave) mais n'empeche pas la phase 3.
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

# PHASE 3 — queue stationnaire v6, sans --digest. Un run non nul arrete.
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
echo "=== fin des runs (la validation locale decide du statut, jamais cette ligne) ==="
