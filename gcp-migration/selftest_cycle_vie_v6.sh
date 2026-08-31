#!/usr/bin/env bash
# SELFTEST DU CYCLE DE VIE de la session G4 v6 — A LANCER A LA MAIN avant
# toute session payante (jamais depuis la CI). Ne touche JAMAIS GCP : il
# execute gcp-migration/v6_session_lifecycle.sh avec de FAUSSES GARDES
# (set_max/start/stop) et un FAUX gcloud (PATH), et injecte un echec apres
# chaque frontiere critique (P0-1 de l'audit GCP v6, point 6). Des qu'un
# start est atteste (temoin de mutation), chaque scenario doit observer
# EXACTEMENT UNE tentative d'arret ciblee (generation exacte) ou un BLOCAGE
# explicite pour generation illisible ; un refus anterieur a la mutation ne
# doit provoquer ni arret ni faux blocage.
#
# Scenarios :
#   0. budget : matrice trop large pour la fenetre => refus AVANT toute garde ;
#   1. refus de preflight sans mutation (start rc=2, aucun temoin) =>
#      propagation, 0 arret, 0 blocage ;
#   2. mutation commencee SANS handoff (temoin ecrit, start rc=3) =>
#      BLOCAGE explicite (exit 71, commande de controle), 0 arret aveugle ;
#   3. start certifie mais handoff corrompu => BLOCAGE (exit 71), 0 arret ;
#   4. echeance inderivable (timestamp non parsable) => UN arret cible avec
#      la generation exacte ;
#   5. echec du build SSH => UN arret cible ;
#   6. campagne SSH en echec + JOURNAL VERROUILLE (chmod 444 pendant la
#      campagne) => le cleanup survit a la panne de journal, UN arret cible ;
#   7. nominal mecanique (toutes etapes passent, validateur reel sur un out
#      vide => rc=65) => UN arret cible, exit 65 ;
#   8. arret en echec (stop rc=9) => exit 70, ARRET NON CERTIFIE ;
#   9. P0-2 : une garde alteree dans un clone => v6_campaign_pin.sh refuse
#      (code 2) avant toute mutation, pour CHACUNE des trois gardes, le
#      cycle de vie, le runner, le validateur et le lanceur.
# Code de sortie : 0 conforme, 1 au moins un scenario en echec.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIFECYCLE="${HERE}/v6_session_lifecycle.sh"
ROOT="$(cd "${HERE}/.." && pwd)"
BASE="$(mktemp -d /tmp/ehgp-v6cyclevie.XXXXXXXX)"
trap 'rm -rf "${BASE}"' EXIT

FAILURES=0
check() {
  local name="$1" rc="$2"
  if [ "${rc}" -ne 0 ]; then
    echo "ECHEC selftest : ${name}" >&2
    FAILURES=$((FAILURES + 1))
  else
    echo "ok : ${name}"
  fi
}
check_true() { local name="$1"; shift; if "$@"; then check "${name}" 0; else check "${name}" 1; fi; }

FAKE_GEN="2026-08-31T14:00:00Z"

# ---- Faux gcloud (PATH) : describe rend la generation, ssh/scp pilotes par
# FAKE_SSH_MODE, tout le reste rend 0. Compte ses appels ssh dans CALLS.
make_fake_bin() { # $1 = dossier
  cat > "$1/gcloud" <<'EOF'
#!/usr/bin/env bash
echo "GCLOUD $*" >> "${FAKE_CALLS}"
case "$*" in
  *"instances describe"*) echo "${FAKE_GEN}"; exit 0 ;;
  *"compute ssh"*)
    n=$(grep -c "compute ssh" "${FAKE_CALLS}" || true)
    if [ "${n}" -le 1 ]; then
      # premier SSH = build + portes
      if [ "${FAKE_SSH_MODE:-ok}" = "build_fail" ]; then echo "build casse" >&2; exit 1; fi
      echo "boot_id=aaaa-bbbb-cccc"
      echo "100% tests passed, 0 tests failed out of 45"
      echo "100% tests passed, 0 tests failed out of 60"
      exit 0
    fi
    # second SSH = campagne
    if [ "${FAKE_SSH_MODE:-ok}" = "campagne_fail_et_journal_verrouille" ]; then
      chmod 444 "${FAKE_LOG_A_VERROUILLER}" 2>/dev/null || true
      echo "campagne cassee" >&2
      exit 1
    fi
    echo "=== fin des runs (la validation locale decide du statut, jamais cette ligne) ==="
    exit 0
    ;;
  *"compute scp"*)
    # rapatriement : materialise un out/ vide (le validateur reel refusera).
    mkdir -p "${FAKE_SCP_DEST}/out"
    exit 0
    ;;
  *) exit 0 ;;
