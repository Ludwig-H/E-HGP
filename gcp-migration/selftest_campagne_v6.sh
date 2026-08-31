#!/usr/bin/env bash
# SELFTEST TRANSACTIONNEL de la campagne v6 — A LANCER A LA MAIN avant toute
# session payante (jamais depuis la CI). Ne touche JAMAIS GCP : il execute le
# runner distant (gcp-migration/v6_campaign_remote.sh) en local avec de FAUX
# pilotes (mhgp5 / mhgp6 / mhgp6_conformity deterministes) et un faux GNU
# time, puis juge le validateur (validate_v6_campaign.py) sur le nominal et
# sur des falsifications — chacune doit etre REFUSEE.
#
# Scenarios :
#   1. nominal : runner rc=0, validateur rc=0 (=== CAMPAGNE COMPLETE ===),
#      plan ABBA contrebalance verifie (config 1 : v5 v6 v6 v5 ; config 2 :
#      v6 v5 v5 v6), resumes ecrits ;
#   2. GNU time absent : runner rc=2 avant tout run ;
#   3. parametre mal forme : runner rc=2 avant tout run ;
#   4. conformite en echec : runner rc=3, AUCUN run de bench, validateur rc=1 ;
#   5. pilote de bench en echec : bench TRONQUE (grave), queue executee,
#      validateur rc=1 ;
#   6. echeance passee : troncature avant le premier run, runner rc=3 ;
#   7. falsifications du dossier rapatrie (statut supprime, pin altere,
#      digest sur un run de bench, compteurs non deterministes, fichier en
#      trop, plan altere, remote_rc non nul, compteur de queue absent) :
#      validateur rc=1 sur chacune.
# Code de sortie : 0 conforme, 1 au moins un scenario en echec.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUNNER="${HERE}/v6_campaign_remote.sh"
VALIDATOR="${HERE}/validate_v6_campaign.py"
WORK="$(mktemp -d /tmp/ehgp-v6selftest.XXXXXXXX)"
trap 'rm -rf "${WORK}"' EXIT
FAKE="${WORK}/fake"
mkdir -p "${FAKE}"

PIN_COMMIT="0000000000000000000000000000000000000000"
PIN_PAYLOAD="1111111111111111111111111111111111111111111111111111111111111111"
PIN_MANIFEST="2222222222222222222222222222222222222222222222222222222222222222"

FAILURES=0
check() { # check NOM CODE(0=ok)
  local name="$1" rc="$2"
  if [ "${rc}" -ne 0 ]; then
    echo "ECHEC selftest : ${name}" >&2
    FAILURES=$((FAILURES + 1))
  else
    echo "ok : ${name}"
  fi
}
check_true() { # check_true NOM CMD... — `if` neutralise errexit sur la condition
  local name="$1"; shift
  if "$@"; then check "${name}" 0; else check "${name}" 1; fi
}

# ---- Faux GNU time : grave un pic RSS puis execute la commande.
cat > "${FAKE}/gnutime" <<'EOF'
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
{
  echo 'Command being timed: (faux GNU time)'
  echo '        Maximum resident set size (kbytes): 123456'
  if [ "${rc}" -ge 128 ]; then
    echo "Command terminated by signal $((rc - 128))"
  else
    echo "Exit status: ${rc}"
  fi
} > "${out}"
exit ${rc}
EOF

# ---- Faux pilotes v5 / v6 : sortie DETERMINISTE par (famille, n, graine),
# memes lignes contractuelles que les vrais (identite+compteurs, tower_scope,
# generation, cardinalites, temps, digest seulement sous --digest ; le faux
# v6 ajoute le grand-livre : sweep, vwspd, octaves_q4, vcensus, p_factor,
# ledger_paires).
make_fake_pilot() { # $1 = chemin, $2 = v5|v6
  local path="$1" eng="$2"
  cat > "${path}" <<EOF
#!/usr/bin/env bash
fam=""; n=0; seed=3; threads=8; digest=0
for a in "\$@"; do
  case "\$a" in
    --family=*) fam="\${a#*=}" ;;
    --n=*) n="\${a#*=}" ;;
    --seed=*) seed="\${a#*=}" ;;
    --threads=*) threads="\${a#*=}" ;;
    --digest) digest=1 ;;
  esac
done
if [ "${eng}" = "v5" ] && [ "\${FAKE_V5_BENCH_FAIL:-0}" = "1" ] && [ "\${digest}" = "0" ]; then
  echo "erreur simulee du pilote v5"; exit 7
fi
if [ "${eng}" = "v6" ] && [ "\${FAKE_V6_FRONT_FAIL:-0}" = "1" ] && [ "\${n}" = "400000" ]; then
  echo "terminate called after throwing an instance of 'std::bad_alloc'"
  kill -ABRT \$\$
fi
if [ "${eng}" = "v6" ] && [ "\${FAKE_V6_FRONT_FAIL:-0}" = "1" ] && [ "\${n}" = "800000" ]; then
  echo "REFUS resource_exhausted : plafond simule du prefiltre" >&2; exit 2
fi
base=\$((n / 4))
echo "payload=mhgp${eng#v}-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none"
echo "backend=cpu_reference"
echo "tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11"
echo "famille=\${fam} n=\${n} coord=200 s=8 smax=11 seed=\${seed} threads=\${threads} emis=\${base} boules_uniques=\${base} mortes_profondeur=10 survivantes=\$((base - 10)) census_int=50 census_shell=20 evenements=\${base} facettes=\${base} fusions=10 deltas=10 noeuds=10"
echo "generation rect_alive=\${base}/\${base}/\${base} rect_visites_fusionnes=\${base} ancres=\${base}/\${base}/\${base} candidats=\${base}/\${base}/\${base}"
if [ "${eng}" = "v6" ]; then
  echo "sweep tests_coeur=\${base} tests_prof_q3=\${base} tests_passe2=\${base} tri_comparaisons=\${base} seeds_passe2=\${base} racines_corde=\${base} groupes=\${base} racines_hors_corde=\${base} temoins_constants=1 rejets=lens:1/owner:1/once:1/i64:1/face:1/det:0/centre:1"
  echo "vwspd nœuds_temoins=\${base} coins=\${base} h_rect=1/1/\${base} h_scan=0/1/\${base} m_anchor=1/1/\${base} entrees_ancres=0/1/\${base} iters_coeur=\${base} iters_passe2=\${base}"
  echo "octaves_q4 ancres=\${base},0,0,0 seeds=\${base},0,0,0 w1=\${base},0,0,0 (octave = log2 de la taille du cover de l'ancre)"
  echo "octaves_q4_seeds cellules=0,0,0,0 coeur=0,0,0,0 corde=0,0,0,0 passe2=\${base},0,0,0"
  echo "vcensus prefiltre_nœuds=\${base} prefiltre_feuilles=0 range_add=1 census_nœuds=\${base} census_feuilles=1"
  echo "p_factor=1/1/\${base} (evaluations d'auto-produits des histogrammes)"
  echo "ledger_paires emis=1/1/1 tues=1/1/1"
fi
echo "ouvriers wspd=\${threads} rects=\${threads} rle=\${threads} prefiltre=\${threads} census=\${threads} expansion=\${threads} fold=\${threads}"
for k in 1 2 3 4 5 6 7 8 9 10; do
  echo "cardinalites K=\${k} evenements=\${base} facettes=\${base} deltas=1 attachements=0 fusions=1 noeuds=1"
