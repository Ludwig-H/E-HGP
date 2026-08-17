#!/usr/bin/env bash
# SELFTEST TRANSACTIONNEL du protocole de campagne v4 (audits « rupture
# SSH » § 4 et « pin source » § 3) — sans une minute de G4 ni de gcloud.
#
# Il execute LE MEME script distant (v4_campaign_remote.sh) et LE MEME
# validateur (validate_v4_campaign.py) que la session, avec un faux probe
# et un faux GNU time, et exige :
#   1. HAPPY PATH : 28 statuts, tous code=0, pin present partout,
#      validateur -> complete (rc 0) ;
#   2. RUNS EN ECHEC : le faux probe rend 7 sur terrain et depasse le
#      timeout sur eight_clusters — les .status MATERIALISENT code=7 et
#      code=124, la campagne va au bout, le validateur -> partial (rc 1)
#      en LISTANT ces runs ;
#   3. PILOTE AVORTE : le pilote n=16000 echoue — la couverture est
#      REFUSEE (exit 3 du script distant), aucun fichier cov_*, les
#      statuts des pilotes restent disponibles ;
#   4. codes de session : remote_rc/scp_rc non nuls -> jamais complete.
# La mesure REELLE du pic RSS est deleguee a GNU time sur la VM (exige au
# build par la session) ; le faux time n'atteste ici que le PROTOCOLE.
# Codes : 0 conforme, 1 desaccord de scenario.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK="$(mktemp -d /tmp/ehgp-v4selftest.XXXXXXXX)"
trap 'rm -rf "${WORK}"' EXIT
BAD=0
fail() { echo "SELFTEST : $1" >&2; BAD=$((BAD + 1)); }

# Faux GNU time : execute la commande, ecrit un pic constant — le
# protocole seul est teste, jamais la mesure.
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
echo "Maximum resident set size (kbytes): 300000" > "${out}"
exit ${rc}
EOF
chmod +x "${WORK}/fake_time"

# Faux probe : la ligne de compteurs attendue par le validateur ; le
# comportement depend de la famille (FAIL_FAMILY -> code 7,
# HANG_FAMILY -> depasse RUN_TIMEOUT) et de PILOT_FAIL_N (pilote avorte).
cat > "${WORK}/fake_probe" <<'EOF'
#!/usr/bin/env bash
fam=""; n=""
for a in "$@"; do
  case "$a" in
    --family=*) fam="${a#--family=}" ;;
    --n=*) n="${a#--n=}" ;;
  esac
done
[ -n "${PILOT_FAIL_N:-}" ] && [ "${n}" = "${PILOT_FAIL_N}" ] && exit 9
[ "${fam}" = "${FAIL_FAMILY:-}" ] && exit 7
[ "${fam}" = "${HANG_FAMILY:-}" ] && sleep 30
echo "famille=${fam} n=${n} candidats=1/1/1 boules_uniques=42 evenements=7 fusions=3 noeuds=1 juge=off desaccords=NA t_gen_ms=1.0"
EOF
chmod +x "${WORK}/fake_probe"

run_campaign() {
  local out="$1"; shift
  mkdir -p "${out}"
  local rc=0
  ( cd "${WORK}" && env "$@" \
      PROBE_BIN="${WORK}/fake_probe" TIME_BIN="${WORK}/fake_time" \
      OUT_DIR="${out}" RUN_TIMEOUT=2 SLEEP_S=0 \
      bash "${HERE}/v4_campaign_remote.sh" cafedeca beefbeef ) \
    > "${out}.log" 2>&1 || rc=$?
  echo "${rc}"
}

# ---- Scenario 1 : happy path -> complete.
RC1=$(run_campaign "${WORK}/out1")
[ "${RC1}" -eq 0 ] || fail "scenario 1 : script distant rc=${RC1}"
[ "$(ls "${WORK}/out1"/*.status 2>/dev/null | wc -l)" -eq 28 ] ||
  fail "scenario 1 : 28 statuts attendus"
grep -L '^source_commit=cafedeca$' "${WORK}/out1"/*.status | grep -q . &&
  fail "scenario 1 : pin source absent d'un statut"
if ! python3 "${HERE}/validate_v4_campaign.py" "${WORK}/out1" cafedeca beefbeef 0 0 \
     > "${WORK}/v1.log" 2>&1; then
  fail "scenario 1 : validateur a refuse un happy path ($(cat "${WORK}/v1.log"))"
fi
grep -q 'campaign_status=complete' "${WORK}/v1.log" ||
  fail "scenario 1 : complete attendu"

# ---- Scenario 2 : echec (code 7) + timeout (code 124) MATERIALISES,
# campagne au bout, validateur partial en les listant.
RC2=$(run_campaign "${WORK}/out2" FAIL_FAMILY=terrain HANG_FAMILY=eight_clusters)
[ "${RC2}" -eq 0 ] || fail "scenario 2 : la campagne doit aller au bout (rc=${RC2})"
grep -q '^code=7$' "${WORK}/out2/cov_terrain_n8000_smax11.status" 2>/dev/null ||
  fail "scenario 2 : code=7 non materialise"
grep -q '^code=124$' "${WORK}/out2/cov_eight_clusters_n8000_smax11.status" 2>/dev/null ||
  fail "scenario 2 : code=124 (timeout) non materialise"
if python3 "${HERE}/validate_v4_campaign.py" "${WORK}/out2" cafedeca beefbeef 0 0 \
     > "${WORK}/v2.log" 2>&1; then
  fail "scenario 2 : le validateur devait refuser"
fi
grep -q 'cov_terrain' "${WORK}/v2.log" && grep -q 'cov_eight_clusters' "${WORK}/v2.log" ||
  fail "scenario 2 : les runs en echec doivent etre LISTES"

# ---- Scenario 3 : pilote avorte -> couverture REFUSEE, statuts conserves.
RC3=$(run_campaign "${WORK}/out3" PILOT_FAIL_N=16000)
[ "${RC3}" -eq 3 ] || fail "scenario 3 : exit 3 attendu (rc=${RC3})"
ls "${WORK}/out3"/cov_*.txt >/dev/null 2>&1 &&
  fail "scenario 3 : aucune couverture ne devait etre lancee"
grep -q '^code=9$' "${WORK}/out3/lat_uniform_n16000_smax11.status" 2>/dev/null ||
  fail "scenario 3 : le statut du pilote avorte doit etre conserve"

# ---- Scenario 4 : un code de session non nul interdit complete.
if python3 "${HERE}/validate_v4_campaign.py" "${WORK}/out1" cafedeca beefbeef 255 0 \
     > "${WORK}/v4.log" 2>&1; then
  fail "scenario 4 : remote_rc=255 devait refuser complete"
fi

echo "selftest_campagne_v4 : violations=${BAD}"
[ "${BAD}" -eq 0 ] || exit 1
echo "PROTOCOLE CONFORME"
