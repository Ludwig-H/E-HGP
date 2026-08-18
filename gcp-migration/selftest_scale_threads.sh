#!/usr/bin/env bash
# PORTE TRANSACTIONNELLE du protocole scale_threads (audits 9223888 /
# b3a6eb4, durcie par 66886c0) : rejoue le runner distant et le
# validateur avec un FAUX probe, sans GCP ni vrai calcul. Le faux probe
# publie le SCHEMA COMPLET de production (identite depuis l'argv,
# workers mesures, digest_balls + K=1..10 + all, cardinalites K=1..10) —
# jamais une miniature. Scenarios :
#   1. HAPPY PATH n32000 : 5 statuts, apparies -> complete ;
#   2. DIGEST DIFFERENT sous tmax (memes totaux) -> refuse ;
#   3. WORKERS : le noyau ignore les fils (gen_workers_max=1 sous une
#      CLI et des digests corrects) -> refuse ;
#   4. nproc supprime d'un statut -> refuse ;
#   5. DEADLINE passee : not_run_budget conserves, runner rc=0,
#      validateur -> partial ;
#   6. PREFLIGHT DE BUDGET du lanceur (budget court, mutant
#      travail-en-plus-sans-budget, garde 8 h) -> trois REFUS code 2 ;
#   7. IDENTITE : wrong-n / wrong-family / wrong-smax sous le bon nom
#      (66886c0 § 1) -> refuses ;
#   8. args_sha256 trafique dans un statut -> refuse (hash recalcule) ;
#   9. SCHEMA : omit-digest-K7 / duplicate-digest-K3 / omit-cardinality-K9
#      (66886c0 § 3) -> refuses.
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

# Le faux probe derive son identite de l'ARGV (comme le vrai) : une
# regression du runner se propage naturellement a la ligne d'identite.
cat > "${WORK}/fake_probe" <<'EOF'
#!/usr/bin/env bash
fam=""; n=0; s=0; smax=0; seed=0; thr=1; dig=0
for a in "$@"; do
  case "$a" in
    --family=*) fam="${a#--family=}" ;;
    --n=*) n="${a#--n=}" ;;
    --s=*) s="${a#--s=}" ;;
    --smax=*) smax="${a#--smax=}" ;;
    --seed=*) seed="${a#--seed=}" ;;
    --threads=*) thr="${a#--threads=}" ;;
    --digest) dig=1 ;;
  esac
done
[ -n "${FAKE_WRONG_N:-}" ] && n="${FAKE_WRONG_N}"
[ -n "${FAKE_WRONG_FAMILY:-}" ] && fam="${FAKE_WRONG_FAMILY}"
[ -n "${FAKE_WRONG_SMAX:-}" ] && smax="${FAKE_WRONG_SMAX}"
echo "famille=${fam} n=${n} s=${s} smax=${smax} seed=${seed} boules_uniques=1000 evenements=500 fusions=400 noeuds=300 juge=off desaccords=NA t_gen_ms=1.0"
gen="${thr}"
[ -n "${FAKE_ONE_WORKER:-}" ] && gen=1
fold=$(( thr < 10 ? thr : 10 ))
# Affinite REELLE du processus (le runner nous lance sous taskset -c
# CPU_SET, donc masque == contractuel) ; mutant cpu-set-one-core : une
# experience « N fils » enfermee sur un cœur.
AFFMASK="$(taskset -pc $$ 2>/dev/null | sed 's/.*: //')"
AFFN="$(python3 -c "
n = 0
for p in '${AFFMASK}'.split(','):
    if '-' in p:
        a, b = p.split('-')
        n += int(b) - int(a) + 1
    else:
        n += 1
print(n)
")"
if [ -n "${FAKE_ONE_CORE_AFFINITY:-}" ]; then AFFN=1; AFFMASK=0; fi
echo "execution threads_requested=${thr} gen_workers_q2=${gen} gen_workers_q3=${gen} gen_workers_q4=${gen} gen_workers_max=${gen} prefilter_workers=${thr} census_workers=${thr} expansion_workers=${thr} fold_workers_max=${fold} affinity_cpus_effective=${AFFN} affinity_mask=${AFFMASK}"
for k in $(seq 1 10); do
  if [ "${k}" = "9" ] && [ -n "${FAKE_OMIT_CARD_K9:-}" ]; then continue; fi
  echo "cardinalites K=${k} evenements=10 facettes=10 deltas=5 attachements=0 fusions=9"