done
echo "temps_mur_ms=1234.5 (etages A et B du fold pipelines)"
echo "temps_fold_mur_ms=100.0 (etages A et B, fold_inflight=1, pic_mesure_en_vol=1)"
if [ "\${digest}" = "1" ]; then
  echo "digest_balls=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  for k in 1 2 3 4 5 6 7 8 9 10; do
    echo "digest_forest_K\${k}=cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
  done
  echo "digest_all=dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
fi
exit 0
EOF
  chmod +x "${path}"
}
make_fake_pilot "${FAKE}/mhgp5" v5
make_fake_pilot "${FAKE}/mhgp6" v6

# ---- Faux juge de conformite : exige la reference (fichier + digest_all),
# imprime la ligne contractuelle.
cat > "${FAKE}/mhgp6_conformity" <<'EOF'
#!/usr/bin/env bash
fam=""; n=0; expected=""
for a in "$@"; do
  case "$a" in
    --family=*) fam="${a#*=}" ;;
    --n=*) n="${a#*=}" ;;
    --expected=*) expected="${a#*=}" ;;
  esac
done
[ -f "${expected}" ] || { echo "reference absente : ${expected}" >&2; exit 2; }
grep -q '^digest_all=' "${expected}" || { echo "reference sans digest_all" >&2; exit 2; }
if [ "${FAKE_CONF_FAIL:-0}" = "1" ]; then
  echo "desaccord simule du juge"; exit 1
fi
echo "conformite v5=v6 : ${fam} n=${n} : 10 forets + digest_all identiques (objet)"
exit 0
EOF
chmod +x "${FAKE}/gnutime" "${FAKE}/mhgp6_conformity"

# ---- Faux outillage GPU : la phase GPU du runner est exercee de bout en
# bout (temoin + mutant + lanes + contrats a digests egaux) sans CUDA.
cat > "${FAKE}/nvcc" <<'EOF'
#!/usr/bin/env bash
echo "Cuda compilation tools, release 12.9, V12.9.41"
echo "Build cuda_12.9.r12.9/compiler.36037853_0"
EOF
cat > "${FAKE}/nvidia-smi" <<'EOF'
#!/usr/bin/env bash
echo "NVIDIA RTX PRO 6000 Blackwell Server Edition, 580.65.06"
EOF
cat > "${FAKE}/cmake" <<'EOF'
#!/usr/bin/env bash
exit 0
EOF
cat > "${FAKE}/mhgp5_device_witness" <<'EOF'
#!/usr/bin/env bash
if [ "${1:-}" = "--inject=witness-no-warp-correction" ]; then
  echo "DESACCORD device/hote (mutant temoin, correction de warp retiree)"; exit 4
fi
echo "arith cas=4096 desaccords=0"
echo "scan famille=uniform ancres=64 seeds=2000 sites=1000 morts=40 desaccords=0 kernel_ms=1.2"
echo "scan famille=eight_clusters ancres=64 seeds=2000 sites=1000 morts=40 desaccords=0 kernel_ms=1.3"
echo "device_witness OK"
EOF
make_fake_lane_gate() { # $1 = q3|q4
  cat > "${FAKE}/mhgp5_${1}_lane_device_gate" <<EOF
#!/usr/bin/env bash
fam=""; n=0; fils=1; wire=""
for a in "\$@"; do
  case "\$a" in
    --family=*) fam="\${a#*=}" ;;
    --n=*) n="\${a#*=}" ;;
    --threads=*) fils="\${a#*=}" ;;
    --wire=index) wire=" wire=index" ;;
  esac
done
cand=300; [ "\${n}" = "8000" ] && cand=150000
echo "${1}_lane_device famille=\${fam} n=\${n} fils=\${fils} candidats_${1}=\${cand} seeds=5 tues=3 lancements=2\${wire} desaccords_vecteur=0 desaccords_compteurs=0"
echo "${1}_lane_device OK"
EOF
  chmod +x "${FAKE}/mhgp5_${1}_lane_device_gate"
}
make_fake_lane_gate q3
make_fake_lane_gate q4
cat > "${FAKE}/mhgp5_cuda" <<'EOF'
#!/usr/bin/env bash
fam=""; n=0; seed=3; threads=8; digest=0; ms=1
for a in "$@"; do
  case "$a" in
    --family=*) fam="${a#*=}" ;;
    --n=*) n="${a#*=}" ;;
    --seed=*) seed="${a#*=}" ;;
    --threads=*) threads="${a#*=}" ;;
    --digest) digest=1 ;;
    --gpu-min-sites=*) ms="${a#*=}" ;;
  esac
done
base=$((n / 4))
echo "payload=mhgp5-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none"
echo "backend=override_experimental (gpu demande, jamais autoritaire)"
echo "tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11"
echo "famille=${fam} n=${n} coord=200 s=8 smax=11 seed=${seed} threads=${threads} emis=${base} boules_uniques=${base} mortes_profondeur=10 survivantes=$((base - 10)) census_int=50 census_shell=20 evenements=${base} facettes=${base} fusions=10 deltas=10 noeuds=10"
echo "generation rect_alive=${base}/${base}/${base} rect_visites_fusionnes=${base} ancres=${base}/${base}/${base} candidats=${base}/${base}/${base}"
echo "gpu=1 kernel_ms=12.3 lancements=4 min_sites=${ms} routage_q3=2/1 ancres (seeds 500/100) routage_q4=2/1 ancres (seeds 400/80)"
echo "ouvriers wspd=${threads} rects=${threads} rle=${threads} prefiltre=${threads} census=${threads} expansion=${threads} fold=${threads}"
for k in 1 2 3 4 5 6 7 8 9 10; do
  echo "cardinalites K=${k} evenements=${base} facettes=${base} deltas=1 attachements=0 fusions=1 noeuds=1"
done
echo "temps_mur_ms=456.7 (etages A et B du fold pipelines)"
if [ "${digest}" = "1" ]; then
  echo "digest_balls=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  for k in 1 2 3 4 5 6 7 8 9 10; do
    echo "digest_forest_K${k}=cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
  done
  echo "digest_all=dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
fi
exit 0
EOF
chmod +x "${FAKE}/nvcc" "${FAKE}/nvidia-smi" "${FAKE}/cmake" \
  "${FAKE}/mhgp5_device_witness" "${FAKE}/mhgp5_cuda"

