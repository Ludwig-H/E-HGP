#!/usr/bin/env bash
# SELFTEST TRANSACTIONNEL du protocole de campagne v5 — sans une minute de G4
# ni de gcloud. Il execute LE MEME script distant (v5_campaign_remote.sh) et
# LE MEME validateur (validate_v5_campaign.py) que la session, avec un faux
# pilote, un faux binaire de conformite et un faux GNU time, et exige :
#   1. HAPPY PATH : 16 statuts (12 conformites + 4 contrats), code=0 partout,
#      pin present partout, validateur -> complete (rc 0) ;
#   2. RUN 50 k EN ECHEC : le faux pilote rend 7 sur terrain et depasse le
#      timeout sur eight_clusters — code=7 et code=124 MATERIALISES, campagne
#      au bout, validateur -> partial en LISTANT ces runs ;
#   3. CONFORMITE NON ETABLIE : la fausse conformite diverge sur
#      scanline_single_pass n=16000 — le contrat 50 k est REFUSE (exit 3),
#      aucun fichier contrat_*, statuts de conformite conserves ;
#   4. codes de session non nuls -> jamais complete ;
#   5. PIN DU PROTOCOLE : runner ou validateur modifie non commite -> refus ;
#   6. statut sans manifeste de protocole -> jamais complete ;
#   7. SCALE_THREADS a jeu reduit (SCALE_THREADS="1 2", une famille, n=400,
#      repeats 1, inflight "1 2", digest "0 1" : 8 runs) : plan annonce,
#      topologie gravee, statuts complets (fils, inflight, digest, commande,
#      GNU time), validateur -> complete et tableau scale_threads_resume.txt
#      SANS conclusion de speedup ; repeats 2 -> ordre CONTREBALANCE (1 2 puis
#      2 1) verifie sur le plan ; puis les refus : digest different a 2 fils,
#      ligne generation differente, inflight ignore par le pilote, digest
#      imprime sans --digest, run annonce supprime, plan reordonne, topologie
#      absente, parametre SCALE_* mal forme (refus code 2 AVANT tout run).
# Codes : 0 conforme, 1 desaccord de scenario.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK="$(mktemp -d /tmp/ehgp-v5selftest.XXXXXXXX)"
trap 'rm -rf "${WORK}"' EXIT
BAD=0
fail() { echo "SELFTEST : $1" >&2; BAD=$((BAD + 1)); }

cat > "${WORK}/fake_time" <<'EOS'
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
  echo "	Command being timed: \"$*\""
  echo "	User time (seconds): 0.10"
  echo "	System time (seconds): 0.01"
  echo "	Percent of CPU this job got: 99%"
  echo "	Elapsed (wall clock) time (h:mm:ss or m:ss): 0:00.11"
  echo "	Maximum resident set size (kbytes): 300000"
  echo "	Voluntary context switches: 1"
  echo "	Exit status: ${rc}"
} > "${out}"
exit ${rc}
EOS
chmod +x "${WORK}/fake_time"

# Faux pilote 50 k : SCHEMA COMPLET de production (identite depuis l'argv,
# generation, ouvriers, temps, fold en vol, cardinalites K=1..10, digests
# seulement sous --digest) ; FAIL_FAMILY -> 7, HANG_FAMILY -> timeout.
# Mutants SCALE_THREADS : SCALE_DIGEST_DRIFT (digest different a 2 fils),
# SCALE_GEN_DRIFT (compteurs de generation differents selon les fils),
# SCALE_INFLIGHT_IGNORED (toujours 1 ordre en vol), SCALE_DIGEST_ALWAYS
# (digests imprimes meme sans --digest).
cat > "${WORK}/fake_probe" <<'EOS'
#!/usr/bin/env bash
fam=""; n=""; thr=1; infl=2; dig=0
for a in "$@"; do
  case "$a" in
    --family=*) fam="${a#--family=}" ;;
    --n=*) n="${a#--n=}" ;;
    --threads=*) thr="${a#--threads=}" ;;
    --fold-inflight=*) infl="${a#--fold-inflight=}" ;;
    --digest) dig=1 ;;
  esac
