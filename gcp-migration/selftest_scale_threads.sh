#!/usr/bin/env bash
# PORTE TRANSACTIONNELLE du protocole scale_threads (audits 9223888 /
# b3a6eb4) : rejoue le runner distant et le validateur avec un FAUX
# probe, sans GCP ni vrai calcul. Six scenarios :
#   1. HAPPY PATH n32000 : 5 statuts, digests apparies -> complete ;
#   2. DIGEST DIFFERENT sous tmax (memes totaux) -> refuse (§ 2.2) ;
#   3. threads_effective=1 alors que t8 est demande -> refuse (§ 2.1) ;
#   4. nproc supprime d'un statut -> refuse ;
#   5. DEADLINE passee : statuts not_run_budget conserves, runner rc=0,
#      validateur -> partial (jamais un run ampute) ;
#   6. PREFLIGHT DE BUDGET du lanceur (mutant « ajouter du travail
#      sequentiel sans etendre le budget ») : MAX_RUN_SECONDS trop court,
#      RUN_TIMEOUT gonfle, ou MAX_RUN_SECONDS > 8 h -> REFUS code 2
#      AVANT toute action GCP.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
WORK="$(mktemp -d /tmp/ehgp-thrselftest.XXXXXXXX)"
trap 'rm -rf "${WORK}"' EXIT
BAD=0
fail() {
  echo "SELFTEST KO : $*" >&2
  BAD=1
}

cat > "${WORK}/fake_time" <<'EOF'
#!/usr/bin/env bash
out=""
while [ $# -gt 0 ]; do
  case "$1" in
    -v) shift ;;
    -o) out="$2"; shift 2 ;;
    *) break ;;
  esac
done
"$@"
rc=$?
[ -n "${out}" ] && echo "Maximum resident set size (kbytes): 12345" > "${out}"
exit "${rc}"
EOF
chmod +x "${WORK}/fake_time"

cat > "${WORK}/fake_probe" <<'EOF'
#!/usr/bin/env bash
fam=""; thr=1; dig=0
for a in "$@"; do
  case "$a" in
    --family=*) fam="${a#--family=}" ;;
    --threads=*) thr="${a#--threads=}" ;;
    --digest) dig=1 ;;
  esac
done
echo "famille=${fam} n=32000 boules_uniques=1000 evenements=500 fusions=400 noeuds=300 juge=off desaccords=NA t_gen_ms=1.0"
if [ -n "${FAKE_THREADS_EFFECTIVE_ONE:-}" ]; then
  echo "execution threads_effective=1"
else
  echo "execution threads_effective=${thr}"
fi
echo "cardinalites K=1 evenements=10 facettes=10 deltas=5 attachements=0 fusions=9"
echo "cardinalites K=2 evenements=20 facettes=20 deltas=9 attachements=3 fusions=19"
if [ "${dig}" -eq 1 ]; then
  h=$(printf 'objet-%s' "${fam}" | sha256sum | cut -c1-64)
  if [ -n "${FAKE_DIGEST_TMAX_DIFFERENT:-}" ] && [ "${thr}" = "$(nproc)" ]; then
    h=$(printf 'AUTRE-%s' "${fam}" | sha256sum | cut -c1-64)
  fi
  echo "digest_balls=${h}"
  echo "digest_forest_K1=${h}"
  echo "digest_all=${h}"
fi
exit 0
EOF
chmod +x "${WORK}/fake_probe"

FUTURE=$(( $(date +%s) + 360000 ))
run_campaign() {
  local out="$1" deadline="$2"
  shift 2
  env "$@" PROBE_BIN="${WORK}/fake_probe" TIME_BIN="${WORK}/fake_time" \
    OUT_DIR="${out}" RUN_TIMEOUT=60 RETRIEVE_MARGIN=10 \
    bash gcp-migration/v4_scale_threads_remote.sh c0mm1t payl0ad man1fest \
    n32000 "${deadline}"
}
validate() {
  local out="$1"
  python3 gcp-migration/validate_v4_scale_threads.py "${out}" c0mm1t payl0ad \
    man1fest 0 0 n32000
}

# ---- Scenario 1 : happy path.
run_campaign "${WORK}/out1" "${FUTURE}" >/dev/null || fail "scenario 1 : runner rc"
[ "$(ls "${WORK}/out1"/*.status | wc -l)" -eq 5 ] || fail "scenario 1 : 5 statuts attendus"
grep -q '^threads_requested=8$' "${WORK}/out1/thr_uniform_n32000_smax11_t8.status" ||
  fail "scenario 1 : threads_requested absent"
grep -q '^nproc=' "${WORK}/out1/thr_uniform_n32000_smax11_tmax.status" ||
  fail "scenario 1 : nproc absent"