# Matrices reduites : DEUX configs de bench (les deux parites ABBA). Les
# surcharges de scenario ("$@") viennent EN DERNIER : env garde la derniere.
# Le PROFIL DE CAMPAGNE est ecrit une fois (matrice epinglee independamment
# du runner) et transmis au validateur — audit GCP v6, P1.
# Fichier CANONIQUE (liaison exigee par le validateur) + profil de campagne.
CANON="${WORK}/selftest_reduit_v1.env"
{
  echo "# profil canonique du selftest (grammaire fermee : identite + neuf axes)"
  echo 'PROFIL_NOM="selftest_reduit_v1"'
  echo 'CONF_SPECS="uniform:50000"'
  echo 'BENCH_SPECS="uniform:32000 eight_clusters:32000"'
  echo 'QUEUE_FAMILIES="terrain_stationnaire"'
  echo 'QUEUE_N="64000"'
  echo 'QUEUE_SEEDS="3 4"'
  echo 'RUN_TIMEOUT=60'
  echo 'THREADS_VM=8'
  echo 'V5_GATE_MIN=40'
  echo 'V6_GATE_MIN=60'
  echo 'SWEEP_SPECS="uniform:32000:2,8"'
  echo 'SWEEP_REPEATS=2'
  echo 'GPU_SPECS="uniform:50000"'
  echo 'FRONTIER_SPECS="uniform:200000 uniform:400000 uniform:800000"'
  echo 'FRONTIER_TIMEOUT=60'
  echo 'GPU_BUILD_TIMEOUT=60'
  echo 'FRONTIER_ULIMIT_KB=60000000'
} > "${CANON}"
CANON_SHA="$(sha256sum "${CANON}" | awk '{print $1}')"
MANIFESTE="${WORK}/manifest_revalide.txt"
{
  echo "schema=e-hgp.protocol-manifest.v1"
  echo "commit=${PIN_COMMIT}"
  printf '%s\t%s\t%s\n' "${CANON_SHA}" "$(wc -c < "${CANON}")" \
    "gcp-migration/profils/selftest_reduit_v1.env"
  printf '%s\t%s\t%s\n' \
    "$(sha256sum "${HERE}/profils/decision_v1.env" | awk '{print $1}')" \
    "$(wc -c < "${HERE}/profils/decision_v1.env")" \
    "gcp-migration/profils/decision_v1.env"
} > "${MANIFESTE}"
PIN_MANIFEST="$(sha256sum "${MANIFESTE}" | awk '{print $1}')"
PROFILE="${WORK}/profil_campagne.txt"
{
  echo "profil=selftest_reduit_v1"
  echo "profil_canonique=selftest_reduit_v1"
  echo "profil_canonique_sha256=${CANON_SHA}"
  echo "conf_specs=uniform:50000"
  echo "bench_specs=uniform:32000 eight_clusters:32000"
  echo "queue_families=terrain_stationnaire"
  echo "queue_n=64000"
  echo "queue_seeds=3 4"
  echo "run_timeout=60"
  echo "threads=8"
  echo "v5_gate_min=40"
  echo "v6_gate_min=60"
  echo "sweep_specs=uniform:32000:2,8"
  echo "sweep_repeats=2"
  echo "gpu_specs=uniform:50000"
  echo "frontier_specs=uniform:200000 uniform:400000 uniform:800000"
  echo "frontier_timeout=60"
  echo "gpu_build_timeout=60"
  echo "frontier_ulimit_kb=60000000"
} > "${PROFILE}"
run_runner() { # $1 = dossier out ; le reste = env supplementaire
  local out="$1"; shift
  env \
    V5_BIN="${FAKE}/mhgp5" V6_BIN="${FAKE}/mhgp6" CONF_BIN="${FAKE}/mhgp6_conformity" \
    TIME_BIN="${FAKE}/gnutime" OUT_DIR="${out}" THREADS=8 RUN_TIMEOUT=60 \
    CONF_SPECS="uniform:50000" \
    BENCH_SPECS="uniform:32000 eight_clusters:32000" \
    QUEUE_FAMILIES="terrain_stationnaire" QUEUE_N="64000" QUEUE_SEEDS="3 4" \
    SWEEP_SPECS="uniform:32000:2,8" SWEEP_REPEATS=2 \
    GPU_SPECS="uniform:50000" FRONTIER_SPECS="uniform:200000 uniform:400000 uniform:800000" FRONTIER_TIMEOUT=60 \
    GPU_BUILD_TIMEOUT=60 FRONTIER_ULIMIT_KB=60000000 \
    FAKE_V6_FRONT_FAIL=1 \
    NVCC_BIN="${FAKE}/nvcc" GPU_CMAKE_BIN="${FAKE}/cmake" \
    GPU_WITNESS_BIN="${FAKE}/mhgp5_device_witness" \
    GPU_Q3_GATE="${FAKE}/mhgp5_q3_lane_device_gate" GPU_Q4_GATE="${FAKE}/mhgp5_q4_lane_device_gate" \
    GPU_BIN="${FAKE}/mhgp5_cuda" V5CPU_BIN="${FAKE}/mhgp5" \
    PATH="${FAKE}:${PATH}" \
    "$@" \
    bash "${RUNNER}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST}" >/dev/null 2>&1
}
run_validator() { # $1 = dossier out, $2 = remote_rc, $3 = scp_rc, [$4 = profil], [$5 = canonique], [$6 = manifeste]
  python3 "${VALIDATOR}" "$1" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST}" "$2" "$3" \
    "${4:-${PROFILE}}" "${5:-${CANON}}" "${6:-${MANIFESTE}}"
}

# ---- 1. Nominal.
OUT="${WORK}/out_nominal"
rc=0; run_runner "${OUT}" || rc=$?
check "nominal : runner rc=0" "${rc}"
rc=0; VOUT="$(run_validator "${OUT}" 0 0)" || rc=$?
check "nominal : validateur rc=0" "${rc}"
check_true "nominal : verifie_non_decisionnel (profil reduit, jamais une decision)" \
  bash -c "printf '%s\n' \"\$1\" | grep -q 'campaign_status=verifie_non_decisionnel'" _ "${VOUT}"
check_true "nominal : plan ABBA contrebalance (parites 1 et 2)" bash -c "
  grep -q 'seq=1 name=bench_uniform_n32000_v5_r1 .* pos=1' '${OUT}/bench_plan.txt' &&
  grep -q 'seq=4 name=bench_uniform_n32000_v5_r2 .* pos=4' '${OUT}/bench_plan.txt' &&
  grep -q 'seq=5 name=bench_eight_clusters_n32000_v6_r1 .* pos=1' '${OUT}/bench_plan.txt' &&
  grep -q 'seq=8 name=bench_eight_clusters_n32000_v6_r2 .* pos=4' '${OUT}/bench_plan.txt'"
check_true "nominal : resumes ecrits HORS de out/ (idempotence)" \
  bash -c "test -s '${WORK}/bench_resume.txt' && test -s '${WORK}/queue_resume.txt' \
    && test -s '${WORK}/sweep_resume.txt' && test -s '${WORK}/gpu_resume.txt' \
    && test -s '${WORK}/frontier_resume.txt' && [ ! -e '${OUT}/bench_resume.txt' ]"
check_true "nominal : plan FILS contrebalance (avant r1, arriere r2)" bash -c "
  grep -q 'seq=1 name=sweep_uniform_n32000_t2_r1 .* pos=1' '${OUT}/sweep_plan.txt' &&
  grep -q 'seq=2 name=sweep_uniform_n32000_t8_r1 .* pos=2' '${OUT}/sweep_plan.txt' &&
  grep -q 'seq=3 name=sweep_uniform_n32000_t8_r2 .* pos=1' '${OUT}/sweep_plan.txt' &&
  grep -q 'seq=4 name=sweep_uniform_n32000_t2_r2 .* pos=2' '${OUT}/sweep_plan.txt'"
check_true "nominal : phase GPU complete (temoin, mutant tue, 4 contrats)" bash -c "
  grep -q '^code=0' '${OUT}/gpu_witness.status' && grep -q '^code=4' '${OUT}/gpu_mutant.status' &&
  grep -q '^code=0' '${OUT}/gpu_idx_uniform_n50000.status' &&
  grep -q 'device_witness OK' '${OUT}/gpu_witness.txt'"