done
[ "${fam}" = "${FAIL_FAMILY:-}" ] && exit 7
[ "${fam}" = "${HANG_FAMILY:-}" ] && sleep 30
gen_w4=22860
[ "${SCALE_GEN_DRIFT:-0}" = "1" ] && gen_w4=$((22860 + thr))
[ "${SCALE_INFLIGHT_IGNORED:-0}" = "1" ] && infl=1
echo "payload=mhgp5-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none"
echo "backend=cpu_reference"
echo "tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11"
echo "famille=${fam} n=${n} coord=1 s=8 smax=11 seed=3 threads=${thr} emis=1 boules_uniques=42 mortes_profondeur=0 survivantes=42 census_int=1 census_shell=0 evenements=7 facettes=9 fusions=3 deltas=1 noeuds=1"
echo "generation rect_alive=7379/14563/15374 ancres=11990/47282/53317 candidats=10982/32163/23942 tues_profondeur=0/388726/103000 ancres_w4=${gen_w4} ancres_w3=19562 ancres_secteurs=3061/5557 ancres_cellules=0/459 seeds_cellules=0/2382 grilles=0/536 seeds_core_tues=157009 seeds_corde_tues=130572 float_cert=7389726/6171068 repli=174685 jung=2255407/1473815/0"
echo "ouvriers wspd=${thr}/${thr}/${thr} rects=${thr}/${thr}/${thr} rle=${thr} prefiltre=${thr} census=${thr} expansion=${thr} fold=${thr}"
echo "temps_ms index=0.1 gen=5.0 (wspd 1.0/1.0/1.0 rects 1.0/1.0/1.0) rle=0.1 prefiltre=0.1 census=0.1 comptage=0.1 expansion=0.1 fold=1.0 (tri 0.1 intern 0.1 fusion 0.1 reduce 0.1) digest=0.1"
echo "temps_mur_ms=$((100 / thr)).5 (etages A et B du fold pipelines : fold+digest ci-dessus sont des cumuls par etage, pas le mur)"
echo "temps_fold_mur_ms=10.0 (etages A et B, fold_inflight=${infl}, pic_mesure_en_vol=1)"
echo "rss_mb apres_generation=22 apres_rle=22 apres_prefiltre=24 apres_census=46 max_fold=62 fin=48"
for k in $(seq 1 10); do
  echo "cardinalites K=${k} evenements=$((k * 100)) facettes=$((k * 300)) deltas=$((k * 90)) attachements=$((k * 50)) fusions=$((k * 299)) noeuds=$((k * 40))"
done
if [ "${dig}" = "1" ] || [ "${SCALE_DIGEST_ALWAYS:-0}" = "1" ]; then
  echo "digest_balls=abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789"
  for k in $(seq 1 10); do
    echo "digest_forest_K${k}=$(printf '%064d' "${k}")"
  done
  if [ "${SCALE_DIGEST_DRIFT:-0}" = "1" ] && [ "${thr}" = "2" ]; then
    echo "digest_all=ffff56789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0"
  else
    echo "digest_all=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
  fi
fi
echo "rss_max_kb=62688"
EOS
chmod +x "${WORK}/fake_probe"

# Faux pilote CUDA : meme sortie que la sonde, plus la ligne gpu=1 ; GPU_DIGEST_MISM=1
# change le digest (doit etre refuse).
cat > "${WORK}/fake_gpu" <<'EOS'
#!/usr/bin/env bash
fam=""; n=""; ms=1
for a in "$@"; do case "$a" in --family=*) fam="${a#--family=}";; --n=*) n="${a#--n=}";; --gpu-min-sites=*) ms="${a#--gpu-min-sites=}";; esac; done
echo "backend=override_experimental (executeur de lane externe : non autoritaire)"
echo "tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11"
echo "famille=${fam} n=${n} coord=1 s=8 smax=11 seed=3 threads=1 emis=1 boules_uniques=42 mortes_profondeur=0 survivantes=42 census_int=1 census_shell=0 evenements=7 facettes=9 fusions=3 deltas=1 noeuds=1"
echo "digest_balls=abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789"
if [ "${GPU_DIGEST_MISM:-0}" = "1" ]; then echo "digest_all=ffff456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"; else echo "digest_all=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"; fi
# Le faux pilote HONORE le seuil : routage mixte a 256, tout-device a 1 (GPU_IGNORE_THRESHOLD=1 : tout-device quel que soit le seuil).
if [ "${ms}" = "1" ] || [ "${GPU_IGNORE_THRESHOLD:-0}" = "1" ]; then
  echo "gpu=1 kernel_ms=1.0 lancements=3 min_sites=${ms} routage_q3=10/0 ancres (seeds 100/0) routage_q4=10/0 ancres (seeds 100/0)"
else
  echo "gpu=1 kernel_ms=1.0 lancements=3 min_sites=${ms} routage_q3=7/3 ancres (seeds 70/30) routage_q4=3/7 ancres (seeds 30/70)"
fi
EOS
chmod +x "${WORK}/fake_gpu"

