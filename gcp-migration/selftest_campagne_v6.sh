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

# Matrices reduites : DEUX configs de bench (les deux parites ABBA). Les
# surcharges de scenario ("$@") viennent EN DERNIER : env garde la derniere.
run_runner() { # $1 = dossier out ; le reste = env supplementaire
  local out="$1"; shift
  env \
    V5_BIN="${FAKE}/mhgp5" V6_BIN="${FAKE}/mhgp6" CONF_BIN="${FAKE}/mhgp6_conformity" \
    TIME_BIN="${FAKE}/gnutime" OUT_DIR="${out}" THREADS=8 RUN_TIMEOUT=60 \
    CONF_FAMILIES="uniform" CONF_N=50000 \
    BENCH_FAMILIES="uniform eight_clusters" BENCH_N="32000" \
    QUEUE_FAMILIES="terrain_stationnaire" QUEUE_N="64000" QUEUE_SEEDS="3 4" \
    "$@" \
    bash "${RUNNER}" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST}" >/dev/null 2>&1
}
run_validator() { # $1 = dossier out, $2 = remote_rc, $3 = scp_rc
  python3 "${VALIDATOR}" "$1" "${PIN_COMMIT}" "${PIN_PAYLOAD}" "${PIN_MANIFEST}" "$2" "$3"
}

# ---- 1. Nominal.
OUT="${WORK}/out_nominal"
rc=0; run_runner "${OUT}" || rc=$?
check "nominal : runner rc=0" "${rc}"
rc=0; VOUT="$(run_validator "${OUT}" 0 0)" || rc=$?
check "nominal : validateur rc=0" "${rc}"
check_true "nominal : CAMPAGNE COMPLETE" \
  bash -c "printf '%s\n' \"\$1\" | grep -q '=== CAMPAGNE COMPLETE ==='" _ "${VOUT}"
check_true "nominal : plan ABBA contrebalance (parites 1 et 2)" bash -c "
  grep -q 'seq=1 name=bench_uniform_n32000_v5_r1 .* pos=1' '${OUT}/bench_plan.txt' &&
  grep -q 'seq=4 name=bench_uniform_n32000_v5_r2 .* pos=4' '${OUT}/bench_plan.txt' &&
  grep -q 'seq=5 name=bench_eight_clusters_n32000_v6_r1 .* pos=1' '${OUT}/bench_plan.txt' &&
  grep -q 'seq=8 name=bench_eight_clusters_n32000_v6_r2 .* pos=4' '${OUT}/bench_plan.txt'"
check_true "nominal : resumes ecrits" \
  bash -c "test -s '${OUT}/bench_resume.txt' && test -s '${OUT}/queue_resume.txt'"

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

# ---- 7. Falsifications du dossier rapatrie : chacune REFUSEE (rc=1).
falsify() { # $1 = nom, $2... = commande de mutation (executee dans la copie)
  local name="$1"; shift
  local dir="${WORK}/out_falsif"
  rm -rf "${dir}"
  cp -r "${OUT}" "${dir}"
  rm -f "${dir}/bench_resume.txt" "${dir}/queue_resume.txt"
  ( cd "${dir}" && "$@" )
  local rc=0
  run_validator "${dir}" 0 0 >/dev/null || rc=$?
  check_true "falsification refusee : ${name}" [ "${rc}" -eq 1 ]
}
falsify "statut de bench supprime" rm bench_uniform_n32000_v6_r1.status
falsify "pin altere dans un statut" \
  sed -i "s/^source_commit=.*/source_commit=badbadbadbad/" bench_uniform_n32000_v5_r1.status
falsify "digest imprime sur un run de bench" \
  sed -i "1i digest_all=eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee" bench_uniform_n32000_v6_r1.txt
falsify "compteurs non deterministes entre repetitions" \
  sed -i "s/boules_uniques=8000/boules_uniques=8001/" bench_uniform_n32000_v6_r2.txt
falsify "fichier en trop" bash -c "echo intrus > intrus.txt"
falsify "plan de bench altere" \
  sed -i "s/^seq=1 name=bench_uniform_n32000_v5_r1/seq=1 name=bench_uniform_n32000_v6_r9/" bench_plan.txt
falsify "compteur de queue absent" \
  sed -i "/^sweep tests_coeur=/d" queue_terrain_stationnaire_n64000_s3.txt
rc=0; run_validator "${OUT}" 3 0 >/dev/null || rc=$?
check_true "falsification refusee : remote_rc non nul" [ "${rc}" -eq 1 ]
rc=0; run_validator "${OUT}" 0 1 >/dev/null || rc=$?
check_true "falsification refusee : scp_rc non nul" [ "${rc}" -eq 1 ]

if [ "${FAILURES}" -ne 0 ]; then
  echo "selftest campagne v6 : ${FAILURES} echec(s)" >&2
  exit 1
fi
echo "selftest campagne v6 : protocole transactionnel conforme (nominal + refus 2/3/4/5/6 + 9 falsifications)"
