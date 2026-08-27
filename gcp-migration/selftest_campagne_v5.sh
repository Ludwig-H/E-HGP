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
#   6. statut sans manifeste de protocole -> jamais complete.
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
echo "Maximum resident set size (kbytes): 300000" > "${out}"
exit ${rc}
EOS
chmod +x "${WORK}/fake_time"

# Faux pilote 50 k : compteurs v5 + digest ; FAIL_FAMILY -> 7, HANG_FAMILY -> timeout.
cat > "${WORK}/fake_probe" <<'EOS'
#!/usr/bin/env bash
fam=""; n=""
for a in "$@"; do
  case "$a" in
    --family=*) fam="${a#--family=}" ;;
    --n=*) n="${a#--n=}" ;;
  esac
done
[ "${fam}" = "${FAIL_FAMILY:-}" ] && exit 7
[ "${fam}" = "${HANG_FAMILY:-}" ] && sleep 30
echo "famille=${fam} n=${n} coord=1 s=8 smax=11 seed=3 threads=1 emis=1 boules_uniques=42 mortes_profondeur=0 survivantes=42 census_int=1 census_shell=0 evenements=7 facettes=9 fusions=3 deltas=1 noeuds=1"
echo "digest_all=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
EOS
chmod +x "${WORK}/fake_probe"

# Faux pilote CUDA : meme sortie que la sonde, plus la ligne gpu=1 ; GPU_DIGEST_MISM=1
# change le digest (doit etre refuse).
cat > "${WORK}/fake_gpu" <<'EOS'
#!/usr/bin/env bash
fam=""; n=""
for a in "$@"; do case "$a" in --family=*) fam="${a#--family=}";; --n=*) n="${a#--n=}";; esac; done
echo "famille=${fam} n=${n} coord=1 s=8 smax=11 seed=3 threads=1 emis=1 boules_uniques=42 mortes_profondeur=0 survivantes=42 census_int=1 census_shell=0 evenements=7 facettes=9 fusions=3 deltas=1 noeuds=1"
if [ "${GPU_DIGEST_MISM:-0}" = "1" ]; then echo "digest_all=ffff456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"; else echo "digest_all=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"; fi
echo "gpu=1 kernel_ms=1.0 lancements=3"
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
[ "$(ls "${WORK}/out1"/*.status 2>/dev/null | wc -l)" -eq 22 ] || fail "scenario 1 : 22 statuts attendus (temoin + lane device + 12 conformites + 4 contrats CPU + 4 contrats GPU)"
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
[ "$(ls "${WORK}/out3d"/*.status 2>/dev/null | wc -l)" -eq 22 ] || fail "scenario 3quater : la lane device ne refuse pas les phases CPU"
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

echo "selftest_campagne_v5 : violations=${BAD}"
[ "${BAD}" -eq 0 ] || exit 1
echo "PROTOCOLE CONFORME"
