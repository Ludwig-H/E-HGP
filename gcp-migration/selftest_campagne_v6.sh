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
#      validateur rc=1 sur chacune ;
#   8. SERIE C (§ 5.12-5.15) : matrice (16 points x aller/retour/rotation8,
#      taskset atteste et recalcule), attribution, build/inventaire/portes/
#      pilote gpuv6 de bout en bout, puis falsifications a cause exacte
#      (plan manquant, troncature, parite par signatures, repetition
#      manquante, ABBA, octets, chronos, porte retiree, affinite, somme,
#      liaison litterale du canon, identite device/en-tete, UUID, hashes de
#      binaires, juge embarque) ;
#   8bis. fail-fast du runner : inventaire -N intrus = AUCUNE porte ; juge
#      en refus = famille suivante jamais consommee ; matrice tronquee =
#      bloc GPU v6 saute ;
#   9. budget du profil canonique serie C rejoue avec l'estimateur REEL
#      extrait du cycle de vie (fenetre 7 h gravee).
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
# Reponses par requete : identite complete (build serie C), instantane
# temperature/horloges (pilote), sinon la ligne historique (phase v5).
case "$*" in
  *name,uuid,compute_cap,driver_version*)
    echo "NVIDIA RTX PRO 6000 Blackwell Server Edition, GPU-11111111-2222-3333-4444-555555555555, 12.0, 580.65.06" ;;
  *uuid,temperature.gpu,clocks.sm,clocks.mem*)
    echo "GPU-11111111-2222-3333-4444-555555555555, 45, 2100, 8001" ;;
  *)
    echo "NVIDIA RTX PRO 6000 Blackwell Server Edition, 580.65.06" ;;
esac
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

# ---- Faux outillage SERIE C (§ 5.12) : ctest a inventaire exact, binaire
# de profil (attribution) et pilote mhgp6_cuda a records conformes au juge.
cat > "${FAKE}/ctest" <<'EOF'
#!/usr/bin/env bash
# -N = listage sans execution (inventaire pre-execution du runner) ;
# FAKE_CTEST_EXTRA=1 ajoute un 17e test intrus au listage.
case " $* " in
  *" -N "*)
    i=0
    for nm in ${GPUV6_GATE_NAMES:-}; do
      i=$((i + 1))
      echo "  Test #${i}: ${nm}"
    done
    if [ "${FAKE_CTEST_EXTRA:-0}" = "1" ]; then
      i=$((i + 1))
      echo "  Test #${i}: mhgp6_intrus"
    fi
    echo ""
    echo "Total Tests: ${i}"
    exit 0 ;;
esac
i=0
for nm in ${GPUV6_GATE_NAMES:-}; do
  i=$((i + 1))
  echo "      Start ${i}: ${nm}"
  echo "Test #${i}: ${nm} ...................................   Passed    0.10 sec"
done
echo ""
echo "100% tests passed, 0 tests failed out of ${i}"
EOF
cat > "${FAKE}/mhgp6_profile" <<'EOF'
#!/usr/bin/env bash
join=0
for a in "$@"; do
  case "$a" in --fold-join=*) join="${a#*=}" ;; esac
done
"$(dirname "$0")/mhgp6" "$@"
echo "profil_kind=reduce_v2 fold_join=${join} pic_workers_b=2 pic_reduce_actif=1"
for k in 1 2 3 4 5 6 7 8 9 10; do
  echo "profil_reduce K=${k} init=0.001 touch=0.001 pre=0.001 unite=0.001 post_remplissage=0.001 materialisation_tri_copie=0.001 liveness=0.001 partition=0.001 liberation=0.001 somme=0.009 mur_reduce_interne=0.010 residuel=0.001 reduce_interne_debut=0.000 reduce_interne_fin=0.010 a_debut=0.000 a_fin=0.000 duree_digest_foret_k_ms=0.000"
  echo "profil_intern K=${k} alloc_empreintes=0.001 offsets_diffusion=0.001 intern_tri=0.001 fusion_et_lib_parts=0.001 remap_et_lib_pools=0.001"
