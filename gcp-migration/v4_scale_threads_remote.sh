#!/usr/bin/env bash
# Script DISTANT des sessions SCALE_THREADS (audits bloquants 9223888 et
# b3a6eb4 apres a694496) : l'experience qui decide de l'architecture —
# le MEME objet est-il calcule a 1/8/nproc fils, et en combien de temps,
# sur les tailles massives ? Deux phases, une session chacune :
#   n32000 : uniform t1/t8/tmax + eight_clusters t8/tmax (t1 uniform
#            seulement — le t1 eight_clusters approcherait le timeout) ;
#   n64000 : quatre familles, tmax uniquement (cette taille n'existe
#            qu'avec le parallelisme).
# CHAQUE run est isole, sequentiel, sur toute la machine, avec --digest :
# le validateur exige l'EGALITE des digests canoniques entre runs
# apparies — jamais code=0 seul (audit § 2.2).
#
# BUDGET TEMPOREL PARTAGE (audit b3a6eb4 § 3) : ce script est la SEULE
# source de verite de la liste des runs et de leurs timeouts —
# `--print-budget PHASE` imprime la somme des timeouts sequentiels, que
# le lanceur consomme pour son preflight ; au run, le meme script refuse
# de DEMARRER un run si le temps restant avant DEADLINE_EPOCH est
# inferieur a son timeout plus la marge de rapatriement : statut
# not_run_budget=1 conserve, jamais un run ampute.
#
# Usage : v4_scale_threads_remote.sh SOURCE_COMMIT PAYLOAD_SHA MANIFEST_SHA PHASE DEADLINE_EPOCH
#         v4_scale_threads_remote.sh --print-budget PHASE
set -euo pipefail

RUN_TIMEOUT="${RUN_TIMEOUT:-3600}"
RETRIEVE_MARGIN="${RETRIEVE_MARGIN:-900}"

phase_runs() {
  # name|family|threads_label par ligne ; threads_label ∈ {1, 8, max}.
  case "$1" in
    n32000)
      printf '%s\n' \
        "thr_uniform_n32000_smax11_t1|uniform|1" \
        "thr_uniform_n32000_smax11_t8|uniform|8" \
        "thr_uniform_n32000_smax11_tmax|uniform|max" \
        "thr_eight_clusters_n32000_smax11_t8|eight_clusters|8" \
        "thr_eight_clusters_n32000_smax11_tmax|eight_clusters|max"
      ;;
    n64000)
      printf '%s\n' \
        "thr_uniform_n64000_smax11_tmax|uniform|max" \
        "thr_terrain_n64000_smax11_tmax|terrain|max" \
        "thr_eight_clusters_n64000_smax11_tmax|eight_clusters|max" \
        "thr_scanline_overlap_multiecho_n64000_smax11_tmax|scanline_overlap_multiecho|max"
      ;;
    *)
      echo "REFUS : phase inconnue '$1' (n32000|n64000)" >&2
      return 2
      ;;
  esac
}

if [ "${1:-}" = "--print-budget" ]; then
  PHASE="${2:?phase requise}"
  NRUNS=$(phase_runs "${PHASE}" | wc -l)
  echo "$((NRUNS * RUN_TIMEOUT))"
  exit 0
fi

SOURCE_COMMIT="${1:?source_commit requis}"
SOURCE_PAYLOAD_SHA256="${2:?source_payload_sha256 requis}"
PROTOCOL_MANIFEST_SHA256="${3:?protocol_manifest_sha256 requis}"
PHASE="${4:?phase requise (n32000|n64000)}"
DEADLINE_EPOCH="${5:?deadline_epoch requis (refus de demarrage au-dela)}"
PROBE_BIN="${PROBE_BIN:-./build/mhgp4_forest_probe}"
OUT_DIR="${OUT_DIR:-out}"
TIME_BIN="${TIME_BIN:-/usr/bin/time}"