check_true "nominal : trois classes de frontiere attestees (0 / 134 abort prouve signal 6 / 2 REFUS type)" bash -c "
  grep -q '^code=0' '${OUT}/front_uniform_n200000.status' &&
  grep -q '^code=134' '${OUT}/front_uniform_n400000.status' &&
  grep -q 'terminated by signal 6' '${OUT}/front_uniform_n400000.status.time' &&
  grep -q '^limit_kind=rlimit_as' '${OUT}/front_uniform_n400000.status' &&
  grep -q '^limit_kb=60000000' '${OUT}/front_uniform_n400000.status' &&
  grep -q 'bad_alloc sous rlimit_as' '${WORK}/frontier_resume.txt' &&
  grep -q '^code=2' '${OUT}/front_uniform_n800000.status' &&
  grep -q 'resource_exhausted (refus du pipeline)' '${WORK}/frontier_resume.txt'"
# IDEMPOTENCE (audit quatrieme tour) : un second passage sur le MEME dossier
# rend le meme verdict (le validateur n'enrichit plus l'inventaire qu'il juge).
rc=0; run_validator "${OUT}" 0 0 >/dev/null || rc=$?
check_true "validateur idempotent : second passage rc=0" [ "${rc}" -eq 0 ]

# ---- 1bis. PATH EMPOISONNE (septieme tour) : de faux `timeout` et `bash`
# en tete de PATH ne doivent JAMAIS etre appeles — les porteurs de bornes
# sont epingles en constantes absolues.
POISON="${WORK}/poison"
mkdir -p "${POISON}"
printf '#!/bin/sh\ntouch "%s/POISON_APPELE_timeout"\nshift 2\nshift\nexec "$@"\n' "${POISON}" > "${POISON}/timeout"
chmod +x "${POISON}/timeout"
OUTP="${WORK}/out_poison"
rc=0
env V5_BIN="${FAKE}/mhgp5" V6_BIN="${FAKE}/mhgp6" CONF_BIN="${FAKE}/mhgp6_conformity" \
  TIME_BIN="${FAKE}/gnutime" OUT_DIR="${OUTP}" THREADS=8 RUN_TIMEOUT=60 \
  CONF_SPECS="uniform:50000" BENCH_SPECS="aucun" QUEUE_FAMILIES="aucun" QUEUE_N="64000" QUEUE_SEEDS="3" \
  SWEEP_SPECS="aucun" SWEEP_REPEATS=2 GPU_SPECS="aucun" \
  FRONTIER_SPECS="uniform:200000" FRONTIER_TIMEOUT=60 GPU_BUILD_TIMEOUT=60 FRONTIER_ULIMIT_KB=60000000 \
  PATH="${POISON}:${FAKE}:${PATH}" \
  /bin/bash "${RUNNER}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST}" >/dev/null 2>&1 || rc=$?
check_true "PATH empoisonne : le faux timeout n'est JAMAIS appele et le wrapper grave est /bin/bash (bornes epinglees)" \
  bash -c "[ '${rc}' -eq 0 ] && [ ! -e '${POISON}/POISON_APPELE_timeout' ] \
    && grep -q '^commande=/bin/bash -c ulimit' '${OUTP}/front_uniform_n200000.status'"

# ---- 2. GNU time absent.
rc=0
env V5_BIN="${FAKE}/mhgp5" V6_BIN="${FAKE}/mhgp6" CONF_BIN="${FAKE}/mhgp6_conformity" \
  TIME_BIN="/nonexistent-gnu-time" OUT_DIR="${WORK}/out_notime" THREADS=8 \
  bash "${RUNNER}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST}" >/dev/null 2>&1 || rc=$?
check_true "GNU time absent : refus rc=2" [ "${rc}" -eq 2 ]

# ---- 3. Parametre mal forme.
rc=0; run_runner "${WORK}/out_badparam" QUEUE_N="64000 abc" || rc=$?
check_true "parametre mal forme : refus rc=2" [ "${rc}" -eq 2 ]

# ---- 4. Conformite en echec : rc=3, aucun bench, validateur rc=1.
OUT4="${WORK}/out_conf_fail"
rc=0; run_runner "${OUT4}" FAKE_CONF_FAIL=1 || rc=$?
check_true "conformite en echec : runner rc=3" [ "${rc}" -eq 3 ]
check_true "conformite en echec : aucun run de bench" \
  bash -c "[ -z \"\$(ls '${OUT4}'/bench_*_r*.status 2>/dev/null)\" ]"
rc=0; run_validator "${OUT4}" 3 0 >/dev/null || rc=$?
check_true "conformite en echec : validateur rc=1" [ "${rc}" -eq 1 ]

# ---- 5. Pilote de bench en echec : troncature gravee, queue executee.
OUT5="${WORK}/out_bench_fail"
rc=0; run_runner "${OUT5}" FAKE_V5_BENCH_FAIL=1 || rc=$?
check_true "bench en echec : troncature gravee, queue executee" bash -c "
  [ '${rc}' -eq 0 ] && [ -f '${OUT5}/bench_tronquee.txt' ] &&
  [ -f '${OUT5}/queue_terrain_stationnaire_n64000_s3.status' ]"
rc=0; run_validator "${OUT5}" 0 0 >/dev/null || rc=$?
check_true "bench en echec : validateur rc=1" [ "${rc}" -eq 1 ]

# ---- 6. Echeance passee : troncature avant le premier run.
OUT6="${WORK}/out_deadline"
rc=0; run_runner "${OUT6}" DEADLINE_EPOCH=1000000 || rc=$?
check_true "echeance passee : troncature et rc=3" \
  bash -c "[ '${rc}' -eq 3 ] && [ -f '${OUT6}/conf_tronquee.txt' ]"

# ---- 7. Falsifications du dossier rapatrie — DEUX DISCIPLINES DISTINCTES
# (sixieme tour : un mutant qui meurt sur le hash de transport ne prouve pas
# la porte semantique annoncee).
#   falsify_transport : corruption SANS rehash — la cause attendue est le
#     controle de transport lui-meme ;
#   falsify_semantique : mutation PUIS recalcul de MANIFESTE_DISTANT.txt
#     (meme recette que le runner) — le validateur doit refuser pour la
#     CAUSE SEMANTIQUE exacte, le transport etant redevenu coherent.
rehash_manifeste() { # $1 = dossier
  ( cd "$1" && find . -maxdepth 1 -type f ! -name 'MANIFESTE_DISTANT.txt*' -printf '%P\n' | sort \
    | xargs -d '\n' sha256sum > MANIFESTE_DISTANT.txt.tmp && mv MANIFESTE_DISTANT.txt.tmp MANIFESTE_DISTANT.txt )
}
falsify_case() { # $1 = nom, $2 = motif exact exige, $3 = rehash(0|1|apres), reste = mutation
  local name="$1" motif="$2" mode="$3"; shift 3
  local dir="${WORK}/out_falsif"
  rm -rf "${dir}"
  cp -r "${OUT}" "${dir}"
  if [ "${mode}" = "apres" ]; then rehash_manifeste "${dir}"; fi
  ( cd "${dir}" && "$@" )
  if [ "${mode}" = "1" ]; then rehash_manifeste "${dir}"; fi
  local rc=0 sortie
  sortie="$(run_validator "${dir}" 0 0 2>&1)" || rc=$?
  check_true "${name}" bash -c "[ \"\$3\" -eq 1 ] && printf '%s' \"\$1\" | grep -q -- \"\$2\"" \
    _ "${sortie}" "${motif}" "${rc}"
}
falsify_transport() { local n="$1" m="$2"; shift 2; falsify_case "${n}" "${m}" 0 "$@"; }
falsify_semantique() { local n="$1" m="$2"; shift 2; falsify_case "${n}" "${m}" 1 "$@"; }