# Fausse conformite : egal partout sauf (DIVERGE_FAMILY, DIVERGE_N) -> code 1.
cat > "${WORK}/fake_conformity" <<'EOS'
#!/usr/bin/env bash
fam=""; n=""
for a in "$@"; do
  case "$a" in
    --family=*) fam="${a#--family=}" ;;
    --n=*) n="${a#--n=}" ;;
  esac
done
echo "famille=${fam} n=${n} boules_uniques=42 evenements=7 facettes=9"
if [ "${fam}" = "${DIVERGE_FAMILY:-}" ] && [ "${n}" = "${DIVERGE_N:-}" ]; then
  echo "conformite_v4 famille=${fam} n=${n} balls=egal all=DIFFERENT"; exit 1
fi
echo "conformite_v4 famille=${fam} n=${n} balls=egal all=egal"
EOS
chmod +x "${WORK}/fake_conformity"
echo "# faux reçu" > "${WORK}/recu.txt"

# Faux nvcc : le temoin device est simule par NVCC_BIN pointant sur un script
# dont le repertoire contient un `cmake` factice ; plus simplement, le run
# gpu_witness est produit par le script distant via `bash -c` : on remplace
# bash par... non — on force le chemin « nvcc present » avec un faux nvcc et
# un PATH ou cmake est un faux qui reussit et cree un faux temoin.
mkdir -p "${WORK}/fakebin"
cat > "${WORK}/fakebin/nvcc" <<'EOS'
#!/usr/bin/env bash
echo "nvcc: NVIDIA (R) Cuda compiler driver (faux)"
echo "Cuda compilation tools, release 12.9, V12.9.41"
EOS
cat > "${WORK}/fakebin/nvidia-smi" <<'EOS'
#!/usr/bin/env bash
echo "NVIDIA RTX PRO 6000 Blackwell Server Edition (faux), 580.173.02"
EOS
chmod +x "${WORK}/fakebin/nvidia-smi"
cat > "${WORK}/fakebin/cmake" <<'EOS'
#!/usr/bin/env bash
# faux cmake : cree le faux temoin device au premier appel (-S), rien au build.
for a in "$@"; do case "$a" in -B) shift; mkdir -p "$1/." ;; esac; done
mkdir -p build-cuda
cat > build-cuda/mhgp5_device_witness <<'EOT'
#!/usr/bin/env bash
[ "${WITNESS_FAIL:-0}" = "1" ] && { echo "DESACCORD device/hote"; exit 1; }
for a in "$@"; do case "$a" in --inject=witness-no-warp-correction) [ "${MUTANT_SURVIVES:-0}" = "1" ] && { echo "MUTANT NON TUE"; exit 1; }; echo "DESACCORD device/hote"; exit 4;; esac; done
echo "device=faux sm=12.0"; echo "arith cas=262144 desaccords=0"
echo "scan famille=uniform ancres=10 seeds=728347 sites=100 morts=1 desaccords=${WITNESS_SCAN_MISM:-0} kernel_ms=1.000"
echo "scan famille=eight_clusters ancres=10 seeds=2308366 sites=100 morts=1 desaccords=0 kernel_ms=1.000"
echo "device_witness OK"
EOT
chmod +x build-cuda/mhgp5_device_witness
cat > build-cuda/mhgp5_q3_lane_device_gate <<'EOT'
#!/usr/bin/env bash
fam=uniform; n=1200; t=1
for a in "$@"; do case "$a" in --family=*) fam="${a#--family=}";; --n=*) n="${a#--n=}";; --threads=*) t="${a#--threads=}";; esac; done
echo "q3_lane_device famille=${fam} n=${n} fils=${t} seuil=65536 vidages=10 max_lot_seeds=1 max_ancre_seeds=1 candidats_q3=1176250 seeds=3373964 tues=3197714 replis=768748 lancements=10 kernel_ms=1.0 desaccords_vecteur=${LANE_MISM:-0} desaccords_compteurs=0"
[ "${LANE_MISM:-0}" = "0" ] && { echo "q3_lane_device OK"; exit 0; }
exit 1
EOT
chmod +x build-cuda/mhgp5_q3_lane_device_gate
cat > build-cuda/mhgp5_q4_lane_device_gate <<'EOT'
#!/usr/bin/env bash
fam=uniform; n=1200; t=1
for a in "$@"; do case "$a" in --family=*) fam="${a#--family=}";; --n=*) n="${a#--n=}";; --threads=*) t="${a#--threads=}";; esac; done
echo "q4_lane_device famille=${fam} n=${n} fils=${t} seuil=65536 vidages=10 max_lot_seeds=1 max_ancre_seeds=1 candidats_q4=144020 candidats_lots=144020 seeds=508979 coeur_tues=357862 completions=3992025 profonds=363074 lancements=3 kernel_ms=1.0 desaccords_vecteur=0 desaccords_compteurs=0"
echo "q4_lane_device OK"
EOT
chmod +x build-cuda/mhgp5_q4_lane_device_gate
exit 0
EOS
chmod +x "${WORK}/fakebin/nvcc" "${WORK}/fakebin/cmake"