done
EOF
cat > "${FAKE}/mhgp6_cuda" <<'EOF'
#!/usr/bin/env bash
fam=""; n=0; repeat=4; ordre="cpu-device"; minlots=0; fils=8; graine=3
for a in "$@"; do
  case "$a" in
    --family=*) fam="${a#*=}" ;;
    --n=*) n="${a#*=}" ;;
    --repeat=*) repeat="${a#*=}" ;;
    --ordre=*) ordre="${a#*=}" ;;
    --min-lots=*) minlots="${a#*=}" ;;
    --threads=*) fils="${a#*=}" ;;
    --seed=*) graine="${a#*=}" ;;
  esac
done
inverse="device-cpu"; [ "${ordre}" = "device-cpu" ] && inverse="cpu-device"
# Identite coherente avec le faux nvidia-smi (§ 5.15.2) ; lot=500 pour que
# lot_effectif=500 = min(lot, nb_total=1000) et lots=2.
echo "pilote_serie_c device=NVIDIA RTX PRO 6000 Blackwell Server Edition sm=12.0 arch_compilees=120 famille=${fam} n=${n} graine=${graine} fils=${fils} lot=500"
r=0
while [ "${r}" -le "${repeat}" ]; do
  if [ "${r}" -eq 0 ]; then ret="NON"; o="${ordre}"; else
    ret="OUI"; m=$(((r - 1) % 4))
    if [ "${m}" -eq 0 ] || [ "${m}" -eq 3 ]; then o="${ordre}"; else o="${inverse}"; fi
  fi
  sig_dev="abababababababababababababababababababababababababababababababab"
  if [ "${FAKE_PILOT_BAD:-0}" = "1" ] && [ "${r}" -eq 2 ]; then
    sig_dev="cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd"
  fi
  echo "repetition=${r} retenue=${ret} ordre=${o} parite=OUI mur_cpu_ms=100.0 mur_route_device_ms=50.0 prefiltre_census_cpu_ms=40.0 route_device_etage_ms=45.0 wire_ms=5.0 setup_alloc_ms=1.0 h2d_ms=10.0 kernels_ms=20.0 d2h_ms=5.0 rebuild_ms=4.0 nb_total=1000 lot_effectif=500 h2d_octets_index=1234 h2d_octets_boules=112000 h2d_octets_sentinelles=100000 d2h_octets=100000 lots=2 digest_all=dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd signature_cpu=abababababababababababababababababababababababababababababababab signature_device=${sig_dev}"
  r=$((r + 1))
done
exit 0
EOF
chmod +x "${FAKE}/ctest" "${FAKE}/mhgp6_profile" "${FAKE}/mhgp6_cuda"

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
# Cles serie C (§ 5.12/5.13) : toujours emises par le cycle de vie — les
# fixtures de profil du selftest les portent aussi (defauts du cycle de vie).
emit_serie_c_defaults() {
  echo "matrice_points=aucun"
  echo "matrice_sequence=aller retour aller"
  echo "matrice_timeout=2400"
  echo "attrib_points=aucun"
  echo "attrib_timeout=2400"
  echo "gpuv6_gate_names=aucun"
  echo "gpuv6_build_timeout=1800"
  echo "gpuv6_gate_timeout=3600"
  echo "gpuv6_pilot_specs=aucun"
  echo "gpuv6_pilot_min_lots=2"
  echo "gpuv6_pilot_timeout=3600"
  echo "session_max_run_seconds=28800"
  echo "session_invite_minutes=470"
  echo "max_run_seconds_effectif=28800"
  echo "guest_shutdown_minutes_effectif=470"
}
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
  emit_serie_c_defaults
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