validate "${WORK}/out1" > "${WORK}/v1.log" 2>&1 || fail "scenario 1 : happy path refuse ($(cat "${WORK}/v1.log"))"
grep -q 'CAMPAGNE SCALE_THREADS COMPLETE' "${WORK}/v1.log" || fail "scenario 1 : complete attendu"

# ---- Scenario 2 : digest different sous tmax, memes totaux.
run_campaign "${WORK}/out2" "${FUTURE}" FAKE_DIGEST_TMAX_DIFFERENT=1 >/dev/null ||
  fail "scenario 2 : runner rc"
if validate "${WORK}/out2" > "${WORK}/v2.log" 2>&1; then
  fail "scenario 2 : digest different accepte a tort"
fi
grep -q 'OBJETS DIFFERENTS' "${WORK}/v2.log" || fail "scenario 2 : motif OBJETS DIFFERENTS attendu"

# ---- Scenario 3 : le probe ignore --threads (threads_effective=1).
run_campaign "${WORK}/out3" "${FUTURE}" FAKE_THREADS_EFFECTIVE_ONE=1 >/dev/null ||
  fail "scenario 3 : runner rc"
if validate "${WORK}/out3" > "${WORK}/v3.log" 2>&1; then
  fail "scenario 3 : threads_effective incoherent accepte a tort"
fi
grep -q 'threads_effective' "${WORK}/v3.log" || fail "scenario 3 : motif threads_effective attendu"

# ---- Scenario 4 : nproc supprime d'un statut.
run_campaign "${WORK}/out4" "${FUTURE}" >/dev/null || fail "scenario 4 : runner rc"
sed -i '/^nproc=/d' "${WORK}/out4/thr_uniform_n32000_smax11_t8.status"
if validate "${WORK}/out4" > "${WORK}/v4.log" 2>&1; then
  fail "scenario 4 : statut sans nproc accepte a tort"
fi

# ---- Scenario 5 : deadline passee -> not_run_budget, runner rc=0,
# validation partielle.
run_campaign "${WORK}/out5" "$(date +%s)" > "${WORK}/r5.log" 2>&1 ||
  fail "scenario 5 : le runner doit rendre 0 (refus honnete, pas un crash)"
grep -q '^not_run_budget=1$' "${WORK}/out5/thr_uniform_n32000_smax11_t1.status" ||
  fail "scenario 5 : not_run_budget attendu"
if validate "${WORK}/out5" > "${WORK}/v5.log" 2>&1; then
  fail "scenario 5 : campagne non lancee acceptee a tort"
fi
grep -q 'non lance (budget)' "${WORK}/v5.log" || fail "scenario 5 : motif budget attendu"

# ---- Scenario 6 : preflight de budget du lanceur, AVANT toute action
# GCP (le lanceur sort en 2 sur la seule arithmetique — gcloud n'est
# jamais atteint ; mutant : du travail sequentiel en plus sans budget).
if MAX_RUN_SECONDS=10000 PHASE=n32000 bash gcp-migration/session_scale_threads_g4.sh \
     > "${WORK}/s6a.log" 2>&1; then
  fail "scenario 6a : budget trop court accepte a tort"
elif [ "$(MAX_RUN_SECONDS=10000 PHASE=n32000 bash gcp-migration/session_scale_threads_g4.sh > /dev/null 2>&1; echo $?)" -ne 2 ]; then
  fail "scenario 6a : code 2 attendu"
fi
grep -q 'REFUS : MAX_RUN_SECONDS' "${WORK}/s6a.log" || fail "scenario 6a : motif REFUS attendu"
if RUN_TIMEOUT=7200 PHASE=n32000 bash gcp-migration/session_scale_threads_g4.sh \
     > "${WORK}/s6b.log" 2>&1; then
  fail "scenario 6b : mutant travail-en-plus-sans-budget accepte a tort"
fi
grep -q 'REFUS' "${WORK}/s6b.log" || fail "scenario 6b : motif REFUS attendu"
if MAX_RUN_SECONDS=30000 GUEST_SHUTDOWN_MINUTES=520 SSH_KEY_TTL_MINUTES=540 \
     PHASE=n32000 bash gcp-migration/session_scale_threads_g4.sh \
     > "${WORK}/s6c.log" 2>&1; then
  fail "scenario 6c : session > 8 h acceptee a tort"
fi
grep -q 'REFUS : MAX_RUN_SECONDS > 8 h' "${WORK}/s6c.log" || fail "scenario 6c : garde 8 h attendue"

echo "selftest_scale_threads : violations=${BAD}"
[ "${BAD}" -eq 0 ] || exit 1
echo "PROTOCOLE CONFORME"