run_campaign() {
  local out="$1"; shift
  mkdir -p "${out}"
  local rc=0
  ( cd "${WORK}" && env "$@" PATH="${WORK}/fakebin:${PATH}" NVCC_BIN="${WORK}/fakebin/nvcc" \
      PROBE_BIN="${WORK}/fake_probe" CONFORMITY_BIN="${WORK}/fake_conformity" GPU_BIN="${WORK}/fake_gpu" \
      RECEIPT="${WORK}/recu.txt" TIME_BIN="${WORK}/fake_time" THREADS=1 \
      OUT_DIR="${out}" RUN_TIMEOUT=2 \
      bash "${HERE}/v5_campaign_remote.sh" cafedeca beefbeef feedf00d ) \
    > "${out}.log" 2>&1 || rc=$?
  echo "${rc}"
}

# ---- Scenario 1 : happy path -> complete.
RC1=$(run_campaign "${WORK}/out1")
[ "${RC1}" -eq 0 ] || fail "scenario 1 : script distant rc=${RC1}"
[ "$(ls "${WORK}/out1"/*.status 2>/dev/null | wc -l)" -eq 25 ] || fail "scenario 1 : 25 statuts attendus (temoin + lane device + mutant + 12 conformites + 4 contrats CPU + 4 contrats GPU + 2 adaptatifs)"
grep -L '^source_commit=cafedeca$' "${WORK}/out1"/*.status | grep -q . && fail "scenario 1 : pin source absent d'un statut"
if ! python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out1" cafedeca beefbeef feedf00d 0 0 > "${WORK}/v1.log" 2>&1; then
  fail "scenario 1 : validateur a refuse un happy path ($(cat "${WORK}/v1.log"))"
fi
grep -q 'campaign_status=complete' "${WORK}/v1.log" || fail "scenario 1 : complete attendu"

# ---- Scenario 2 : echec (7) + timeout (124) materialises, validateur partial.
RC2=$(run_campaign "${WORK}/out2" FAIL_FAMILY=terrain HANG_FAMILY=eight_clusters)
[ "${RC2}" -eq 0 ] || fail "scenario 2 : la campagne doit aller au bout (rc=${RC2})"
grep -q '^code=7$' "${WORK}/out2/contrat_terrain_n50000.status" 2>/dev/null || fail "scenario 2 : code=7 non materialise"
grep -q '^code=124$' "${WORK}/out2/contrat_eight_clusters_n50000.status" 2>/dev/null || fail "scenario 2 : code=124 non materialise"
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out2" cafedeca beefbeef feedf00d 0 0 > "${WORK}/v2.log" 2>&1; then
  fail "scenario 2 : le validateur devait refuser"
fi
grep -q 'contrat_terrain' "${WORK}/v2.log" && grep -q 'contrat_eight_clusters' "${WORK}/v2.log" || fail "scenario 2 : runs en echec non listes"

# ---- Scenario 3 : conformite non etablie -> contrat refuse (exit 3).
RC3=$(run_campaign "${WORK}/out3" DIVERGE_FAMILY=scanline_single_pass DIVERGE_N=16000)
[ "${RC3}" -eq 3 ] || fail "scenario 3 : exit 3 attendu (rc=${RC3})"
ls "${WORK}/out3"/contrat_*.txt >/dev/null 2>&1 && fail "scenario 3 : aucun contrat 50 k ne devait etre lance"
grep -q '^code=1$' "${WORK}/out3/conf_scanline_single_pass_n16000.status" 2>/dev/null || fail "scenario 3 : statut de la conformite divergente absent"
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out3" cafedeca beefbeef feedf00d 3 0 > "${WORK}/v3.log" 2>&1; then
  fail "scenario 3 : le validateur devait refuser"
fi

# ---- Scenario 3bis : temoin device en desaccord -> jamais complete (statut code=1 conserve).
RC3b=$(run_campaign "${WORK}/out3b" WITNESS_FAIL=1)
grep -q '^code=1$' "${WORK}/out3b/gpu_witness.status" 2>/dev/null || fail "scenario 3bis : code=1 du temoin non materialise"
[ "$(ls "${WORK}/out3b"/*.status 2>/dev/null | wc -l)" -eq 1 ] || fail "scenario 3bis : les phases 1 et 2 devaient etre REFUSEES apres un temoin en echec"
[ "${RC3b}" -eq 3 ] || fail "scenario 3bis : code distant 3 attendu (refus des phases), obtenu ${RC3b}"
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out3b" cafedeca beefbeef feedf00d "${RC3b}" 0 > "${WORK}/v3b.log" 2>&1; then
  fail "scenario 3bis : un temoin device en desaccord devait refuser complete"
fi
grep -q 'gpu_witness' "${WORK}/v3b.log" || fail "scenario 3bis : le temoin doit etre LISTE"

# ---- Scenario 3ter : temoin « OK » en sous-chaine mais desaccords=1 sur un scan -> refuse par le validateur.
RC3c=$(run_campaign "${WORK}/out3c" WITNESS_SCAN_MISM=1)
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out3c" cafedeca beefbeef feedf00d "${RC3c}" 0 > "${WORK}/v3c.log" 2>&1; then
  fail "scenario 3ter : desaccords=1 avec la sous-chaine OK devait etre refuse"
fi
grep -q 'scan uniform' "${WORK}/v3c.log" || fail "scenario 3ter : le desaccord du scan doit etre nomme"

# ---- Scenario 3quater : lane device en desaccord -> les phases CPU tournent (18 statuts) mais jamais complete.
RC3d=$(run_campaign "${WORK}/out3d" LANE_MISM=1)
# Les phases CPU tournent (19 statuts : temoin, lane, mutant, 12 conformites, 4 contrats CPU) ; les contrats GPU sont REFUSES (precondition de phase).
[ "$(ls "${WORK}/out3d"/*.status 2>/dev/null | wc -l)" -eq 19 ] || fail "scenario 3quater : la lane device ne refuse pas les phases CPU (ou n'a pas refuse les contrats GPU)"
[ "$(ls "${WORK}/out3d"/contrat_gpu*.status 2>/dev/null | wc -l)" -eq 0 ] || fail "scenario 3quater : contrats GPU executes malgre une lane device en desaccord"
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out3d" cafedeca beefbeef feedf00d "${RC3d}" 0 > "${WORK}/v3d.log" 2>&1; then
  fail "scenario 3quater : une lane device en desaccord devait refuser complete"
fi
grep -q 'gpu_lane' "${WORK}/v3d.log" || fail "scenario 3quater : la lane device doit etre nommee"

# ---- Scenario 3quinquies : digest GPU different du CPU -> jamais complete.
RC3e=$(run_campaign "${WORK}/out3e" GPU_DIGEST_MISM=1)
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out3e" cafedeca beefbeef feedf00d "${RC3e}" 0 > "${WORK}/v3e.log" 2>&1; then
  fail "scenario 3quinquies : un digest GPU different devait refuser complete"
fi
grep -q 'DIFFERENT du contrat CPU' "${WORK}/v3e.log" || fail "scenario 3quinquies : le desaccord de digest doit etre nomme"

# ---- Scenario 3sexies : mutant du temoin survivant sur le device -> jamais complete.
RC3f=$(run_campaign "${WORK}/out3f" MUTANT_SURVIVES=1)
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out3f" cafedeca beefbeef feedf00d "${RC3f}" 0 > "${WORK}/v3f.log" 2>&1; then
  fail "scenario 3sexies : un mutant survivant devait refuser complete"
fi
grep -q 'gpu_mutant' "${WORK}/v3f.log" || fail "scenario 3sexies : le mutant doit etre nomme"

# ---- Scenario 3septies : pilote GPU ignorant le seuil (adaptatif tout-device) -> refuse.
RC3g=$(run_campaign "${WORK}/out3g" GPU_IGNORE_THRESHOLD=1)
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out3g" cafedeca beefbeef feedf00d "${RC3g}" 0 > "${WORK}/v3g.log" 2>&1; then
  fail "scenario 3septies : un adaptatif sans route hote devait etre refuse"
fi
grep -q 'les deux routes' "${WORK}/v3g.log" || fail "scenario 3septies : la vacuite de route doit etre nommee"

# ---- Scenario 3octies : mutant du temoin survivant -> les contrats GPU ne doivent PAS tourner (precondition de phase).
[ "$(ls "${WORK}/out3f"/contrat_gpu*.status 2>/dev/null | wc -l)" -eq 0 ] || fail "scenario 3octies : contrats GPU executes malgre un mutant survivant"

# ---- Scenario 3novies : runs d'extension (EXTRA_N) presents et juges ; un run d'extension en echec refuse complete.
RC3h=$(run_campaign "${WORK}/out3h" EXTRA_N="100000")
[ "$(ls "${WORK}/out3h"/contrat_*_n100000.status 2>/dev/null | wc -l)" -eq 2 ] || fail "scenario 3novies : deux runs d'extension attendus"
python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out3h" cafedeca beefbeef feedf00d "${RC3h}" 0 > "${WORK}/v3h.log" 2>&1 || fail "scenario 3novies : extension nominale refusee ($(cat "${WORK}/v3h.log"))"
RC3i=$(run_campaign "${WORK}/out3i" EXTRA_N="100000" FAIL_FAMILY=eight_clusters)
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out3i" cafedeca beefbeef feedf00d "${RC3i}" 0 > "${WORK}/v3i.log" 2>&1; then
  fail "scenario 3novies : un run d'extension en echec devait refuser complete"
fi

# ---- Scenario 4 : code de session non nul.
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out1" cafedeca beefbeef feedf00d 255 0 > "${WORK}/v4.log" 2>&1; then
  fail "scenario 4 : remote_rc=255 devait refuser complete"
fi

# ---- Scenario 5 : pin du protocole.
PINREPO="${WORK}/pinrepo"
mkdir -p "${PINREPO}/gcp-migration" "${PINREPO}/morsehgp3D_v5"
cp "${HERE}/session_campagne_v5_scale_g4.sh" "${HERE}/v5_campaign_remote.sh" \
   "${HERE}/validate_v5_campaign.py" "${HERE}/v5_campaign_pin.sh" "${PINREPO}/gcp-migration/"
echo "stub" > "${PINREPO}/morsehgp3D_v5/README.md"
( cd "${PINREPO}" && git init -q && git add -A && git -c user.email=t@t -c user.name=t commit -qm pin )
pin_run() { ( cd "${PINREPO}" && mkdir -p w && ./gcp-migration/v5_campaign_pin.sh "$(pwd)/w" ); }
pin_run > "${WORK}/pin1.log" 2>&1 || fail "scenario 5 : pin propre refuse"
grep -q '^protocol_manifest_sha256=' "${WORK}/pin1.log" || fail "scenario 5 : manifeste absent"
echo "# graine modifiee" >> "${PINREPO}/gcp-migration/v5_campaign_remote.sh"
pin_run > "${WORK}/pin2.log" 2>&1 && fail "scenario 5 : runner modifie ACCEPTE"
grep -q 'REFUS' "${WORK}/pin2.log" || fail "scenario 5 : refus runner sans message"
( cd "${PINREPO}" && git checkout -q -- gcp-migration/v5_campaign_remote.sh )
printf '#!/usr/bin/env python3\nprint("campaign_status=complete")\n' > "${PINREPO}/gcp-migration/validate_v5_campaign.py"
pin_run > "${WORK}/pin3.log" 2>&1 && fail "scenario 5 : validateur toujours-zero ACCEPTE"

# ---- Scenario 6 : statut sans manifeste.
cp -r "${WORK}/out1" "${WORK}/out6"
sed -i '/^protocol_manifest_sha256=/d' "${WORK}/out6/conf_uniform_n8000.status"
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out6" cafedeca beefbeef feedf00d 0 0 > "${WORK}/v6.log" 2>&1; then
  fail "scenario 6 : manifeste absent devait refuser"
fi

# ---- Scenario 7 : SCALE_THREADS a jeu reduit (8 runs) -> complete + resume.
SCALE_ENV=(SCALE_THREADS="1 2" SCALE_FAMILIES=eight_clusters SCALE_N=400 SCALE_INFLIGHT="1 2" SCALE_DIGEST="0 1" SCALE_REPEATS=1)
RC7=$(run_campaign "${WORK}/out7" "${SCALE_ENV[@]}")
[ "${RC7}" -eq 0 ] || fail "scenario 7 : script distant rc=${RC7} ($(tail -3 "${WORK}/out7.log"))"
[ "$(ls "${WORK}/out7"/scale_*.status 2>/dev/null | wc -l)" -eq 8 ] || fail "scenario 7 : 8 statuts scale_* attendus (2 fils x 2 inflight x 2 digest x 1 repetition)"
[ -f "${WORK}/out7/topologie.txt" ] && grep -q '^nproc=' "${WORK}/out7/topologie.txt" || fail "scenario 7 : topologie.txt absente ou sans nproc"
grep -q '^runs=8$' "${WORK}/out7/scale_threads_plan.txt" || fail "scenario 7 : plan annonce sans runs=8"
ST7="${WORK}/out7/scale_eight_clusters_n400_t2_f2_d1_r1.status"
for want in '^threads=2$' '^fold_inflight=2$' '^digest=1$' '^timing_scope=scale_threads$' '^commande=.*--threads=2 --fold-inflight=2 --digest$' '^seq=8$'; do
  grep -qE "${want}" "${ST7}" || fail "scenario 7 : statut sans ${want}"
done
grep -q 'Elapsed (wall clock)' "${ST7}.time" || fail "scenario 7 : sortie complete de GNU time absente"
grep -qE '^commande=.*--threads=1 --fold-inflight=1$' "${WORK}/out7/scale_eight_clusters_n400_t1_f1_d0_r1.status" || fail "scenario 7 : la commande digest=0 ne doit pas porter --digest"
if ! python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out7" cafedeca beefbeef feedf00d 0 0 > "${WORK}/v7.log" 2>&1; then
  fail "scenario 7 : validateur a refuse le jeu reduit ($(cat "${WORK}/v7.log"))"
fi
grep -q 'scale_threads 8 runs annonces' "${WORK}/v7.log" || fail "scenario 7 : le validateur doit annoncer les 8 runs scale"
RES7="${WORK}/out7/scale_threads_resume.txt"
[ -f "${RES7}" ] || fail "scenario 7 : scale_threads_resume.txt absent"
[ "$(grep -vc '^#' "${RES7}")" -eq 9 ] || fail "scenario 7 : 1 en-tete + 8 lignes (famille, fils, inflight, digest) attendues dans le resume"
grep -qE '^eight_clusters	2	2	1	1	50\.5	50\.5	50\.5	' "${RES7}" || fail "scenario 7 : ligne de resume (eight_clusters, 2 fils, inflight 2, digest 1) absente ou mal formee"
grep -v '^#' "${RES7}" | grep -qiE 'speedup|accelerat|gain' && fail "scenario 7 : le resume ne doit ecrire aucune conclusion de speedup"
grep -q 'Aucune conclusion de speedup' "${RES7}" || fail "scenario 7 : l en-tete du resume doit dire qu aucune conclusion de speedup n est ecrite"

# ---- Scenario 7bis : repeats=2 -> ordre CONTREBALANCE (1 2 puis 2 1) dans le plan, 16 runs.
RC7b=$(run_campaign "${WORK}/out7b" "${SCALE_ENV[@]}" SCALE_REPEATS=2)
[ "${RC7b}" -eq 0 ] || fail "scenario 7bis : rc=${RC7b}"
ORDER7b="$(sed -n 's/^seq=[0-9]* name=scale_eight_clusters_n400_t\([0-9]*\)_f1_d0_r\([0-9]\).*/r\2t\1/p' "${WORK}/out7b/scale_threads_plan.txt" | tr '\n' ' ')"
[ "${ORDER7b}" = "r1t1 r1t2 r2t2 r2t1 " ] || fail "scenario 7bis : ordre contrebalance attendu r1t1 r1t2 r2t2 r2t1, vu '${ORDER7b}'"
[ "$(ls "${WORK}/out7b"/scale_*.status | wc -l)" -eq 16 ] || fail "scenario 7bis : 16 statuts attendus"
python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out7b" cafedeca beefbeef feedf00d 0 0 > "${WORK}/v7b.log" 2>&1 || fail "scenario 7bis : validateur a refuse ($(cat "${WORK}/v7b.log"))"
grep -qE '^eight_clusters	1	1	0	2	' "${WORK}/out7b/scale_threads_resume.txt" || fail "scenario 7bis : le resume doit compter 2 runs par combinaison"