esac
EOF
  chmod +x "$1/gcloud"
}

# ---- Fausses gardes : chaque appel est journalise ; start pilote par
# FAKE_START_MODE ; stop rend FAKE_STOP_RC.
make_fake_guards() { # $1 = dossier
  cat > "$1/set_max_run_duration_and_verify.sh" <<'EOF'
#!/usr/bin/env bash
echo "SETMAX $*" >> "${FAKE_CALLS}"
exit "${FAKE_SETMAX_RC:-0}"
EOF
  cat > "$1/start_and_verify.sh" <<'EOF'
#!/usr/bin/env bash
echo "START $*" >> "${FAKE_CALLS}"
handoff=""; witness=""
while [ $# -gt 0 ]; do
  case "$1" in
    --handoff-file) handoff="$2"; shift 2 ;;
    --mutation-witness-file) witness="$2"; shift 2 ;;
    *) shift ;;
  esac
done
write_witness() { printf 'schema=e-hgp.start-mutation-witness.v1\n' > "${witness}"; }
write_handoff() { # $1 = last_start_timestamp
  printf '{"guest_shutdown_minutes":470,"instance":"%s","last_start_timestamp":"%s","project":"%s","schema":"e-hgp.start-handoff.v3","status":"targeted_running","zone":"%s"}\n' \
    "${GCP_INSTANCE_NAME}" "$1" "${GCP_PROJECT_ID}" "${GCP_ZONE}" > "${handoff}"
}
case "${FAKE_START_MODE:-ok}" in
  refus_preflight) exit 2 ;;
  mutation_sans_handoff) write_witness; exit 3 ;;
  ok_handoff_corrompu) write_witness; echo "pas du json" > "${handoff}"; exit 0 ;;
  ok_bad_timestamp) write_witness; write_handoff "pas-une-date"; exit 0 ;;
  ok) write_witness; write_handoff "${FAKE_GEN}"; exit 0 ;;
esac
EOF
  cat > "$1/stop_and_verify.sh" <<'EOF'