# Transport : la corruption meurt sur le controle de transport lui-meme.
falsify_transport "transport : fichier en trop sans rehash" "liste != artefacts rapatries" \
  bash -c "echo intrus > intrus.txt"
falsify_transport "transport : statut altere sans rehash (hash de rapatriement)" "hash different du manifeste distant" \
  sed -i "s/^code=0/code=0 /" bench_uniform_n32000_v5_r1.status

# Semantique : rehash PUIS diagnostic exact — la porte annoncee tue seule.
falsify_semantique "semantique : statut de bench supprime" ".status ABSENT" \
  rm bench_uniform_n32000_v6_r1.status
falsify_semantique "semantique : pin altere dans un statut" "source_commit absent ou different du pin" \
  sed -i "s/^source_commit=.*/source_commit=badbadbadbad/" bench_uniform_n32000_v5_r1.status
falsify_semantique "semantique : digest imprime sur un run de bench" "digest imprime sur un run de bench" \
  sed -i "1i digest_all=eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee" bench_uniform_n32000_v6_r1.txt
falsify_semantique "semantique : compteurs non deterministes entre repetitions" "compteurs NON deterministes" \
  sed -i "s/boules_uniques=8000/boules_uniques=8001/" bench_uniform_n32000_v6_r2.txt
falsify_semantique "semantique : fichier inattendu (avec rehash)" "fichier inattendu" \
  bash -c "echo intrus > intrus.txt"
falsify_semantique "semantique : plan de bench altere" "sequence annoncee != sequence recalculee" \
  sed -i "s/^seq=1 name=bench_uniform_n32000_v5_r1/seq=1 name=bench_uniform_n32000_v6_r9/" bench_plan.txt
falsify_semantique "semantique : compteur de queue absent" "compteur W_sweep1_evals_coeur absent" \
  sed -i "/^sweep tests_coeur=/d" queue_terrain_stationnaire_n64000_s3.txt
falsify_semantique "semantique : invariance du grand-livre entre fils violee" "INVARIANCE DU GRAND-LIVRE VIOLEE" \
  sed -i "s/rect_visites_fusionnes=8000/rect_visites_fusionnes=8001/" sweep_uniform_n32000_t8_r1.txt
falsify_semantique "semantique : ligne invariante de FILS supprimee (vacuite)" "ligne invariante" \
  sed -i "/^generation /d" sweep_uniform_n32000_t2_r1.txt
falsify_semantique "semantique : digest GPU different du contrat CPU" "digest_all DIFFERENT du contrat CPU" \
  sed -i "s/^digest_all=d/digest_all=e/" gpu_dev_uniform_n50000.txt
falsify_semantique "semantique : mutant du temoin device non tue" "code=0 (attendu 4)" \
  sed -i "s/^code=4/code=0/" gpu_mutant.status
falsify_semantique "semantique : argv de route GPU altere (idx sans --gpu-wire=index)" "argument de route absent" \
  sed -i "s/ --gpu-wire=index//" gpu_idx_uniform_n50000.status
falsify_semantique "semantique : statut de frontiere supprime" ".status ABSENT" \
  rm front_uniform_n400000.status
falsify_semantique "semantique : plan GPU supprime" "gpu_plan.txt: ABSENT" \
  rm gpu_plan.txt
# Les SIX mutants de frontiere du sixieme tour — tous avec rehash causal.
falsify_semantique "frontiere : code=abc a corps bad_alloc" "code non decimal" \
  sed -i "s/^code=134/code=abc/" front_uniform_n400000.status
falsify_semantique "frontiere : code=3 a corps bad_alloc (hors classes)" "HORS des trois classes fermees" \
  sed -i "s/^code=134/code=3/" front_uniform_n400000.status
falsify_semantique "frontiere : wrapper supprime puis jetons decoratifs ajoutes" "correspondance EXACTE" \
  bash -c "sed -i 's|/bin/bash -c ulimit -v \"\$1\" && shift && exec \"\$@\" _ 60000000 ||' front_uniform_n400000.status; sed -i 's/^commande=\(.*\)$/commande=\1 ulimit 60000000/' front_uniform_n400000.status"
falsify_semantique "frontiere : INVARIANT ajoute au refus bad_alloc (code 134)" "motif FATAL" \
  bash -c "echo 'INVARIANT casse' >> front_uniform_n400000.txt"
falsify_semantique "frontiere : INVARIANT ajoute a un pipeline complet (code 0)" "motif FATAL" \
  bash -c "echo 'INVARIANT casse' >> front_uniform_n200000.txt"
falsify_semantique "frontiere : duree_s=abc" "duree_s non decimale" \
  sed -i "s/^duree_s=[0-9]*/duree_s=abc/" front_uniform_n400000.status
falsify_semantique "frontiere : argument decore xxx--n=400000xxx" "correspondance EXACTE" \
  sed -i "s/--n=400000/xxx--n=400000xxx/" front_uniform_n400000.status
falsify_semantique "frontiere : code 124 avec INVARIANT" "motif FATAL" \
  bash -c "sed -i 's/^code=134/code=124/' front_uniform_n400000.status; echo 'INVARIANT casse' >> front_uniform_n400000.txt"
falsify_semantique "frontiere : sortie 124 NON ATTRIBUEE (meme avec Exit status: 124)" "NON ATTRIBUEE" \
  bash -c "sed -i 's/^code=134/code=124/' front_uniform_n400000.status; sed -i 's/terminated by signal 6/Exit status: 124/' front_uniform_n400000.status.time"
falsify_semantique "frontiere : exit(134) simple sans preuve de signal 6" "sans preuve de signal 6" \
  sed -i "s/Command terminated by signal 6/Exit status: 134/" front_uniform_n400000.status.time
falsify_semantique "frontiere : REFUS suffixe resource_exhausted_faux" "sans exactement une ligne" \
  sed -i "s/REFUS resource_exhausted :/REFUS resource_exhausted_faux :/" front_uniform_n800000.txt
falsify_semantique "frontiere : classe 2 avec bad_alloc (exclusivite)" "classes non exclusives" \
  bash -c "echo 'std::bad_alloc' >> front_uniform_n800000.txt"
falsify_semantique "frontiere : classe 134 avec REFUS (exclusivite)" "classes non exclusives" \
  bash -c "echo 'REFUS parasite' >> front_uniform_n400000.txt"
falsify_semantique "frontiere : seconde ligne limit_kind ignoree hier, refusee" "dupliques" \
  bash -c "echo 'limit_kind=none' >> front_uniform_n400000.status; echo 'limit_kb=1' >> front_uniform_n400000.status"
falsify_semantique "frontiere : -v retire du wrapper" "correspondance EXACTE" \
  bash -c "sed -i 's/ulimit -v /ulimit /' front_uniform_n400000.status"
falsify_semantique "frontiere : arguments en conflit (--n duplique)" "correspondance EXACTE" \
  bash -c "sed -i 's/--n=400000/--n=400000 --n=1/' front_uniform_n400000.status"
falsify_case "semantique : MANIFESTE_DISTANT a ligne dupliquee" "chemin duplique" apres \
  bash -c "l=\$(grep topologie.txt MANIFESTE_DISTANT.txt | head -1); printf '%s\n' \"\$l\" >> MANIFESTE_DISTANT.txt"
falsify_semantique "frontiere : code 134 sans diagnostic std::bad_alloc" "sans diagnostic exact std::bad_alloc" \
  bash -c "sed -i 's/bad_alloc/xxx/' front_uniform_n400000.txt"