# ---- Scenario 7ter : mutants du pilote -> refuses et NOMMES.
scale_mutant() {
  local label="$1" pattern="$2"; shift 2
  local out="${WORK}/out7_${label}" rc
  rc=$(run_campaign "${out}" "${SCALE_ENV[@]}" "$@")
  if python3 "${HERE}/validate_v5_campaign.py" "${out}" cafedeca beefbeef feedf00d "${rc}" 0 > "${out}.vlog" 2>&1; then
    fail "scenario 7ter (${label}) : devait etre refuse"
  fi
  grep -qE "${pattern}" "${out}.vlog" || fail "scenario 7ter (${label}) : le refus doit nommer '${pattern}' ($(cat "${out}.vlog"))"
}
scale_mutant digest_drift 'eight_clusters: digest_all DIFFERENT' SCALE_DIGEST_DRIFT=1
scale_mutant gen_drift 'eight_clusters: ligne generation DIFFERENT' SCALE_GEN_DRIFT=1
scale_mutant inflight_ignored 'fold_inflight imprime 1 != 2 demande' SCALE_INFLIGHT_IGNORED=1
scale_mutant digest_always 'digest imprime alors que digest=0' SCALE_DIGEST_ALWAYS=1
scale_mutant failed_run 'scale_eight_clusters_n400_t1_f1_d0_r1: code=7' FAIL_FAMILY=eight_clusters