done
if [ "${dig}" -eq 1 ]; then
  h() { printf 'objet-%s-%s-%s' "${fam}" "${n}" "$1" | sha256sum | cut -c1-64; }
  hb=$(h balls)
  if [ -n "${FAKE_DIGEST_TMAX_DIFFERENT:-}" ] && [ "${thr}" = "$(nproc)" ]; then
    hb=$(printf 'AUTRE-%s' "${fam}" | sha256sum | cut -c1-64)
  fi
  echo "digest_balls=${hb}"
  for k in $(seq 1 10); do
    if [ "${k}" = "7" ] && [ -n "${FAKE_OMIT_DIGEST_K7:-}" ]; then continue; fi
    echo "digest_forest_K${k}=$(h "K${k}")"
    if [ "${k}" = "3" ] && [ -n "${FAKE_DUP_DIGEST_K3:-}" ]; then
      echo "digest_forest_K3=$(h K3)"
    fi
  done
  echo "digest_all=$(h all)"
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
validate "${WORK}/out1" > "${WORK}/v1.log" 2>&1 || fail "scenario 1 : happy path refuse ($(cat "${WORK}/v1.log"))"
grep -q 'thread_equivalence_checked' "${WORK}/v1.log" || fail "scenario 1 : etiquette d'equivalence attendue"

# ---- Scenario 2 : digest different sous tmax, memes totaux.
run_campaign "${WORK}/out2" "${FUTURE}" FAKE_DIGEST_TMAX_DIFFERENT=1 >/dev/null ||
  fail "scenario 2 : runner rc"
if validate "${WORK}/out2" > "${WORK}/v2.log" 2>&1; then
  fail "scenario 2 : digest different accepte a tort"
fi
grep -q 'OBJETS DIFFERENTS' "${WORK}/v2.log" || fail "scenario 2 : motif OBJETS DIFFERENTS attendu"

# ---- Scenario 3 : le noyau ignore les fils — CLI et digests corrects,
# seule la mesure gen_workers_max le trahit (66886c0 § 2).
run_campaign "${WORK}/out3" "${FUTURE}" FAKE_ONE_WORKER=1 >/dev/null ||
  fail "scenario 3 : runner rc"
if validate "${WORK}/out3" > "${WORK}/v3.log" 2>&1; then
  fail "scenario 3 : gen_workers_max=1 accepte a tort"
fi
grep -q 'gen_workers_q' "${WORK}/v3.log" || fail "scenario 3 : motif gen_workers_q attendu"

# ---- Scenario 4 : nproc supprime d'un statut.
run_campaign "${WORK}/out4" "${FUTURE}" >/dev/null || fail "scenario 4 : runner rc"
sed -i '/^nproc=/d' "${WORK}/out4/thr_uniform_n32000_smax11_t8.status"
if validate "${WORK}/out4" > "${WORK}/v4.log" 2>&1; then
  fail "scenario 4 : statut sans nproc accepte a tort"
fi

# ---- Scenario 5 : deadline passee -> not_run_budget, validation partielle.
run_campaign "${WORK}/out5" "$(date +%s)" > "${WORK}/r5.log" 2>&1 ||
  fail "scenario 5 : le runner doit rendre 0 (refus honnete, pas un crash)"
grep -q '^not_run_budget=1$' "${WORK}/out5/thr_uniform_n32000_smax11_t1.status" ||
  fail "scenario 5 : not_run_budget attendu"
if validate "${WORK}/out5" > "${WORK}/v5.log" 2>&1; then
  fail "scenario 5 : campagne non lancee acceptee a tort"
fi
grep -q 'non lance (budget)' "${WORK}/v5.log" || fail "scenario 5 : motif budget attendu"

# ---- Scenario 6 : preflight de budget du lanceur, AVANT toute action GCP.
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
# 6d : la garde 5 de start_and_verify (invite + 300 s <= maxRunDuration)
# est modelisee AU preflight — le refus du 18 aout (57 min @ 3600 s)
# doit tomber ICI, avant toute action GCP.
if MAX_RUN_SECONDS=3600 GUEST_SHUTDOWN_MINUTES=57 SSH_KEY_TTL_MINUTES=68 \
     RUN_TIMEOUT=600 BUILD_MARGIN=480 RETRIEVE_MARGIN=300 PHASE=n64000 \
     bash gcp-migration/session_scale_threads_g4.sh > "${WORK}/s6d.log" 2>&1; then
  fail "scenario 6d : invite 57 min @ 3600 s accepte a tort"
fi
grep -q 'garde 5' "${WORK}/s6d.log" || fail "scenario 6d : motif garde 5 attendu"
# 6e : la garde 6 (fenetre de TTL des DEUX cotes) — un TTL trop long est
# refuse autant qu'un trop court.
if MAX_RUN_SECONDS=3600 GUEST_SHUTDOWN_MINUTES=55 SSH_KEY_TTL_MINUTES=75 \
     RUN_TIMEOUT=600 BUILD_MARGIN=480 RETRIEVE_MARGIN=300 PHASE=n64000 \
     bash gcp-migration/session_scale_threads_g4.sh > "${WORK}/s6e.log" 2>&1; then
  fail "scenario 6e : TTL 75 min @ 3600 s accepte a tort"
