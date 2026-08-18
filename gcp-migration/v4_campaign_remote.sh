#!/usr/bin/env bash
# Script DISTANT de la campagne v4 — execute sur la VM par la session
# gardee (ou localement par le selftest transactionnel, avec un faux
# probe). Il ne demarre ni n'arrete rien : la session locale detient les
# gardes. $1 = source_commit, $2 = source_tar_sha256 — graves dans CHAQUE
# .status avec $3 = protocol_manifest_sha256 (audit « pin du protocole » :
# le reçu doit savoir quel code exact ET quel protocole exact ont produit
# ses compteurs).
#
# TRANSACTIONNEL : chaque run ecrit deux fichiers atomiques (.txt payload,
# .status code/duree/pic RSS/scope/pin), errexit desarme autour du run.
# GNU time est OBLIGATOIRE (audit « pin source » § 2 : le repli VmHWM
# mesurait le wrapper timeout, pas le probe — supprime, pas de mesure
# de memoire non testee). PILOTES : la phase de couverture est REFUSEE
# (exit 3, statuts conserves) si un pilote RSS n'a pas code=0, finished=1
# et un pic numerique > 0 — jamais de concurrence derivee d'un processus
# avorte.
set -euo pipefail

SOURCE_COMMIT="${1:?source_commit requis}"
SOURCE_PAYLOAD_SHA256="${2:?source_payload_sha256 requis}"
PROTOCOL_MANIFEST_SHA256="${3:?protocol_manifest_sha256 requis}"
PROBE_BIN="${PROBE_BIN:-./build/mhgp4_forest_probe}"
OUT_DIR="${OUT_DIR:-out}"
RUN_TIMEOUT="${RUN_TIMEOUT:-10800}"
TIME_BIN="${TIME_BIN:-/usr/bin/time}"
SLEEP_S="${SLEEP_S:-3}"

test -x "${TIME_BIN}" || {
  echo "REFUS : GNU time requis (${TIME_BIN}) pour une campagne RSS-pilotee" >&2
  exit 2
}
mkdir -p "${OUT_DIR}"
export OMP_NUM_THREADS=1
NCPU=$(nproc)

run_one() {
  local name="$1" scope="$2" cpu="$3"; shift 3
  local out="${OUT_DIR}/${name}.txt" status="${OUT_DIR}/${name}.status"
  local rc=0 t0 t1 hwm=""
  t0=$(date +%s)
  "${TIME_BIN}" -v -o "${status}.time" \
    timeout "${RUN_TIMEOUT}" taskset -c "${cpu}" "${PROBE_BIN}" "$@" \
    >"${out}" 2>&1 || rc=$?
  t1=$(date +%s)
  hwm=$(grep -oE 'Maximum resident set size[^0-9]*[0-9]+' "${status}.time" 2>/dev/null \
        | grep -oE '[0-9]+$' || true)
  {
    printf 'code=%d\n' "${rc}"
    printf 'duree_s=%d\n' "$((t1 - t0))"
    printf 'peak_rss_kb=%s\n' "${hwm:-inconnu}"
    printf 'timing_scope=%s\n' "${scope}"
    printf 'source_commit=%s\n' "${SOURCE_COMMIT}"
    printf 'source_payload_sha256=%s\n' "${SOURCE_PAYLOAD_SHA256}"
    printf 'protocol_manifest_sha256=%s\n' "${PROTOCOL_MANIFEST_SHA256}"
    printf 'finished=1\n'
  } > "${status}.tmp"
  mv "${status}.tmp" "${status}"
  echo "--- fini ${name} (code=${rc}, $((t1 - t0))s, rss=${hwm:-?}kB)"
  return 0
}

# PHASE 1 — LATENCE ISOLEE (un run a la fois, cœur fixe — borne par nproc
# pour que la porte a faux probe tourne aussi sur un petit poste). Les pics
# RSS mesures ici pilotent la concurrence de la phase 2.
run_one lat_uniform_n8000_smax11_rep1 isolated_latency $((2 % NCPU)) \
  --family=uniform --n=8000 --s=8 --smax=11 --seed=3 --min-balls=10000 --min-fusions=1000
run_one lat_uniform_n8000_smax11_rep2 isolated_latency $((2 % NCPU)) \
  --family=uniform --n=8000 --s=8 --smax=11 --seed=3 --min-balls=10000 --min-fusions=1000
run_one lat_uniform_n16000_smax11 isolated_latency $((2 % NCPU)) \
  --family=uniform --n=16000 --s=8 --smax=11 --seed=3 --min-balls=10000 --min-fusions=1000
run_one lat_uniform_n32000_smax11 isolated_latency $((2 % NCPU)) \
  --family=uniform --n=32000 --s=8 --smax=11 --seed=3 --min-balls=10000 --min-fusions=1000

# PORTE DES PILOTES (audit « rupture SSH » § 3) : jamais une couverture
# sur une estimation memoire issue d'un processus avorte.
pilot_ok() {
  local status="$1"
  [ -f "${status}" ] || return 1
  grep -q '^code=0$' "${status}" || return 1
  grep -q '^finished=1$' "${status}" || return 1
  grep -q '^timing_scope=isolated_latency$' "${status}" || return 1
  local kb
  kb=$(grep -oE '^peak_rss_kb=[0-9]+$' "${status}" | grep -oE '[0-9]+$' || true)
  [ -n "${kb}" ] && [ "${kb}" -gt 0 ]
}
for p in lat_uniform_n8000_smax11_rep1 lat_uniform_n16000_smax11 \
         lat_uniform_n32000_smax11; do
  if ! pilot_ok "${OUT_DIR}/${p}.status"; then
    echo "PILOTE INVALIDE : ${p} — couverture refusee, statuts conserves" >&2
    exit 3
  fi
done

# Concurrence par taille : min(cap prudent, 0.75*RAM/pic) — audit § 3.
conc_for() {
  local status="$1" cap="$2"
  local kb mem c
  kb=$(grep -oE '^peak_rss_kb=[0-9]+$' "${status}" | grep -oE '[0-9]+$' || true)
  mem=$(grep MemTotal /proc/meminfo | awk '{print $2}')
  if [ -z "${kb}" ] || [ "${kb}" -eq 0 ]; then echo 1; return; fi
  c=$(( (mem * 3 / 4) / kb ))
  [ "${c}" -lt 1 ] && c=1
  [ "${c}" -gt "${cap}" ] && c=${cap}
  echo "${c}"
}
C8=$(conc_for "${OUT_DIR}/lat_uniform_n8000_smax11_rep1.status" 8)
C16=$(conc_for "${OUT_DIR}/lat_uniform_n16000_smax11.status" 4)
C32=$(conc_for "${OUT_DIR}/lat_uniform_n32000_smax11.status" 2)
echo "concurrences pilotees par la memoire : n8000=${C8} n16000=${C16} n32000=${C32}"

# PHASE 2 — COUVERTURE CONTENDUE, par vagues de taille croissante ; les
# gros runs ne demarrent jamais tous au meme instant.
wave() {
  local n="$1" conc="$2"
  local i=0 cpu
  for fam in uniform terrain eight_clusters scanline_overlap_multiecho; do
    for smax in 11 6; do
      cpu=$(( (4 + (i % conc) * 2) % NCPU ))
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
echo "=== fin des runs (la validation locale decide du statut, jamais cette ligne) ==="