# ---- Scenario 7quater : mutations du reçu -> refusees (run annonce supprime, plan reordonne, topologie absente).
cp -r "${WORK}/out7" "${WORK}/out7m1"; rm "${WORK}/out7m1/scale_eight_clusters_n400_t2_f1_d1_r1.status"
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out7m1" cafedeca beefbeef feedf00d 0 0 > "${WORK}/v7m1.log" 2>&1; then
  fail "scenario 7quater : run annonce absent ACCEPTE"
fi
grep -q 'scale_eight_clusters_n400_t2_f1_d1_r1: .status ABSENT' "${WORK}/v7m1.log" || fail "scenario 7quater : le run annonce absent doit etre nomme"
cp -r "${WORK}/out7" "${WORK}/out7m2"
python3 - "${WORK}/out7m2/scale_threads_plan.txt" <<'PY'
import sys
p = sys.argv[1]
lines = open(p).read().splitlines()
i = [k for k, l in enumerate(lines) if l.startswith("seq=1 ")][0]
lines[i], lines[i + 1] = lines[i + 1].replace("seq=2 ", "seq=1 "), lines[i].replace("seq=1 ", "seq=2 ")
open(p, "w").write("\n".join(lines) + "\n")
PY
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out7m2" cafedeca beefbeef feedf00d 0 0 > "${WORK}/v7m2.log" 2>&1; then
  fail "scenario 7quater : plan reordonne (non contrebalance) ACCEPTE"