test -x "${TIME_BIN}" || {
  echo "REFUS : GNU time requis (${TIME_BIN})" >&2
  exit 2
}
phase_runs "${PHASE}" >/dev/null
mkdir -p "${OUT_DIR}"
NCPU=$(nproc)
N_OF_PHASE="${PHASE#n}"
CPU_SET="0-$((NCPU - 1))"


run_one() {
  local name="$1" treq="$2"; shift 2
  local out="${OUT_DIR}/${name}.txt" status="${OUT_DIR}/${name}.status"
  local rc=0 t0 t1 hwm="" args_sha
  args_sha=$(printf '%s' "$*" | sha256sum | awk '{print $1}')
  t0=$(date +%s)
  # REFUS DE DEMARRAGE (audit b3a6eb4 § 3) : le run n'est pas lance si
  # son timeout ne tient plus avant la deadline — statut explicite.
  if [ $((t0 + RUN_TIMEOUT + RETRIEVE_MARGIN)) -gt "${DEADLINE_EPOCH}" ]; then
    {
      printf 'not_run_budget=1\n'
      printf 'timing_scope=scale_threads\n'
      printf 'threads_requested=%s\n' "${treq}"
      printf 'nproc=%d\n' "${NCPU}"
      printf 'cpu_set=%s\n' "${CPU_SET}"
      printf 'args_sha256=%s\n' "${args_sha}"
      printf 'source_commit=%s\n' "${SOURCE_COMMIT}"
      printf 'source_payload_sha256=%s\n' "${SOURCE_PAYLOAD_SHA256}"
      printf 'protocol_manifest_sha256=%s\n' "${PROTOCOL_MANIFEST_SHA256}"
      printf 'finished=1\n'
    } > "${status}.tmp"
    mv "${status}.tmp" "${status}"
    echo "--- NON LANCE (budget) : ${name}"
    return 0
  fi
  set +e
  "${TIME_BIN}" -v -o "${status}.time" \
    timeout "${RUN_TIMEOUT}" taskset -c "${CPU_SET}" "${PROBE_BIN}" "$@" \
    >"${out}" 2>&1
  rc=$?
  set -e
  t1=$(date +%s)
  hwm=$(grep -oE 'Maximum resident set size[^0-9]*[0-9]+' "${status}.time" 2>/dev/null \
        | grep -oE '[0-9]+$' || true)
  {
    printf 'code=%d\n' "${rc}"
    printf 'duree_s=%d\n' "$((t1 - t0))"
    printf 'peak_rss_kb=%s\n' "${hwm:-inconnu}"
    printf 'timing_scope=scale_threads\n'
    printf 'threads_requested=%s\n' "${treq}"
    printf 'nproc=%d\n' "${NCPU}"
    printf 'cpu_set=%s\n' "${CPU_SET}"
    printf 'args_sha256=%s\n' "${args_sha}"
    printf 'source_commit=%s\n' "${SOURCE_COMMIT}"
    printf 'source_payload_sha256=%s\n' "${SOURCE_PAYLOAD_SHA256}"
    printf 'protocol_manifest_sha256=%s\n' "${PROTOCOL_MANIFEST_SHA256}"
    printf 'finished=1\n'
  } > "${status}.tmp"
  mv "${status}.tmp" "${status}"
  echo "--- fini ${name} (code=${rc}, $((t1 - t0))s, rss=${hwm:-?}kB)"
}

while IFS='|' read -r name fam tlabel; do
  # Chaque run teste lui-meme la deadline (le temps n'avance que dans un
  # sens : apres un premier refus, les suivants refusent aussi).
  if [ "${tlabel}" = "max" ]; then T="${NCPU}"; else T="${tlabel}"; fi
  run_one "${name}" "${T}" \
    --family="${fam}" --n="${N_OF_PHASE}" --s=8 --smax=11 --seed=3 \
    --min-balls=10000 --min-fusions=1000 --threads="${T}" --digest
done < <(phase_runs "${PHASE}")
echo "=== fin des runs scale_threads ${PHASE} (la validation locale decide du statut) ==="