# ---- 8. SERIE C (§ 5.12/5.13) : matrice CPU + attribution + build/portes/
# pilote gpuv6 de bout en bout (faux outillage), puis les falsifications
# exigees par le GO conditionnel — plan manquant, parite falsifiee (les
# SIGNATURES sont l'autorite, jamais le booleen), repetition manquante,
# ordre hors ABBA, octets falsifies, chrono non termine, troncature gravee,
# porte gpu absente de l'inventaire, affinite non attestee, somme
# d'attribution faussee.
GATE_NAMES_C="mhgp6_device_witness mhgp6_device_witness_mutant_carry mhgp6_device_witness_mutant_skip_write mhgp6_device_witness_mutant_skip_native mhgp6_census_device mhgp6_census_device_mutant_range_le mhgp6_census_device_mutant_stack mhgp6_census_device_mutant_swap mhgp6_census_device_mutant_nonstrict mhgp6_census_device_mutant_skip_write mhgp6_census_device_mutant_nshell mhgp6_census_device_mutant_skip_count mhgp6_pilote_parite_400 mhgp6_pilote_refus_n mhgp6_pilote_lot17 mhgp6_pilote_mutant_base"
MAT_POINTS_C="uniform:16000:2:2:0:sans uniform:16000:1:1:0:sans uniform:16000:2:2:1:avec"
CANON_C="${WORK}/serie_c_selftest_v1.env"
{
  echo 'PROFIL_NOM="serie_c_selftest_v1"'
  echo 'CONF_SPECS="uniform:50000"'
  echo 'BENCH_SPECS="aucun"'
  echo 'QUEUE_FAMILIES="aucun"'
  echo 'QUEUE_N="64000"'
  echo 'QUEUE_SEEDS="3"'
  echo 'RUN_TIMEOUT=60'
  echo 'THREADS_VM=8'
  echo 'V5_GATE_MIN=40'
  echo 'V6_GATE_MIN=60'
  echo 'SWEEP_SPECS="aucun"'
  echo 'SWEEP_REPEATS=1'
  echo 'GPU_SPECS="aucun"'
  echo 'FRONTIER_SPECS="aucun"'
  echo 'FRONTIER_TIMEOUT=60'
  echo 'GPU_BUILD_TIMEOUT=60'
  echo 'FRONTIER_ULIMIT_KB=0'
  echo "MATRICE_POINTS=\"${MAT_POINTS_C}\""
  echo 'MATRICE_SEQUENCE="aller retour rotation8"'
  echo 'MATRICE_TIMEOUT=60'
  echo 'ATTRIB_POINTS="uniform:16000:2:2:0"'
  echo 'ATTRIB_TIMEOUT=60'
  echo "GPUV6_GATE_NAMES=\"${GATE_NAMES_C}\""
  echo 'GPUV6_BUILD_TIMEOUT=60'
  echo 'GPUV6_GATE_TIMEOUT=60'
  echo 'GPUV6_PILOT_SPECS="uniform:50000"'
  echo 'GPUV6_PILOT_MIN_LOTS=2'
  echo 'GPUV6_PILOT_TIMEOUT=60'
  echo 'SESSION_MAX_RUN_SECONDS=18000'
  echo 'SESSION_INVITE_MINUTES=270'
} > "${CANON_C}"
CANON_C_SHA="$(sha256sum "${CANON_C}" | awk '{print $1}')"
MANIFESTE_C="${WORK}/manifest_serie_c.txt"
{
  echo "schema=e-hgp.protocol-manifest.v1"
  echo "commit=${PIN_COMMIT}"
  printf '%s\t%s\t%s\n' "${CANON_C_SHA}" "$(wc -c < "${CANON_C}")" \
    "gcp-migration/profils/serie_c_selftest_v1.env"
} > "${MANIFESTE_C}"
PIN_MANIFEST_C="$(sha256sum "${MANIFESTE_C}" | awk '{print $1}')"
PROFILE_C="${WORK}/profil_serie_c.txt"
{
  echo "profil=serie_c_selftest_v1"
  echo "profil_canonique=serie_c_selftest_v1"
  echo "profil_canonique_sha256=${CANON_C_SHA}"
  echo "conf_specs=uniform:50000"
  echo "bench_specs=aucun"
  echo "queue_families=aucun"
  echo "queue_n=64000"
  echo "queue_seeds=3"
  echo "run_timeout=60"
  echo "threads=8"
  echo "v5_gate_min=40"
  echo "v6_gate_min=60"
  echo "sweep_specs=aucun"
  echo "sweep_repeats=1"
  echo "gpu_specs=aucun"
  echo "frontier_specs=aucun"
  echo "frontier_timeout=60"
  echo "gpu_build_timeout=60"
  echo "frontier_ulimit_kb=0"
  echo "matrice_points=${MAT_POINTS_C}"
  echo "matrice_sequence=aller retour rotation8"
  echo "matrice_timeout=60"
  echo "attrib_points=uniform:16000:2:2:0"
  echo "attrib_timeout=60"
  echo "gpuv6_gate_names=${GATE_NAMES_C}"
  echo "gpuv6_build_timeout=60"
  echo "gpuv6_gate_timeout=60"
  echo "gpuv6_pilot_specs=uniform:50000"
  echo "gpuv6_pilot_min_lots=2"
  echo "gpuv6_pilot_timeout=60"
  echo "session_max_run_seconds=18000"
  echo "session_invite_minutes=270"
  echo "max_run_seconds_effectif=18000"
  echo "guest_shutdown_minutes_effectif=270"
} > "${PROFILE_C}"
OUT8="${WORK}/out_serie_c"
run_runner_c() { # $1 = dossier out ; le reste = env supplementaire
  local out="$1"; shift
  env \
    V5_BIN="${FAKE}/mhgp5" V6_BIN="${FAKE}/mhgp6" CONF_BIN="${FAKE}/mhgp6_conformity" \
    V6_PROFILE_BIN="${FAKE}/mhgp6_profile" GPUV6_PILOT_BIN="${FAKE}/mhgp6_cuda" \
    TIME_BIN="${FAKE}/gnutime" OUT_DIR="${out}" THREADS=8 RUN_TIMEOUT=60 \
    CONF_SPECS="uniform:50000" BENCH_SPECS="aucun" QUEUE_FAMILIES="aucun" \
    QUEUE_N="64000" QUEUE_SEEDS="3" SWEEP_SPECS="aucun" SWEEP_REPEATS=1 \
    GPU_SPECS="aucun" FRONTIER_SPECS="aucun" FRONTIER_TIMEOUT=60 \
    GPU_BUILD_TIMEOUT=60 FRONTIER_ULIMIT_KB=0 \
    MATRICE_POINTS="${MAT_POINTS_C}" MATRICE_SEQUENCE="aller retour rotation8" MATRICE_TIMEOUT=60 \
    ATTRIB_POINTS="uniform:16000:2:2:0" ATTRIB_TIMEOUT=60 \
    GPUV6_GATE_NAMES="${GATE_NAMES_C}" GPUV6_BUILD_TIMEOUT=60 GPUV6_GATE_TIMEOUT=60 \
    GPUV6_PILOT_SPECS="uniform:50000" GPUV6_PILOT_MIN_LOTS=2 GPUV6_PILOT_TIMEOUT=60 \
    NVCC_BIN="${FAKE}/nvcc" GPU_CMAKE_BIN="${FAKE}/cmake" \
    PILOTE_JUGE="${HERE}/../morsehgp3D_v6/tests/pilote_juge.py" \
    PATH="${FAKE}:${PATH}" \
    "$@" \
    bash "${RUNNER}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_C}" >/dev/null 2>&1
}
rc=0; run_runner_c "${OUT8}" || rc=$?
check "serie C : runner rc=0" "${rc}"
check_true "serie C : plans matrice (9 = 3 points x 3 passages, rotation8 exacte), attrib (1), gpuv6 (3)" bash -c "
  [ \"\$(sed -n 's/^runs=//p' '${OUT8}/matrice_plan.txt')\" = '9' ] &&
  [ \"\$(sed -n 's/^runs=//p' '${OUT8}/attrib_plan.txt')\" = '1' ] &&
  [ \"\$(sed -n 's/^runs=//p' '${OUT8}/gpuv6_plan.txt')\" = '3' ] &&
  grep -q 'seq=7 name=mat_uniform_n16000_t2_i2_j1_avec_p3 .* passage=3 pos=1' '${OUT8}/matrice_plan.txt' &&
  grep -q 'seq=8 name=mat_uniform_n16000_t2_i2_j0_sans_p3 .* passage=3 pos=2' '${OUT8}/matrice_plan.txt'"
check_true "serie C : affinite demandee ET attestee sur un run de matrice" bash -c "
  grep -q '^affinite_demandee=' '${OUT8}/mat_uniform_n16000_t2_i2_j0_sans_p1.status' &&
  grep -q '^affinite_effective=' '${OUT8}/mat_uniform_n16000_t2_i2_j0_sans_p1.status' &&
  grep -q '^commande=taskset -c ' '${OUT8}/mat_uniform_n16000_t2_i2_j0_sans_p1.status'"
check_true "serie C : pilote a flags explicites et 5 records" bash -c "
  grep -q -- '--repeat=4' '${OUT8}/pilote_uniform_n50000.status' &&
  grep -q -- '--ordre=cpu-device' '${OUT8}/pilote_uniform_n50000.status' &&
  [ \"\$(grep -c '^repetition=' '${OUT8}/pilote_uniform_n50000.txt')\" = '5' ]"
run_validator_c() { # $1 = dossier out — le pin du manifeste est CELUI grave par le runner serie C
  python3 "${VALIDATOR}" "$1" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_C}" 0 0 \
    "${PROFILE_C}" "${CANON_C}" "${MANIFESTE_C}"
}
rc=0; VOUT8="$(run_validator_c "${OUT8}")" || rc=$?
check_true "serie C : validateur rc=0, verifie_non_decisionnel, resumes matrice + gpuv6 ecrits" \
  bash -c "[ '${rc}' -eq 0 ] && printf '%s\n' \"\$1\" | grep -q 'campaign_status=verifie_non_decisionnel' \
    && test -s '${WORK}/matrice_resume.txt' && test -s '${WORK}/gpuv6_resume.txt' \
    && [ \"\$(grep -c 'uniform' '${WORK}/gpuv6_resume.txt')\" = '4' ]" _ "${VOUT8}"
falsify_c() { # $1 = nom, $2 = motif exact exige, reste = mutation (rehash causal)
  local name="$1" motif="$2"; shift 2
  local dir="${WORK}/out_falsif_c"
  rm -rf "${dir}"
  cp -r "${OUT8}" "${dir}"
  ( cd "${dir}" && "$@" )
  rehash_manifeste "${dir}"
  local rc=0 sortie
  sortie="$(run_validator_c "${dir}" 2>&1)" || rc=$?
  check_true "${name}" bash -c "[ \"\$3\" -eq 1 ] && printf '%s' \"\$1\" | grep -q -- \"\$2\"" \
    _ "${sortie}" "${motif}" "${rc}"
}
falsify_c "serie C : plan matrice supprime" "matrice_plan.txt: ABSENT" \
  rm matrice_plan.txt
falsify_c "serie C : troncature matrice gravee" "campagne TRONQUEE" \
  bash -c "printf 'truncated_at=x\nphase=matrice\nreason=echeance simulee\n' > matrice_tronquee.txt"
falsify_c "serie C : parite falsifiee (signatures divergentes, le booleen parite=OUI ne sauve pas)" "signatures CPU/device divergentes" \
  sed -i 's/\(repetition=2 .*\)signature_device=ab/\1signature_device=cd/' pilote_uniform_n50000.txt
falsify_c "serie C : repetition manquante du pilote" "records au lieu de" \
  sed -i '/^repetition=4 /d' pilote_uniform_n50000.txt
falsify_c "serie C : ordre hors ABBA" "hors sequence ABBA" \
  sed -i 's/repetition=2 retenue=OUI ordre=device-cpu/repetition=2 retenue=OUI ordre=cpu-device/' pilote_uniform_n50000.txt
falsify_c "serie C : octets H2D falsifies" "112\*nb_total" \
  sed -i 's/\(repetition=1 .*\)h2d_octets_boules=112000/\1h2d_octets_boules=112001/' pilote_uniform_n50000.txt
falsify_c "serie C : chrono non termine (mur nul)" "mur nul" \
  sed -i 's/\(repetition=3 .*\)mur_cpu_ms=100.0/\1mur_cpu_ms=0.0/' pilote_uniform_n50000.txt
falsify_c "serie C : porte gpu retiree de l'inventaire" "absente ou non Passed" \
  sed -i '/Test #3:/d' gpuv6_gates.txt
falsify_c "serie C : affinite effective non conforme a la demande" "affinite effective" \
  sed -i 's/^affinite_effective=.*/affinite_effective=63/' mat_uniform_n16000_t2_i2_j0_sans_p1.status
falsify_c "serie C : somme d'attribution faussee (seuil 0.0051)" "somme imprimee != somme des neuf composantes" \
  sed -i 's/somme=0.009/somme=0.020/' attrib_uniform_n16000_t2_i2_j0.txt
falsify_c "serie C : plan matrice altere (rotation8 recalculee)" "sequence annoncee != sequence recalculee" \
  sed -i 's/^seq=7 name=mat_uniform_n16000_t2_i2_j1_avec_p3/seq=7 name=mat_uniform_n16000_t9_i2_j1_avec_p3/' matrice_plan.txt
falsify_c "serie C : bras sans-digest contamine par un digest" "digest imprime sur un bras sans-digest" \
  sed -i '1i digest_all=eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee' mat_uniform_n16000_t2_i2_j0_sans_p1.txt
# § 5.14.4 : inventaire pre-execution, verdict du juge embarque, identite
# des binaires et masque d'affinite recalcule — chacun causal.
falsify_c "serie C : inventaire pre-execution falsifie" "noms != plan" \
  sed -i 's/mhgp6_pilote_lot17/mhgp6_intrus_renomme/' gpuv6_inventaire.txt
falsify_c "serie C : verdict du juge embarque falsifie" "verdict du juge embarque non conforme" \
  sed -i 's/pilote juge conforme/pilote juge en doute/' pilote_uniform_n50000.juge.txt
falsify_c "serie C : binaire change en cours de phase (hash non unique)" "binaire_sha256 non unique" \
  sed -i "s/^binaire_sha256=.*/binaire_sha256=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/" mat_uniform_n16000_t2_i2_j0_sans_p1.status
falsify_c "serie C : affinite coherente mais hors du masque (socket,core) recalcule" "masque recalcule" \
  sed -i 's/^affinite_demandee=.*/affinite_demandee=1,3/; s/^affinite_effective=.*/affinite_effective=1,3/; s/taskset -c [0-9,-]*/taskset -c 1,3/' mat_uniform_n16000_t2_i2_j0_sans_p1.status

# § 5.15 : liaison litterale du canon (61 != 60 sous un nom canonique),
# identite device de l'en-tete du pilote liee au build, UUID stable.
PROF_C61="${WORK}/profil_serie_c_61.txt"
sed 's/^matrice_timeout=60$/matrice_timeout=61/' "${PROFILE_C}" > "${PROF_C61}"
D61="${WORK}/cas_serie_c_61"
rm -rf "${D61}"; cp -r "${OUT8}" "${D61}"
rc=0; V61="$(python3 "${VALIDATOR}" "${D61}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST_C}" 0 0 \
  "${PROF_C61}" "${CANON_C}" "${MANIFESTE_C}" 2>&1)" || rc=$?
check_true "serie C § 5.15 : matrice_timeout 60->61 sous un nom canonique REFUSE (liaison litterale)" \
  bash -c "[ '${rc}' -eq 1 ] && printf '%s' \"\$1\" | grep -q 'axe matrice_timeout != MATRICE_TIMEOUT'" _ "${V61}"
falsify_c "serie C § 5.15 : en-tete du pilote annoncant une autre famille" "en-tete : famille=" \
  sed -i 's/famille=uniform n=50000 graine=3/famille=terrain n=50000 graine=3/' pilote_uniform_n50000.txt
falsify_c "serie C § 5.15 : device de l'en-tete != nom du build" "device de l'en-tete" \
  sed -i 's/device=NVIDIA RTX PRO 6000 Blackwell Server Edition sm=/device=NVIDIA T4 sm=/' pilote_uniform_n50000.txt
falsify_c "serie C § 5.15 : sm de l'en-tete != compute capability du build" "sm de l'en-tete" \
  sed -i 's/ sm=12.0 arch_compilees=/ sm=8.6 arch_compilees=/' pilote_uniform_n50000.txt
falsify_c "serie C § 5.15 : UUID de l'instantane != UUID du build" "UUID .* != UUID du build" \
  sed -i 's/^GPU-11111111-2222-3333-4444-555555555555, 45/GPU-99999999-2222-3333-4444-555555555555, 45/' pilote_uniform_n50000.txt
falsify_c "serie C § 5.15 : commande d'attribution sans taskset" "confinement taskset" \
  sed -i 's/^commande=taskset -c [0-9,-]* /commande=/' attrib_uniform_n16000_t2_i2_j0.status

# ---- 8bis. FAIL-FAST DU RUNNER (§ 5.14.3/4) : l'inventaire intrus tronque
# AVANT toute porte ; un stdout de pilote falsifie a code nul tronque APRES
# la premiere famille — la seconde n'est jamais consommee.
OUT8B="${WORK}/out_serie_c_intrus"
rc=0; run_runner_c "${OUT8B}" FAKE_CTEST_EXTRA=1 || rc=$?
check_true "serie C fail-fast : 17e test au listage -N => troncature AVANT execution (aucune porte)" \
  bash -c "[ '${rc}' -eq 0 ] && grep -q 'inventaire pre-execution' '${OUT8B}/gpuv6_tronquee.txt' \
    && [ ! -e '${OUT8B}/gpuv6_gates.status' ] && [ ! -e '${OUT8B}/pilote_uniform_n50000.status' ]"
OUT8C="${WORK}/out_serie_c_juge"
rc=0; run_runner_c "${OUT8C}" GPUV6_PILOT_SPECS="uniform:50000 terrain:50000" FAKE_PILOT_BAD=1 || rc=$?
check_true "serie C fail-fast : records falsifies a code nul => juge tronque avant la famille suivante" \
  bash -c "[ '${rc}' -eq 0 ] && grep -q 'records du pilote refuses par le juge' '${OUT8C}/gpuv6_tronquee.txt' \
    && [ -e '${OUT8C}/pilote_uniform_n50000.status' ] && [ ! -e '${OUT8C}/pilote_terrain_n50000.status' ]"
OUT8D="${WORK}/out_serie_c_prereq"
rc=0; run_runner_c "${OUT8D}" V6_BIN=/bin/false || rc=$?
check_true "serie C § 5.15 fail-fast : matrice en echec => bloc GPU v6 SAUTE (aucun build, cause publiee)" \
  bash -c "[ '${rc}' -eq 0 ] && grep -q 'premier run non nul' '${OUT8D}/matrice_tronquee.txt' \
    && grep -q 'prerequis matrice/attribution tronques' '${OUT8D}/gpuv6_tronquee.txt' \
    && [ ! -e '${OUT8D}/gpuv6_build.status' ] && [ ! -e '${OUT8D}/pilote_uniform_n50000.status' ]"

# ---- 9. BUDGET DU PROFIL CANONIQUE SERIE C (§ 5.14.2) : le calcul est
# rejoue avec l'ESTIMATEUR REEL extrait du cycle de vie (jamais une
# reimplementation) sur les axes du profil g4_serie_c_v1 — un profil qui se
# refuse lui-meme au preflight (l'enveloppe 5 h se refusait de 257 s) ne
# peut plus etre livre. Fenetre gravee pour 25200 s / 405 min aux defauts
# du cycle de vie : SCP_BUDGET=935, POST=2915, CUTOFF=24300, MARGE=3905,
# WINDOW = 25200 - 3905 - 900 = 20395 s. Plancher 10000 s contre un profil
# silencieusement vide.
SERIE_C_ENV="${HERE}/profils/g4_serie_c_v1.env"
est_c="$(bash -c '
  set -euo pipefail
  '"$(sed -n '/^budget_estimate() {/,/^}$/p' "${HERE}/v6_session_lifecycle.sh")"'
  set -a
  # shellcheck disable=SC1090
  source "'"${SERIE_C_ENV}"'"
  budget_estimate
' 2>&1)" || est_c="ECHEC:${est_c}"
check_true "budget serie C : ESTIMATE=${est_c}s tient dans la fenetre 7h gravee (10000 <= est <= 20395)" \
  bash -c "[[ '${est_c}' =~ ^[0-9]+$ ]] && [ '${est_c}' -ge 10000 ] && [ '${est_c}' -le 20395 ]"

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
    emit_serie_c_defaults
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
    emit_serie_c_defaults
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
