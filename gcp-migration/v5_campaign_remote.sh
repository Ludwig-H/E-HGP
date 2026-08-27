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

test -x "${TIME_BIN}" || {
  echo "REFUS : GNU time requis (${TIME_BIN}) pour une campagne a RSS mesure" >&2
  exit 2
}
test -f "${RECEIPT}" || { echo "REFUS : reçu de conformite absent (${RECEIPT})" >&2; exit 2; }
mkdir -p "${OUT_DIR}"

run_one() {
  local name="$1" scope="$2"; shift 2
  local out="${OUT_DIR}/${name}.txt" status="${OUT_DIR}/${name}.status"
  local rc=0 t0 t1 hwm=""
  t0=$(date +%s)
  "${TIME_BIN}" -v -o "${status}.time" timeout "${RUN_TIMEOUT}" "$@" >"${out}" 2>&1 || rc=$?
  t1=$(date +%s)
  hwm=$(grep -oE 'Maximum resident set size[^0-9]*[0-9]+' "${status}.time" 2>/dev/null | grep -oE '[0-9]+$' || true)
  {
    printf 'code=%d\n' "${rc}"
    printf 'duree_s=%d\n' "$((t1 - t0))"
    printf 'peak_rss_kb=%s\n' "${hwm:-inconnu}"
    printf 'timing_scope=%s\n' "${scope}"
    printf 'threads=%s\n' "${THREADS}"
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
  run_one gpu_witness device_witness bash -c "set -e; echo nvcc=${NVCC_BIN}; uname -m; ${NVCC_BIN} --version 2>&1 | tail -2; nvidia-smi --query-gpu=name,driver_version --format=csv,noheader 2>&1 | head -1; cmake -S morsehgp3D_v5 -B build-cuda -DCMAKE_BUILD_TYPE=Release -DMHGP5_ENABLE_CUDA=ON -DCMAKE_CUDA_COMPILER=${NVCC_BIN} 2>&1 | tail -40; test \${PIPESTATUS[0]} -eq 0; cmake --build build-cuda --target mhgp5_device_witness -j8 2>&1 | grep -vE '^\[|^Scanning' | tail -60; test \${PIPESTATUS[0]} -eq 0; ./build-cuda/mhgp5_device_witness"
  # Refus IMMEDIAT des phases suivantes si le temoin n'est pas conforme
  # (P1 audit 9762daaf) : aucune conformite ni contrat sur une VM dont le
  # device n'est pas prouve ; le statut du temoin reste grave.
  if ! grep -q '^code=0$' "${OUT_DIR}/gpu_witness.status"; then
    echo "REFUS : temoin device non conforme (voir gpu_witness.txt) — phases 1 et 2 refusees"
    exit 3
  fi
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
echo "=== fin des runs (la validation locale decide du statut, jamais cette ligne) ==="