fi
grep -q 'garde 6' "${WORK}/s6e.log" || fail "scenario 6e : motif garde 6 attendu"

# ---- Scenario 7 : la mauvaise experience sous le bon nom (66886c0 § 1).
for c in "FAKE_WRONG_N=16000:wrong-n" "FAKE_WRONG_FAMILY=uniform:wrong-family" \
         "FAKE_WRONG_SMAX=6:wrong-smax"; do
  envkv="${c%%:*}"; label="${c##*:}"
  run_campaign "${WORK}/out7_${label}" "${FUTURE}" "${envkv}" >/dev/null ||
    fail "scenario 7 (${label}) : runner rc"
  if validate "${WORK}/out7_${label}" > "${WORK}/v7_${label}.log" 2>&1; then
    fail "scenario 7 (${label}) : identite fausse acceptee a tort"
  fi
  grep -q 'identite imprimee' "${WORK}/v7_${label}.log" ||
    fail "scenario 7 (${label}) : motif identite attendu"
done

# ---- Scenario 8 : args_sha256 trafique — le validateur RECALCULE.
run_campaign "${WORK}/out8" "${FUTURE}" >/dev/null || fail "scenario 8 : runner rc"
sed -i 's/^args_sha256=.*/args_sha256=0000000000000000000000000000000000000000000000000000000000000000/' \
  "${WORK}/out8/thr_uniform_n32000_smax11_t1.status"
if validate "${WORK}/out8" > "${WORK}/v8.log" 2>&1; then
  fail "scenario 8 : args_sha256 trafique accepte a tort"
fi
grep -q 'argv contractuel' "${WORK}/v8.log" || fail "scenario 8 : motif argv attendu"

# ---- Scenario 9 : schema incomplet ou double (66886c0 § 3).
for c in "FAKE_OMIT_DIGEST_K7=1:omit-digest-K7:digest forest_K7" \
         "FAKE_DUP_DIGEST_K3=1:duplicate-digest-K3:digest forest_K3" \
         "FAKE_OMIT_CARD_K9=1:omit-cardinality-K9:cardinalites K=9"; do
  envkv="${c%%:*}"; rest="${c#*:}"; label="${rest%%:*}"; motif="${rest#*:}"
  run_campaign "${WORK}/out9_${label}" "${FUTURE}" "${envkv}" >/dev/null ||
    fail "scenario 9 (${label}) : runner rc"
  if validate "${WORK}/out9_${label}" > "${WORK}/v9_${label}.log" 2>&1; then
    fail "scenario 9 (${label}) : schema degrade accepte a tort"
  fi
  grep -q "${motif}" "${WORK}/v9_${label}.log" ||
    fail "scenario 9 (${label}) : motif « ${motif} » attendu"
done

# ---- Scenario 10 : phase courte court1h (directive <= 1 h) — 4 runs,
# paires t8/tmax, etiquette d'equivalence restreinte honnete.
env PROBE_BIN="${WORK}/fake_probe" TIME_BIN="${WORK}/fake_time" \
  OUT_DIR="${WORK}/out10" RUN_TIMEOUT=60 RETRIEVE_MARGIN=10 \
  bash gcp-migration/v4_scale_threads_remote.sh c0mm1t payl0ad man1fest \
  court1h "${FUTURE}" >/dev/null || fail "scenario 10 : runner rc"
[ "$(ls "${WORK}/out10"/*.status | wc -l)" -eq 4 ] || fail "scenario 10 : 4 statuts attendus"
python3 gcp-migration/validate_v4_scale_threads.py "${WORK}/out10" c0mm1t \
  payl0ad man1fest 0 0 court1h > "${WORK}/v10.log" 2>&1 ||
  fail "scenario 10 : happy path court1h refuse ($(cat "${WORK}/v10.log"))"
grep -q 'thread_equivalence_checked_t8_tmax' "${WORK}/v10.log" ||
  fail "scenario 10 : etiquette t8_tmax attendue"

# ---- Scenario 11 : cpu-set-one-core (audit c9c3a48 § 3) — workers et
# digests corrects, mais l'affinite effective publiee vaut 1 : le
# validateur doit refuser avec un motif d'affinite explicite.
run_campaign "${WORK}/out11" "${FUTURE}" FAKE_ONE_CORE_AFFINITY=1 >/dev/null ||
  fail "scenario 11 : runner rc"
if validate "${WORK}/out11" > "${WORK}/v11.log" 2>&1; then
  fail "scenario 11 : affinite un cœur acceptee a tort"
fi
grep -q 'affinite effective' "${WORK}/v11.log" ||
  fail "scenario 11 : motif d'affinite attendu"

echo "selftest_scale_threads : violations=${BAD}"
[ "${BAD}" -eq 0 ] || exit 1
echo "PROTOCOLE CONFORME"