# CAS SUR SNAPSHOT PROPRE (audit quatrieme tour : plus jamais l'inventaire
# deja juge) avec DIAGNOSTIC EXACT exige — chaque cas repart d'une copie
# fraiche du nominal et doit echouer pour SA cause.
fresh_case() { # $1 = nom, $2 = remote_rc, $3 = scp_rc, $4 = profil (vide = defaut), $5 = motif exige, [$6 = canon], [$7 = manifeste]
  local dir="${WORK}/cas_propre"
  rm -rf "${dir}"
  cp -r "$(dirname "${OUT}")/$(basename "${OUT}")" "${dir}"
  local rc=0 sortie
  sortie="$(run_validator "${dir}" "$2" "$3" "${4:-${PROFILE}}" "${6:-${CANON}}" "${7:-${MANIFESTE}}" 2>&1)" || rc=$?
  check_true "$1" bash -c "[ \"\$3\" -eq 1 ] && printf '%s' \"\$1\" | grep -q -- \"\$2\"" _ "${sortie}" "$5" "${rc}"
}
fresh_case "falsification refusee : remote_rc non nul (cause exacte)" 3 0 "" "remote_campaign_rc=3"
fresh_case "falsification refusee : scp_rc non nul (cause exacte)" 0 1 "" "scp_rc=1"
fresh_case "falsification refusee : profil absent (cause exacte)" 0 0 "/nonexistent-profile" "profil de campagne ABSENT"
PROF2="${WORK}/profil_reduit.txt"
sed 's/^bench_specs=.*/bench_specs=uniform:64000 eight_clusters:64000/' "${PROFILE}" > "${PROF2}"
fresh_case "falsification refusee : plans != profil epingle (cause exacte)" 0 0 "${PROF2}" "!= profil epingle"
PROF3="${WORK}/profil_sha_faux.txt"
sed 's/^profil_canonique_sha256=.*/profil_canonique_sha256='"$(printf 'e%.0s' {1..64})"'/' "${PROFILE}" > "${PROF3}"
fresh_case "falsification refusee : sha du canon discordant (cause exacte)" 0 0 "${PROF3}" "profil_canonique_sha256 !="
CANON_INCONNU="${WORK}/canon_ligne_inconnue.env"
{ cat "${CANON}"; echo 'AXE_INVENTE="x"'; } > "${CANON_INCONNU}"
PROF_CI="${WORK}/profil_canon_inconnu.txt"
sed "s/^profil_canonique_sha256=.*/profil_canonique_sha256=$(sha256sum "${CANON_INCONNU}" | awk '{print $1}')/" "${PROFILE}" > "${PROF_CI}"
fresh_case "falsification refusee : ligne inconnue dans le canon (grammaire totale)" 0 0 "${PROF_CI}" "axe inconnu AXE_INVENTE" "${CANON_INCONNU}"
CANON_GUILLEMET="${WORK}/canon_guillemet.env"
sed 's/^CONF_SPECS="uniform:50000"/CONF_SPECS="uniform:50000/' "${CANON}" > "${CANON_GUILLEMET}"
PROF_CG="${WORK}/profil_canon_guillemet.txt"
sed "s/^profil_canonique_sha256=.*/profil_canonique_sha256=$(sha256sum "${CANON_GUILLEMET}" | awk '{print $1}')/" "${PROFILE}" > "${PROF_CG}"
fresh_case "falsification refusee : guillemet desapparie dans le canon" 0 0 "${PROF_CG}" "ligne hors grammaire" "${CANON_GUILLEMET}"
PROF5="${WORK}/profil_sans_sweep.txt"
sed '/^sweep_specs=/d' "${PROFILE}" > "${PROF5}"
fresh_case "falsification refusee : cle sweep_specs absente (cause exacte)" 0 0 "${PROF5}" "cle sweep_specs absente"
# MUTANT decision_v1 REDUIT (quatrieme tour) : noms decision_v1 + VRAI sha du
# canon + plans reduits concordants — doit rester verifie_non_decisionnel
# pour la cause « axes != canon », jamais decision_complete.
PROF4="${WORK}/profil_pretendu_decision.txt"
sed -e 's/^profil=selftest_reduit_v1/profil=decision_v1/' \
    -e 's/^profil_canonique=selftest_reduit_v1/profil_canonique=decision_v1/' "${PROFILE}" > "${PROF4}"
D4="${WORK}/cas_decision_reduit"
rm -rf "${D4}"; cp -r "${OUT}" "${D4}"
rc=0; VOUT4="$(run_validator "${D4}" 0 0 "${PROF4}")" || rc=$?
check_true "mutant decision_v1 reduit : TUE par l'identite profil_canonique != PROFIL_NOM (jamais decision_complete)" \
  bash -c "[ '${rc}' -eq 1 ] && printf '%s\n' \"\$1\" | grep -q 'profil_canonique=decision_v1 != PROFIL_NOM' \
    && ! printf '%s\n' \"\$1\" | grep -q 'decision_complete'" _ "${VOUT4}"
# MUTANT COORDONNE COMPLET (cinquieme tour) : canon REDUIT auto-declare
# decision_v1, hash et profil effectif recalcules et concordants — seule la
# LIAISON AU MANIFESTE le tue (le manifeste epingle ne contient pas ce canon).
CANON5="${WORK}/canon_pretendu_decision.env"
sed 's/^PROFIL_NOM="selftest_reduit_v1"/PROFIL_NOM="decision_v1"/' "${CANON}" > "${CANON5}"
PROF5B="${WORK}/profil_coordonne.txt"
sed -e 's/^profil=selftest_reduit_v1/profil=decision_v1/' \
    -e 's/^profil_canonique=selftest_reduit_v1/profil_canonique=decision_v1/' \
    -e "s/^profil_canonique_sha256=.*/profil_canonique_sha256=$(sha256sum "${CANON5}" | awk '{print $1}')/" \
    "${PROFILE}" > "${PROF5B}"
D5="${WORK}/cas_coordonne"
rm -rf "${D5}"; cp -r "${OUT}" "${D5}"
rc=0; VOUT5="$(run_validator "${D5}" 0 0 "${PROF5B}" "${CANON5}")" || rc=$?
check_true "mutant coordonne (canon reduit auto-declare + hash concordant) : TUE par la liaison au manifeste" \
  bash -c "[ '${rc}' -eq 1 ] && printf '%s\n' \"\$1\" | grep -q 'NON LIE au manifeste revalide' \
    && ! printf '%s\n' \"\$1\" | grep -q 'decision_complete'" _ "${VOUT5}"