#!/usr/bin/env bash
echo "STOP $*" >> "${FAKE_CALLS}"
exit "${FAKE_STOP_RC:-0}"
EOF
  chmod +x "$1"/*.sh
}

# ---- Un scenario : prepare WORK (pinned + bundle + gardes fausses), lance
# le cycle de vie, rend son rc ; CALLS et LOG restent inspectables.
# run_scenario START_MODE [SSH_MODE] [STOP_RC] [MAX_RUN] — arguments
# explicites : aucune fuite d'environnement entre scenarios.
SCENARIO_DIR=""
run_scenario() {
  local start_mode="$1" ssh_mode="${2:-ok}" stop_rc="${3:-0}" max_run="${4:-28800}"
  SCENARIO_DIR="$(mktemp -d "${BASE}/scenario.XXXXXX")"
  local W="${SCENARIO_DIR}/work"
  mkdir -p "${W}/pinned/gcp-migration" "${SCENARIO_DIR}/bin" "${SCENARIO_DIR}/guards"
  make_fake_bin "${SCENARIO_DIR}/bin"
  make_fake_guards "${SCENARIO_DIR}/guards"
  cp "${HERE}/v6_campaign_remote.sh" "${W}/pinned/gcp-migration/"
  cp "${HERE}/validate_v6_campaign.py" "${W}/pinned/gcp-migration/"
  echo "bundle factice" > "${W}/bundle.tgz"
  local payload_sha
  payload_sha="$(sha256sum "${W}/bundle.tgz" | awk '{print $1}')"
  export FAKE_CALLS="${SCENARIO_DIR}/calls.log"
  : > "${FAKE_CALLS}"
  export FAKE_GEN FAKE_SCP_DEST="${W}" FAKE_LOG_A_VERROUILLER="${W}/session.log"
  local rc=0
  env PATH="${SCENARIO_DIR}/bin:${PATH}" \
    FAKE_START_MODE="${start_mode}" FAKE_SSH_MODE="${ssh_mode}" FAKE_STOP_RC="${stop_rc}" \
    MAX_RUN_SECONDS="${max_run}" \
    MHGP6_LIFECYCLE_WORK="${W}" \
    MHGP6_LIFECYCLE_GUARDS_DIR="${SCENARIO_DIR}/guards" \
    MHGP6_LIFECYCLE_SOURCE_COMMIT="0000000000000000000000000000000000000000" \
    MHGP6_LIFECYCLE_PAYLOAD_SHA256="${payload_sha}" \
    MHGP6_LIFECYCLE_MANIFEST_SHA256="1111111111111111111111111111111111111111111111111111111111111111" \
    GCP_PROJECT_ID=projet-factice GCP_ZONE=zone-factice GCP_INSTANCE_NAME=instance-factice \
    bash "${LIFECYCLE}" > "${SCENARIO_DIR}/stdout.log" 2> "${SCENARIO_DIR}/stderr.log" || rc=$?
  return "${rc}"
}

# ---- 0. Budget : matrice trop large => refus AVANT toute garde (le seul
# appel gcloud autorise est `config set project`, en lecture de config).
rc=0; run_scenario ok ok 0 3600 || rc=$?
check_true "budget : refus rc=2 avant toute garde" \
  bash -c "[ '${rc}' -eq 2 ] && ! grep -qE '^(SETMAX|START|STOP) ' '${FAKE_CALLS}'"

# ---- 1. Refus de preflight sans mutation : propagation, 0 arret, 0 blocage.
rc=0; run_scenario refus_preflight || rc=$?
check_true "refus preflight : propagation rc=2, aucun arret, aucun blocage" \
  bash -c "[ '${rc}' -eq 2 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 0 ] && ! grep -q BLOCAGE '${SCENARIO_DIR}/stderr.log'"

# ---- 2. Mutation commencee sans handoff : BLOCAGE explicite, 0 arret.
rc=0; run_scenario mutation_sans_handoff || rc=$?
check_true "mutation sans handoff : blocage exit 71, commande de controle, aucun arret" \
  bash -c "[ '${rc}' -eq 71 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 0 ] \
    && grep -q 'BLOCAGE' '${SCENARIO_DIR}/stderr.log' \
    && grep -q 'instances describe instance-factice' '${SCENARIO_DIR}/stderr.log'"

# ---- 3. Handoff corrompu apres start certifie : BLOCAGE, 0 arret.
rc=0; run_scenario ok_handoff_corrompu || rc=$?
check_true "handoff corrompu : blocage exit 71, aucun arret aveugle" \
  bash -c "[ '${rc}' -eq 71 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 0 ] \
    && grep -q 'BLOCAGE' '${SCENARIO_DIR}/stderr.log'"

# ---- 4. Echeance inderivable : UN arret cible avec la generation exacte.
rc=0; run_scenario ok_bad_timestamp || rc=$?
check_true "echeance inderivable : un arret cible sur la generation exacte" \
  bash -c "[ '${rc}' -eq 73 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q -- '--expected-last-start-timestamp pas-une-date' '${FAKE_CALLS}'"

# ---- 5. Echec du build SSH : UN arret cible.
rc=0; run_scenario ok build_fail || rc=$?
check_true "build SSH en echec : un arret cible" \
  bash -c "[ '${rc}' -ne 0 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q -- '--expected-last-start-timestamp ${FAKE_GEN}' '${FAKE_CALLS}'"

# ---- 6. Campagne en echec + journal verrouille : le cleanup survit, UN arret.
rc=0; run_scenario ok campagne_fail_et_journal_verrouille || rc=$?
check_true "journal verrouille pendant la campagne : un arret cible malgre la panne de journal" \
  bash -c "[ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q -- '--expected-last-start-timestamp ${FAKE_GEN}' '${FAKE_CALLS}'"

# ---- 7. Nominal mecanique : validateur reel sur out vide => 65, UN arret.
rc=0; run_scenario ok || rc=$?
check_true "nominal mecanique : exit 65 (validateur), un arret cible, ordre des etapes" \
  bash -c "[ '${rc}' -eq 65 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q '^SETMAX ' '${FAKE_CALLS}' && grep -q '^START ' '${FAKE_CALLS}' \
    && [ \"\$(grep -n '^STOP ' '${FAKE_CALLS}' | tail -1 | cut -d: -f1)\" -eq \"\$(wc -l < '${FAKE_CALLS}')\" ]"

# ---- 8. Arret en echec : exit 70, ARRET NON CERTIFIE.
rc=0; run_scenario ok build_fail 9 || rc=$?
check_true "arret en echec : exit 70 et ARRET NON CERTIFIE" \
  bash -c "[ '${rc}' -eq 70 ] && grep -q 'ARRET NON CERTIFIE' '${SCENARIO_DIR}/stderr.log'"

# ---- 9. P0-2 : alteration de chaque fichier du protocole => le pin refuse
# (code 2) avant toute mutation. Clone local partage (aucun reseau, le
# worktree reel n'est JAMAIS touche) ; les versions COURANTES du protocole y
# sont synchronisees depuis le worktree et committees DANS LE CLONE JETABLE
# (le selftest doit pouvoir prouver le refus avant que le protocole ne soit
# committe sur main).
CLONE="${BASE}/clone"
git clone --quiet --shared --no-hardlinks "${ROOT}" "${CLONE}" 2>/dev/null
for f in session_campagne_v6_g4.sh v6_session_lifecycle.sh v6_campaign_pin.sh \
         v6_campaign_remote.sh validate_v6_campaign.py \
         set_max_run_duration_and_verify.sh start_and_verify.sh stop_and_verify.sh; do
  cp "${HERE}/${f}" "${CLONE}/gcp-migration/${f}"
done
( cd "${CLONE}" \
  && git -c user.name=selftest -c user.email=selftest@local add -- \
       gcp-migration/session_campagne_v6_g4.sh gcp-migration/v6_session_lifecycle.sh \
       gcp-migration/v6_campaign_pin.sh gcp-migration/v6_campaign_remote.sh \
       gcp-migration/validate_v6_campaign.py gcp-migration/set_max_run_duration_and_verify.sh \
       gcp-migration/start_and_verify.sh gcp-migration/stop_and_verify.sh \
  && git -c user.name=selftest -c user.email=selftest@local commit --quiet -m "selftest : protocole courant" )
# Sanite : le pin du clone DOIT accepter l'etat propre avant les alterations.
rc=0
( cd "${CLONE}" && ./gcp-migration/v6_campaign_pin.sh "$(mktemp -d "${BASE}/pin.XXXXXX")" ) >/dev/null 2>&1 || rc=$?
check_true "pin du clone propre : accepte (prealable des refus)" [ "${rc}" -eq 0 ]
for f in set_max_run_duration_and_verify.sh start_and_verify.sh stop_and_verify.sh \
         v6_session_lifecycle.sh v6_campaign_remote.sh validate_v6_campaign.py \
         session_campagne_v6_g4.sh; do
  ( cd "${CLONE}" && echo "# alteration" >> "gcp-migration/${f}" )
  rc=0
  ( cd "${CLONE}" && ./gcp-migration/v6_campaign_pin.sh "$(mktemp -d "${BASE}/pin.XXXXXX")" ) >/dev/null 2>&1 || rc=$?
  check_true "pin refuse la garde/le script altere : ${f}" [ "${rc}" -eq 2 ]
  ( cd "${CLONE}" && git checkout --quiet -- "gcp-migration/${f}" )
done

if [ "${FAILURES}" -ne 0 ]; then
  echo "selftest cycle de vie v6 : ${FAILURES} echec(s)" >&2
  exit 1
fi
echo "selftest cycle de vie v6 : arret cible prouve sur chaque sortie apres demarrage (10 scenarios + 7 refus de pin)"