fi
grep -q 'sequence annoncee != sequence contrebalancee' "${WORK}/v7m2.log" || fail "scenario 7quater : le plan reordonne doit etre nomme"
cp -r "${WORK}/out7" "${WORK}/out7m3"; rm "${WORK}/out7m3/topologie.txt"
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out7m3" cafedeca beefbeef feedf00d 0 0 > "${WORK}/v7m3.log" 2>&1; then
  fail "scenario 7quater : topologie absente ACCEPTEE"
fi
grep -q 'topologie.txt: ABSENT' "${WORK}/v7m3.log" || fail "scenario 7quater : la topologie absente doit etre nommee"
# Des runs scale_* sans plan annonce sont des fichiers inattendus, jamais un sous-ensemble juge.
cp -r "${WORK}/out7" "${WORK}/out7m4"; rm "${WORK}/out7m4/scale_threads_plan.txt"
if python3 "${HERE}/validate_v5_campaign.py" "${WORK}/out7m4" cafedeca beefbeef feedf00d 0 0 > "${WORK}/v7m4.log" 2>&1; then
  fail "scenario 7quater : runs scale sans plan ACCEPTES"
fi
grep -q 'fichier inattendu' "${WORK}/v7m4.log" || fail "scenario 7quater : les runs sans plan doivent etre inattendus"