# ---- PROFIL CANONIQUE G4 EXACT DE BOUT EN BOUT (cinquieme tour : le profil
# etait AUTO-INVALIDANT — queue_sequence recalculait la sentinelle. Plus
# jamais un profil canonique livre sans etre exerce runner -> validateur).
G4ENV="${HERE}/profils/g4_mesure_v1.env"
G4_SHA="$(sha256sum "${G4ENV}" | awk '{print $1}')"
OUTG4="${WORK}/out_g4"
PROFILE_G4="${WORK}/profil_g4.txt"
MANIFESTE_G4="${WORK}/manifest_g4.txt"
G4_AXES="${WORK}/g4_axes.env"
( set -a; source "${G4ENV}" >/dev/null
  {
    echo "profil=g4_mesure_v1"
    echo "profil_canonique=g4_mesure_v1"
    echo "profil_canonique_sha256=${G4_SHA}"
    echo "conf_specs=${CONF_SPECS}"
    echo "bench_specs=${BENCH_SPECS}"
    echo "queue_families=${QUEUE_FAMILIES}"
    echo "queue_n=${QUEUE_N}"
    echo "queue_seeds=${QUEUE_SEEDS}"
    echo "run_timeout=${RUN_TIMEOUT}"
    echo "threads=${THREADS_VM}"
    echo "v5_gate_min=${V5_GATE_MIN}"
    echo "v6_gate_min=${V6_GATE_MIN}"
    echo "sweep_specs=${SWEEP_SPECS}"
    echo "sweep_repeats=${SWEEP_REPEATS}"
    echo "gpu_specs=${GPU_SPECS}"
    echo "frontier_specs=${FRONTIER_SPECS}"
    echo "frontier_timeout=${FRONTIER_TIMEOUT}"
    echo "gpu_build_timeout=${GPU_BUILD_TIMEOUT}"
    echo "frontier_ulimit_kb=${FRONTIER_ULIMIT_KB}"
  } > "${PROFILE_G4}"
  {
    printf 'CONF_SPECS=%q\nBENCH_SPECS=%q\nQUEUE_FAMILIES=%q\nQUEUE_N=%q\nQUEUE_SEEDS=%q\n' \
      "${CONF_SPECS}" "${BENCH_SPECS}" "${QUEUE_FAMILIES}" "${QUEUE_N}" "${QUEUE_SEEDS}"
    printf 'RUN_TIMEOUT=%q\nTHREADS=%q\nSWEEP_SPECS=%q\nSWEEP_REPEATS=%q\nGPU_SPECS=%q\n' \
      "${RUN_TIMEOUT}" "${THREADS_VM}" "${SWEEP_SPECS}" "${SWEEP_REPEATS}" "${GPU_SPECS}"
    printf 'FRONTIER_SPECS=%q\nFRONTIER_TIMEOUT=%q\nGPU_BUILD_TIMEOUT=%q\nFRONTIER_ULIMIT_KB=%q\n' \
      "${FRONTIER_SPECS}" "${FRONTIER_TIMEOUT}" "${GPU_BUILD_TIMEOUT}" "${FRONTIER_ULIMIT_KB}"
  } > "${G4_AXES}"
)
{
  echo "schema=e-hgp.protocol-manifest.v1"
  echo "commit=${PIN_COMMIT}"
  printf '%s\t%s\t%s\n' "${G4_SHA}" "$(wc -c < "${G4ENV}")" "gcp-migration/profils/g4_mesure_v1.env"
} > "${MANIFESTE_G4}"
PIN_MANIFEST_G4="$(sha256sum "${MANIFESTE_G4}" | awk '{print $1}')"
rc=0
(
  set -a
  # shellcheck disable=SC1090
  source "${G4_AXES}"
  V5_BIN="${FAKE}/mhgp5" V6_BIN="${FAKE}/mhgp6" CONF_BIN="${FAKE}/mhgp6_conformity"
  TIME_BIN="/usr/bin/time" OUT_DIR="${OUTG4}"
  NVCC_BIN="${FAKE}/nvcc" GPU_CMAKE_BIN="${FAKE}/cmake"
  GPU_WITNESS_BIN="${FAKE}/mhgp5_device_witness"
  GPU_Q3_GATE="${FAKE}/mhgp5_q3_lane_device_gate" GPU_Q4_GATE="${FAKE}/mhgp5_q4_lane_device_gate"
  GPU_BIN="${FAKE}/mhgp5_cuda" V5CPU_BIN="${FAKE}/mhgp5"
  PATH="${FAKE}:${PATH}"
  bash "${RUNNER}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_G4}"
) >/dev/null 2>&1 || rc=$?
check_true "profil G4 exact : runner rc=0 (81 runs, queue a zero run)" \
  bash -c "[ '${rc}' -eq 0 ] && grep -q '^runs=0$' '${OUTG4}/queue_plan.txt' \
    && [ \"\$(sed -n 's/^runs=//p' '${OUTG4}/sweep_plan.txt')\" = '34' ] \
    && [ \"\$(sed -n 's/^runs=//p' '${OUTG4}/gpu_plan.txt')\" = '19' ] \
    && [ \"\$(sed -n 's/^runs=//p' '${OUTG4}/frontier_plan.txt')\" = '4' ]"
rc=0; VG4="$(python3 "${VALIDATOR}" "${OUTG4}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_G4}" 0 0 \
  "${PROFILE_G4}" "${G4ENV}" "${MANIFESTE_G4}")" || rc=$?
check_true "profil G4 exact : validateur rc=0, verifie_non_decisionnel" \
  bash -c "[ '${rc}' -eq 0 ] && printf '%s\n' \"\$1\" | grep -q 'campaign_status=verifie_non_decisionnel profil=g4_mesure_v1'" _ "${VG4}"

# ---- MUTANT decision_v1 + FAUX TIME_BIN (huitieme tour) : le profil
# DECISIONNEL exact, canon reel epingle au manifeste, execute de bout en
# bout par les faux pilotes ET un faux GNU time — l'instrumentation gravee
# (time_bin != /usr/bin/time) doit DECLASSER le verdict : jamais
# decision_complete, cause exacte.
DECENV="${HERE}/profils/decision_v1.env"
DEC_SHA="$(sha256sum "${DECENV}" | awk '{print $1}')"
OUTDEC="${WORK}/out_dec"
PROFILE_DEC="${WORK}/profil_dec.txt"
DEC_AXES="${WORK}/dec_axes.env"
( set -a; source "${DECENV}" >/dev/null
  {
    echo "profil=decision_v1"
    echo "profil_canonique=decision_v1"
    echo "profil_canonique_sha256=${DEC_SHA}"
    echo "conf_specs=${CONF_SPECS}"
    echo "bench_specs=${BENCH_SPECS}"
    echo "queue_families=${QUEUE_FAMILIES}"
    echo "queue_n=${QUEUE_N}"
    echo "queue_seeds=${QUEUE_SEEDS}"
    echo "run_timeout=${RUN_TIMEOUT}"
    echo "threads=${THREADS_VM}"
    echo "v5_gate_min=${V5_GATE_MIN}"
    echo "v6_gate_min=${V6_GATE_MIN}"
    echo "sweep_specs=${SWEEP_SPECS}"
    echo "sweep_repeats=${SWEEP_REPEATS}"
    echo "gpu_specs=${GPU_SPECS}"
    echo "frontier_specs=${FRONTIER_SPECS}"
    echo "frontier_timeout=${FRONTIER_TIMEOUT}"
    echo "gpu_build_timeout=${GPU_BUILD_TIMEOUT}"
    echo "frontier_ulimit_kb=${FRONTIER_ULIMIT_KB}"
  } > "${PROFILE_DEC}"
  {
    printf 'CONF_SPECS=%q\nBENCH_SPECS=%q\nQUEUE_FAMILIES=%q\nQUEUE_N=%q\nQUEUE_SEEDS=%q\n' \
      "${CONF_SPECS}" "${BENCH_SPECS}" "${QUEUE_FAMILIES}" "${QUEUE_N}" "${QUEUE_SEEDS}"
    printf 'RUN_TIMEOUT=%q\nTHREADS=%q\nSWEEP_SPECS=%q\nSWEEP_REPEATS=%q\nGPU_SPECS=%q\n' \
      "${RUN_TIMEOUT}" "${THREADS_VM}" "${SWEEP_SPECS}" "${SWEEP_REPEATS}" "${GPU_SPECS}"
    printf 'FRONTIER_SPECS=%q\nFRONTIER_TIMEOUT=%q\nGPU_BUILD_TIMEOUT=%q\nFRONTIER_ULIMIT_KB=%q\n' \
      "${FRONTIER_SPECS}" "${FRONTIER_TIMEOUT}" "${GPU_BUILD_TIMEOUT}" "${FRONTIER_ULIMIT_KB}"
  } > "${DEC_AXES}"
)
MANIFESTE_DEC="${WORK}/manifest_dec.txt"
{
  echo "schema=e-hgp.protocol-manifest.v1"
  echo "commit=${PIN_COMMIT}"
  printf '%s\t%s\t%s\n' "${DEC_SHA}" "$(wc -c < "${DECENV}")" "gcp-migration/profils/decision_v1.env"
} > "${MANIFESTE_DEC}"
PIN_MANIFEST_DEC="$(sha256sum "${MANIFESTE_DEC}" | awk '{print $1}')"
rc=0
(
  set -a
  # shellcheck disable=SC1090
  source "${DEC_AXES}"
  V5_BIN="${FAKE}/mhgp5" V6_BIN="${FAKE}/mhgp6" CONF_BIN="${FAKE}/mhgp6_conformity"
  TIME_BIN="${FAKE}/gnutime" OUT_DIR="${OUTDEC}"
  PATH="${FAKE}:${PATH}"
  /bin/bash "${RUNNER}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_DEC}"
) >/dev/null 2>&1 || rc=$?
check_true "mutant decision_v1 + faux TIME_BIN : runner rc=0 (82 runs)" [ "${rc}" -eq 0 ]
rc=0; VDEC="$(python3 "${VALIDATOR}" "${OUTDEC}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_DEC}" 0 0 \
  "${PROFILE_DEC}" "${DECENV}" "${MANIFESTE_DEC}")" || rc=$?
check_true "mutant decision_v1 + faux TIME_BIN : DECLASSE (instrumentation de test), jamais decision_complete" \
  bash -c "[ '${rc}' -eq 0 ] && printf '%s\n' \"\$1\" | grep -q 'cause=instrumentation de test' \
    && ! printf '%s\n' \"\$1\" | grep -q 'decision_complete'" _ "${VDEC}"

# ---- MUTANT g4 + FAUX TIME_BIN (dixieme tour) : le profil de MESURE ne
# publie JAMAIS les RSS d'un faux instrument — invalide, pas declasse.
OUTG4F="${WORK}/out_g4_faux"
rc=0
(
  set -a
  # shellcheck disable=SC1090
  source "${G4_AXES}"
  V5_BIN="${FAKE}/mhgp5" V6_BIN="${FAKE}/mhgp6" CONF_BIN="${FAKE}/mhgp6_conformity"
  TIME_BIN="${FAKE}/gnutime" OUT_DIR="${OUTG4F}"
  NVCC_BIN="${FAKE}/nvcc" GPU_CMAKE_BIN="${FAKE}/cmake"
  GPU_WITNESS_BIN="${FAKE}/mhgp5_device_witness"
  GPU_Q3_GATE="${FAKE}/mhgp5_q3_lane_device_gate" GPU_Q4_GATE="${FAKE}/mhgp5_q4_lane_device_gate"
  GPU_BIN="${FAKE}/mhgp5_cuda" V5CPU_BIN="${FAKE}/mhgp5"
  PATH="${FAKE}:${PATH}"
  /bin/bash "${RUNNER}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_G4}"
) >/dev/null 2>&1 || rc=$?
check_true "mutant g4 + faux TIME_BIN : runner rc=0" [ "${rc}" -eq 0 ]
rc=0; VG4F="$(python3 "${VALIDATOR}" "${OUTG4F}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_G4}" 0 0 \
  "${PROFILE_G4}" "${G4ENV}" "${MANIFESTE_G4}" 2>&1)" || rc=$?
check_true "mutant g4 + faux TIME_BIN : validateur rc=1, mesures invalides et non recevables" \
  bash -c "[ \"\$2\" -eq 1 ] && printf '%s\n' \"\$1\" | grep -q 'instrumentation NON STANDARD'" _ "${VG4F}" "${rc}"

# ---- TEMOIN POSITIF APPARIE (neuvieme tour) : decision_v1 + /usr/bin/time
# REEL => decision_complete reste atteignable (le mutant negatif prouve le
# declassement, pas l'atteignabilite). Conditionne a la presence de GNU
# time — son absence est bruyante, jamais un vert par vacuite.
if [ -x /usr/bin/time ]; then
  OUTDECP="${WORK}/out_dec_positif"
  rc=0
  (
    set -a
    # shellcheck disable=SC1090
    source "${DEC_AXES}"
    V5_BIN="${FAKE}/mhgp5" V6_BIN="${FAKE}/mhgp6" CONF_BIN="${FAKE}/mhgp6_conformity"
    TIME_BIN="/usr/bin/time" OUT_DIR="${OUTDECP}"
    PATH="${FAKE}:${PATH}"
    /bin/bash "${RUNNER}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_DEC}"
  ) >/dev/null 2>&1 || rc=$?
  check_true "temoin positif : decision_v1 + /usr/bin/time reel, runner rc=0" [ "${rc}" -eq 0 ]
  rc=0; VDECP="$(python3 "${VALIDATOR}" "${OUTDECP}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_DEC}" 0 0 \
    "${PROFILE_DEC}" "${DECENV}" "${MANIFESTE_DEC}")" || rc=$?
  check_true "temoin positif : decision_complete ATTEIGNABLE sous instrumentation canonique" \
    bash -c "[ '${rc}' -eq 0 ] && printf '%s\n' \"\$1\" | grep -q 'campaign_status=decision_complete'" _ "${VDECP}"
  # time_bin VIDE sur UN statut du temoin positif : la totalite le tue.
  DVIDE="${WORK}/out_dec_vide"
  rm -rf "${DVIDE}"; cp -r "${OUTDECP}" "${DVIDE}"
  onest=$(ls "${DVIDE}"/*.status | head -1)
  sed -i 's|^time_bin=.*|time_bin=|' "${onest}"
  rehash_manifeste "${DVIDE}"
  rc=0; VVIDE="$(python3 "${VALIDATOR}" "${DVIDE}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_DEC}" 0 0 \
    "${PROFILE_DEC}" "${DECENV}" "${MANIFESTE_DEC}" 2>&1)" || rc=$?
  check_true "mutant time_bin VIDE : refuse (jamais decision_complete par 81/82)" \
    bash -c "[ '${rc}' -eq 1 ] && printf '%s\n' \"\$1\" | grep -q 'time_bin VIDE' \
      && ! printf '%s\n' \"\$1\" | grep -q 'decision_complete'" _ "${VVIDE}"
else
  echo "ECHEC selftest : temoin positif IMPOSSIBLE — GNU time absent (/usr/bin/time requis)"
  FAILURES=$((FAILURES + 1))
fi

if [ "${FAILURES}" -ne 0 ]; then
  echo "selftest campagne v6 : ${FAILURES} echec(s)" >&2
  exit 1
fi
echo "selftest campagne v6 : runner distant + validateur conformes (nominal idempotent + profil G4 exact de bout en bout + refus + falsifications a cause exacte + mutants coordonnes tues par identite et liaison au manifeste) — le cycle de vie du lanceur et les gardes GCP sont prouves par selftest_cycle_vie_v6.sh, a lancer aussi"