# ---- Scenario 7quinquies : parametre SCALE_* mal forme -> refus code 2 AVANT tout run.
RC7e=$(run_campaign "${WORK}/out7e" SCALE_THREADS="1 deux" SCALE_FAMILIES=eight_clusters SCALE_N=400 SCALE_REPEATS=1)
[ "${RC7e}" -eq 2 ] || fail "scenario 7quinquies : refus code 2 attendu (rc=${RC7e})"
[ "$(ls "${WORK}/out7e"/*.status 2>/dev/null | wc -l)" -eq 0 ] || fail "scenario 7quinquies : aucun run ne devait etre lance"
RC7f=$(run_campaign "${WORK}/out7f" SCALE_THREADS="1 2" SCALE_INFLIGHT="0 1" SCALE_FAMILIES=eight_clusters SCALE_N=400 SCALE_REPEATS=1)
[ "${RC7f}" -eq 2 ] || fail "scenario 7quinquies : inflight 0 hors domaine devait etre refuse (rc=${RC7f})"

# ---- Scenario 7sexies : sans SCALE_THREADS, aucun fichier scale_*/topologie (phase strictement optionnelle).
ls "${WORK}/out1"/scale_* "${WORK}/out1/topologie.txt" >/dev/null 2>&1 && fail "scenario 7sexies : la phase SCALE_THREADS ne doit rien produire quand elle n'est pas demandee"

echo "selftest_campagne_v5 : violations=${BAD}"
[ "${BAD}" -eq 0 ] || exit 1
echo "PROTOCOLE CONFORME"
