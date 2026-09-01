#!/usr/bin/env bash
# SELFTEST DU CYCLE DE VIE de la session G4 v6 — A LANCER A LA MAIN avant
# toute session payante (jamais depuis la CI). Ne touche JAMAIS GCP : il
# execute gcp-migration/v6_session_lifecycle.sh avec de FAUSSES GARDES
# (set_max/start/stop, enregistrement d'etat de cycle de vie compris) et un
# FAUX gcloud (PATH), et injecte un echec apres chaque frontiere critique
# (P0-1 de l'audit GCP v6, deux tours). Des qu'un start est atteste
# (enregistrement d'etat), chaque scenario doit observer EXACTEMENT UNE
# tentative d'arret ciblee (generation exacte), un BLOCAGE explicite pour
# generation illisible, ou la reconnaissance d'un arret DEJA certifie par le
# garde (jamais un second arret ni un faux blocage).
#
# Scenarios (cycle de vie, fausses gardes) :
#   0. budget : matrice trop large => refus AVANT toute garde ;
#   1. refus de preflight sans mutation => propagation, 0 arret, 0 blocage ;
#   2. mutation attestee SANS generation (etat start_may_have_been_requested,
#      pas de handoff) => BLOCAGE exit 71, commande de controle, 0 arret ;
#   3. handoff corrompu mais generation dans l'ETAT (targeted_running) =>
#      UN arret cible sur la generation exacte (plus jamais un faux blocage) ;
#   4. echeance inderivable (timestamp non parsable) => UN arret cible ;
#   5. echec du build SSH => UN arret cible ;
#   6. campagne en echec + JOURNAL VERROUILLE => cleanup survit, UN arret ;
#   7. nominal mecanique => exit 65 (validateur reel sur out vide), UN arret
#      en DERNIER appel, recu durable publie (RECU_SESSION.txt + SHA256SUMS) ;
#   8. arret en echec (stop rc=9) => exit 70, ARRET NON CERTIFIE ;
#   9. ARRET DEJA CERTIFIE PAR LE GARDE (etat targeted_stopped) => 0 arret,
#      0 blocage, propagation, message « arret deja certifie » (contre-
#      scenario du deuxieme tour) ;
#  10. TTL OS Login par defaut : la cle demandee tient dans la fenetre du
#      preflight du garde (MAX_RUN_SECONDS + 660 s).
# Scenarios bootstrap + pin (clone jetable, worktree jamais touche) :
#  11. pin altere dans le worktree du clone + garde alteree : l'etage 1
#      materialise le pin DU COMMIT, qui refuse (le pin altere qui
#      neutraliserait son propre controle n'est JAMAIS execute) ;
#  12. bootstrap altere dans le worktree : l'etage 1 refuse (identite) ;
#  13. chaque fichier du protocole altere => le pin refuse (code 2).
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

# Generation RELATIVE au present : l'echeance du cycle de vie derive du
# cutoff min(GCE, invite) depuis GEN_EPOCH — un horodatage fige casserait
# les scenarios au fil de l'horloge.
FAKE_GEN="$(date -u -d '-60 seconds' +%Y-%m-%dT%H:%M:%SZ)"
# § 5.15.4 : chemins REPO-RELATIFS, meme inventaire et meme ordre que les
# deux listes normatives (pin + lifecycle) — treize fichiers dans DEUX
# repertoires (le juge du pilote vit sous morsehgp3D_v6/tests/).
PROTOCOL_FILES=(gcp-migration/session_campagne_v6_g4.sh gcp-migration/v6_session_lifecycle.sh
                gcp-migration/v6_campaign_pin.sh gcp-migration/v6_campaign_remote.sh
                gcp-migration/validate_v6_campaign.py gcp-migration/profils/decision_v1.env
                gcp-migration/profils/smoke_v1.env gcp-migration/profils/g4_mesure_v1.env
                gcp-migration/profils/g4_serie_c_v1.env morsehgp3D_v6/tests/pilote_juge.py
                gcp-migration/set_max_run_duration_and_verify.sh
                gcp-migration/start_and_verify.sh gcp-migration/stop_and_verify.sh)

# ---- Faux gcloud (PATH) : describe rend la generation ; ssh n1 = handshake
# boot_id seul, n2 = build (portes), n3 = campagne ; scp materialise out/.
make_fake_bin() { # $1 = dossier
  cat > "$1/gcloud" <<'EOF'
#!/usr/bin/env bash
echo "GCLOUD $*" >> "${FAKE_CALLS}"
case "$*" in
  *"instances describe"*)
    if [ "${FAKE_DESCRIBE_HANG:-0}" = "1" ]; then sleep 3600; fi
    echo "${FAKE_GEN}"; exit 0 ;;
  *"compute ssh"*)
    n=$(grep -c "compute ssh" "${FAKE_CALLS}" || true)
    if [ "${n}" -le 1 ]; then
      echo "aaaabbbb-cccc-dddd-eeee-ffff00001111"
      exit 0
    fi
    if [ "${n}" -eq 2 ]; then
      if [ "${FAKE_SSH_MODE:-ok}" = "build_fail" ]; then echo "build casse" >&2; exit 1; fi
      if [ "${FAKE_SSH_MODE:-ok}" = "build_lent" ]; then sleep "${FAKE_BUILD_SLEEP_S:-90}"; fi
      # UN bloc par format de resume CTest (<=4.3 et 4.4+) : le parseur des
      # portes doit accepter les deux (refus du 1er septembre).
      echo "100% tests passed, 0 tests failed out of 45"
      echo "100% tests passed out of 60"
      exit 0
    fi
    if [ "${FAKE_SSH_MODE:-ok}" = "campagne_fail_et_journal_verrouille" ]; then
      chmod 444 "${FAKE_LOG_A_VERROUILLER}" 2>/dev/null || true
      echo "campagne cassee" >&2
      exit 1
    fi
    # Mutants permanents du REGISTRE (audit cinquieme tour) : la campagne
    # echoue APRES avoir perdu/corrompu le registre — le cleanup doit encore
    # arreter via le handoff, jamais conclure « avant mutation ».
    case "${FAKE_SSH_MODE:-ok}" in
      campagne_supprime_registre) rm -f "${FAKE_STATE_CIBLE}"; echo "campagne cassee" >&2; exit 1 ;;
      campagne_registre_duplique) echo "state=targeted_running" >> "${FAKE_STATE_CIBLE}"; echo "campagne cassee" >&2; exit 1 ;;
      campagne_registre_sans_schema) sed -i '/^schema=/d' "${FAKE_STATE_CIBLE}"; echo "campagne cassee" >&2; exit 1 ;;
      campagne_registre_tronque) printf 'schema=e-hgp.lifecycle-state.v1\nstate=targeted_runn' > "${FAKE_STATE_CIBLE}"; echo "campagne cassee" >&2; exit 1 ;;
    esac
    echo "=== fin des runs (la validation locale decide du statut, jamais cette ligne) ==="
    exit 0
    ;;
  *"compute scp"*)
    mkdir -p "${FAKE_SCP_DEST}/out"
    exit 0
    ;;
  *) exit 0 ;;
esac
EOF
  chmod +x "$1/gcloud"
}

# ---- Fausses gardes : start pilote par FAKE_START_MODE et ecrit le fichier
# --lifecycle-state-file comme le vrai garde ; stop rend FAKE_STOP_RC.
make_fake_guards() { # $1 = dossier
  cat > "$1/set_max_run_duration_and_verify.sh" <<'EOF'
#!/usr/bin/env bash
echo "SETMAX $*" >> "${FAKE_CALLS}"
exit "${FAKE_SETMAX_RC:-0}"
EOF
  cat > "$1/start_and_verify.sh" <<'EOF'
#!/usr/bin/env bash
echo "START $*" >> "${FAKE_CALLS}"
handoff=""; state=""
while [ $# -gt 0 ]; do
  case "$1" in
    --handoff-file) handoff="$2"; shift 2 ;;
    --lifecycle-state-file) state="$2"; shift 2 ;;
    *) shift ;;
  esac
done
write_state() { # $1 = etat, $2 = generation
  printf 'schema=e-hgp.lifecycle-state.v1\nstate=%s\nproject=%s\nzone=%s\ninstance=%s\ngeneration=%s\n' \
    "$1" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}" "$2" > "${state}"
}
write_handoff() { # $1 = last_start_timestamp
  printf '{"guest_shutdown_minutes":470,"instance":"%s","last_start_timestamp":"%s","project":"%s","schema":"e-hgp.start-handoff.v3","status":"targeted_running","zone":"%s"}\n' \
    "${GCP_INSTANCE_NAME}" "$1" "${GCP_PROJECT_ID}" "${GCP_ZONE}" > "${handoff}"
}
case "${FAKE_START_MODE:-ok}" in
  refus_preflight) exit 2 ;;
  mutation_sans_generation) write_state start_may_have_been_requested ""; exit 3 ;;
  ok_handoff_corrompu) write_state targeted_running "${FAKE_GEN}"; echo "pas du json" > "${handoff}"; exit 0 ;;
  ok_bad_timestamp) write_state targeted_running "pas-une-date"; write_handoff "pas-une-date"; exit 0 ;;
  stopped_by_guard) write_state targeted_stopped "${FAKE_GEN}"; exit 5 ;;
  stopped_wrong_generation) write_state targeted_stopped "generation-perimee"; write_handoff "${FAKE_GEN}"; exit 5 ;;
  registre_illisible) write_state targeted_running "${FAKE_GEN}"; chmod 000 "${state}"; exit 4 ;;
  publication_interrompue) printf 'schema=e-hgp.lifecycle-st' > "$(dirname "${state}")/.$(basename "${state}").zzz.partial"; exit 3 ;;
  stop_failed_by_guard)
    # REPRISE EXECUTEE (cinquieme tour) : le garde tente REELLEMENT son arret
    # interne — c'est son echec observe, jamais un etat fabrique, qui produit
    # targeted_stop_failed.
    if "$(dirname "$0")/stop_and_verify.sh" --yes --expected-last-start-timestamp "${FAKE_GEN}"; then
      write_state targeted_stopped "${FAKE_GEN}"
    else
      write_state targeted_stop_failed "${FAKE_GEN}"
    fi
    exit 6 ;;
  ok) write_state targeted_running "${FAKE_GEN}"; write_handoff "${FAKE_GEN}"; exit 0 ;;
esac
EOF
  cat > "$1/stop_and_verify.sh" <<'EOF'
#!/usr/bin/env bash
echo "STOP $*" >> "${FAKE_CALLS}"
n=$(grep -c '^STOP ' "${FAKE_CALLS}" || true)
if [ "${FAKE_STOP_FAIL_FIRST:-0}" = "1" ] && [ "${n}" -le 1 ]; then exit 1; fi
if [ "${FAKE_STOP_VANISH_STATE:-0}" = "1" ]; then
  rm -f "${FAKE_STATE_CIBLE}"
  chmod 555 "$(dirname "${FAKE_STATE_CIBLE}")"
fi
if [ "${FAKE_STOP_FOREIGN_STATE:-0}" = "1" ]; then
  printf 'schema=e-hgp.lifecycle-state.v1\nstate=targeted_stopped\nproject=autre-projet\nzone=autre-zone\ninstance=autre-instance\ngeneration=2020-01-01T00:00:00Z\n' > "${FAKE_STATE_CIBLE}"
  chmod 555 "$(dirname "${FAKE_STATE_CIBLE}")"
fi
exit "${FAKE_STOP_RC:-0}"
EOF
  chmod +x "$1"/*.sh
}

# ---- Un scenario : pinned/ complet (les DIX fichiers du protocole, copies
# du worktree) + manifeste canonique recalcule, gardes FAUSSES dans un
# dossier separe, recu durable dans le scenario.
# run_scenario START_MODE [SSH_MODE] [STOP_RC] [MAX_RUN]
SCENARIO_DIR=""
run_scenario() {
  local start_mode="$1" ssh_mode="${2:-ok}" stop_rc="${3:-0}" max_run="${4:-28800}"
  SCENARIO_DIR="$(mktemp -d "${BASE}/scenario.XXXXXX")"
  local W="${SCENARIO_DIR}/work"
  mkdir -p "${W}/pinned/gcp-migration/profils" "${W}/pinned/morsehgp3D_v6/tests" \
           "${SCENARIO_DIR}/bin" "${SCENARIO_DIR}/guards" "${SCENARIO_DIR}/recu"
  make_fake_bin "${SCENARIO_DIR}/bin"
  make_fake_guards "${SCENARIO_DIR}/guards"
  for f in "${PROTOCOL_FILES[@]}"; do
    cp "${ROOT}/${f}" "${W}/pinned/${f}"
  done
  if [ "${FAKE_VALIDATOR_STUB:-0}" = "1" ]; then
    # Substitue AVANT le calcul du manifeste (la copie epinglee reste donc
    # coherente) : le stub journalise VALIDATE dans le meme ledger que STOP.
    printf '#!/usr/bin/env python3\nimport os\nwith open(os.environ["FAKE_CALLS"], "a") as fh:\n    fh.write("VALIDATE\\n")\nprint("campaign_status=stub")\nraise SystemExit(1)\n' \
      > "${W}/pinned/gcp-migration/validate_v6_campaign.py"
  fi
  echo "bundle factice" > "${W}/bundle.tgz"
  local payload_sha commit="0000000000000000000000000000000000000000"
  payload_sha="$(sha256sum "${W}/bundle.tgz" | awk '{print $1}')"
  {
    echo "schema=e-hgp.protocol-manifest.v1"
    echo "commit=${commit}"
    for f in "${PROTOCOL_FILES[@]}"; do
      printf '%s\t%s\t%s\n' \
        "$(sha256sum "${W}/pinned/${f}" | awk '{print $1}')" \
        "$(wc -c < "${W}/pinned/${f}")" \
        "${f}"
    done
  } > "${SCENARIO_DIR}/manifest.txt"
  local manifest_sha
  manifest_sha="$(sha256sum "${SCENARIO_DIR}/manifest.txt" | awk '{print $1}')"
  export FAKE_CALLS="${SCENARIO_DIR}/calls.log"
  : > "${FAKE_CALLS}"
  export FAKE_GEN FAKE_SCP_DEST="${W}" FAKE_LOG_A_VERROUILLER="${W}/session.log"
  export FAKE_STATE_CIBLE="${W}/etat_cycle_vie"
  local rc=0
  env PATH="${SCENARIO_DIR}/bin:${PATH}" \
    FAKE_START_MODE="${start_mode}" FAKE_SSH_MODE="${ssh_mode}" FAKE_STOP_RC="${stop_rc}" \
    MAX_RUN_SECONDS="${max_run}" \
    DURABLE_RECEIPT_BASE="${SCENARIO_DIR}/recu" DURABLE_RECEIPT_PREFIX="s" \
    MHGP6_LIFECYCLE_WORK="${W}" \
    MHGP6_LIFECYCLE_GUARDS_DIR="${SCENARIO_DIR}/guards" \
    MHGP6_LIFECYCLE_SOURCE_COMMIT="${commit}" \
    MHGP6_LIFECYCLE_PAYLOAD_SHA256="${payload_sha}" \
    MHGP6_LIFECYCLE_MANIFEST_SHA256="${manifest_sha}" \
    GCP_PROJECT_ID=projet-factice GCP_ZONE=zone-factice GCP_INSTANCE_NAME=instance-factice \
    bash "${LIFECYCLE}" > "${SCENARIO_DIR}/stdout.log" 2> "${SCENARIO_DIR}/stderr.log" || rc=$?
  return "${rc}"
}

# ---- 0. Budget : matrice trop large => refus AVANT toute garde (seul un
# `gcloud config set project` en configuration PRIVEE a pu se produire).
rc=0; run_scenario ok ok 0 3600 || rc=$?
check_true "budget : refus rc=2 avant toute garde" \
  bash -c "[ '${rc}' -eq 2 ] && ! grep -qE '^(SETMAX|START|STOP) ' '${FAKE_CALLS}'"

# ---- 1. Refus de preflight sans mutation : propagation, 0 arret, 0 blocage.
rc=0; run_scenario refus_preflight || rc=$?
check_true "refus preflight : propagation rc=2, aucun arret, aucun blocage" \
  bash -c "[ '${rc}' -eq 2 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 0 ] && ! grep -q BLOCAGE '${SCENARIO_DIR}/stderr.log'"

# ---- 2. Mutation attestee sans generation : BLOCAGE explicite, 0 arret.
rc=0; run_scenario mutation_sans_generation || rc=$?
check_true "mutation sans generation : blocage exit 71, commande de controle, aucun arret" \
  bash -c "[ '${rc}' -eq 71 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 0 ] \
    && grep -q 'BLOCAGE' '${SCENARIO_DIR}/stderr.log' \
    && grep -q 'instances describe instance-factice' '${SCENARIO_DIR}/stderr.log'"

# ---- 3. Handoff corrompu, generation dans l'ETAT : UN arret cible (plus
# jamais un faux blocage — l'enregistrement partage porte la generation).
rc=0; run_scenario ok_handoff_corrompu || rc=$?
check_true "handoff corrompu : un arret cible via l'etat partage, aucun blocage" \
  bash -c "[ '${rc}' -eq 72 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q -- '--expected-last-start-timestamp ${FAKE_GEN}' '${FAKE_CALLS}' \
    && ! grep -q 'BLOCAGE' '${SCENARIO_DIR}/stderr.log'"

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

# ---- 7. Nominal mecanique : exit 65, UN arret en dernier, recu durable.
rc=0; run_scenario ok || rc=$?
check_true "nominal mecanique : exit 65 (validateur), un arret cible, ordre des etapes" \
  bash -c "[ '${rc}' -eq 65 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q '^SETMAX ' '${FAKE_CALLS}' && grep -q '^START ' '${FAKE_CALLS}' \
    && [ \"\$(grep -n '^STOP ' '${FAKE_CALLS}' | tail -1 | cut -d: -f1)\" -eq \"\$(wc -l < '${FAKE_CALLS}')\" ]"
check_true "nominal mecanique : recu durable UNIQUE publie atomiquement (RECU + SHA256SUMS + profil)" \
  bash -c "d=\$(ls -d '${SCENARIO_DIR}'/recu/s_* 2>/dev/null | head -1); [ -n \"\$d\" ] \
    && [ -s \"\$d/RECU_SESSION.txt\" ] && [ -s \"\$d/SHA256SUMS\" ] \
    && grep -q 'profil_campagne.txt' \"\$d/SHA256SUMS\" \
    && grep -q '^profil=decision_v1' \"\$d/RECU_SESSION.txt\" \
    && [ -z \"\$(ls -d '${SCENARIO_DIR}'/recu/*.partial 2>/dev/null)\" ]"
# VERIFICATION EXTERIEURE du recu (cinquieme tour) : le manifeste se verifie
# depuis l'exterieur, couvre EXACTEMENT les fichiers publies, les CINQ
# resumes sont durables, et aucun temporaire du VRAI motif `s_*.partial.*`
# (mktemp de finalize_receipt) ne survit.
check_true "nominal mecanique : recu verifie de l'EXTERIEUR (sha256sum -c, couverture exacte, resumes durables, aucun .partial.*)" \
  bash -c "d=\$(ls -d '${SCENARIO_DIR}'/recu/s_* | head -1) \
    && (cd \"\$d\" && sha256sum -c --quiet SHA256SUMS >/dev/null 2>&1) \
    && [ \"\$(cd \"\$d\" && find . -type f ! -name SHA256SUMS | sort)\" = \"\$(cd \"\$d\" && awk '{print \$2}' SHA256SUMS | sort)\" ] \
    && for r in bench_resume queue_resume sweep_resume gpu_resume frontier_resume; do \
         grep -q \"\${r}.txt\" \"\$d/SHA256SUMS\" || exit 1; done \
    && [ -z \"\$(ls -d '${SCENARIO_DIR}'/recu/s_*.partial.* 2>/dev/null)\" ]"
# Registre partage jusqu'au TERMINAL par le cleanup exterieur (troisieme
# tour) : apres un arret nominal certifie, l'etat est targeted_stopped et le
# recu le grave.
check_true "nominal mecanique : registre au terminal targeted_stopped, grave au recu" \
  bash -c "grep -q '^state=targeted_stopped' '${SCENARIO_DIR}/work/etat_cycle_vie' \
    && d=\$(ls -d '${SCENARIO_DIR}'/recu/s_* | head -1) \
    && grep -q '^etat_cycle_vie=targeted_stopped' \"\$d/RECU_SESSION.txt\""
# TTL OS Login par defaut : dans la fenetre du preflight (scenario 10).
check_true "TTL OS Login par defaut dans la fenetre du preflight" \
  bash -c "ttl=\$(grep -oE 'ttl=[0-9]+m' '${FAKE_CALLS}' | head -1 | grep -oE '[0-9]+'); \
    [ -n \"\${ttl}\" ] && [ \$((ttl * 60)) -le \$((28800 + 660)) ]"

# ---- 8. Arret en echec persistant : la seconde reserve est employee
# (dixieme tour), puis exit 70 — DEUX tentatives, jamais trois.
rc=0; run_scenario ok build_fail 9 || rc=$?
check_true "arret en echec persistant : deux tentatives du cleanup puis exit 70, ARRET NON CERTIFIE" \
  bash -c "[ '${rc}' -eq 70 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 2 ] \
    && grep -q 'ARRET NON CERTIFIE' '${SCENARIO_DIR}/stderr.log'"
# ---- 8bis (dixieme tour) : sortie PRE-SCP (trop tard) + premier arret en
# echec TRANSITOIRE — la seconde reserve arrete la cible : deux arrets, le
# second reussi, zero SSH/SCP/validation, recu rc=77 stop_rc=0.
_OLD_FAKE_GEN="${FAKE_GEN}"
FAKE_GEN="$(date -u -d '-28000 seconds' +%Y-%m-%dT%H:%M:%SZ)"
export FAKE_STOP_FAIL_FIRST=1
rc=0; run_scenario ok || rc=$?
unset FAKE_STOP_FAIL_FIRST
FAKE_GEN="${_OLD_FAKE_GEN}"
check_true "sortie pre-SCP + echec transitoire : seconde reserve employee, arret certifie, zero SSH/SCP" \
  bash -c "[ '${rc}' -eq 77 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 2 ] \
    && ! grep -q 'compute ssh' '${FAKE_CALLS}' && ! grep -q 'compute scp' '${FAKE_CALLS}' \
    && [ ! -e '${SCENARIO_DIR}/work/validation.txt' ] \
    && grep -q '^state=targeted_stopped' '${SCENARIO_DIR}/work/etat_cycle_vie' \
    && d=\$(ls -d '${SCENARIO_DIR}'/recu/s_* | head -1) \
    && grep -q '^stop_rc=0' \"\$d/RECU_SESSION.txt\""

# ---- 9. Arret deja certifie par le garde : 0 arret, 0 blocage, propagation.
rc=0; run_scenario stopped_by_guard || rc=$?
check_true "arret deja certifie par le garde : aucun second arret, aucun blocage" \
  bash -c "[ '${rc}' -eq 5 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 0 ] \
    && ! grep -q 'BLOCAGE' '${SCENARIO_DIR}/stderr.log' \
    && grep -q 'arret deja certifie par le garde' '${SCENARIO_DIR}/stdout.log'"

# ---- 9bis. REPRISE BORNEE **EXECUTEE** (cinquieme tour) : le garde tente
# REELLEMENT son arret interne (premier appel de stop_and_verify, en echec),
# publie targeted_stop_failed d'apres ce resultat observe, puis le cleanup
# exterieur RE-tente UNE fois — exactement DEUX appels, meme cible, meme
# generation, dans cet ordre, terminal targeted_stopped.
export FAKE_STOP_FAIL_FIRST=1
rc=0; run_scenario stop_failed_by_guard || rc=$?
unset FAKE_STOP_FAIL_FIRST
check_true "reprise EXECUTEE : deux appels de stop ordonnes (interne au garde puis exterieur), meme generation, terminal targeted_stopped" \
  bash -c "[ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 2 ] \
    && [ \"\$(grep -c -- 'STOP --yes --expected-last-start-timestamp ${FAKE_GEN}' '${FAKE_CALLS}')\" -eq 2 ] \
    && s1=\$(grep -n '^STOP ' '${FAKE_CALLS}' | head -1 | cut -d: -f1) \
    && s2=\$(grep -n '^STOP ' '${FAKE_CALLS}' | tail -1 | cut -d: -f1) \
    && d=\$(grep -n '^START ' '${FAKE_CALLS}' | head -1 | cut -d: -f1) \
    && [ \"\${d}\" -lt \"\${s1}\" ] && [ \"\${s1}\" -lt \"\${s2}\" ] \
    && [ \"\${s2}\" -eq \"\$(wc -l < '${FAKE_CALLS}')\" ] \
    && grep -q '^state=targeted_stopped' '${SCENARIO_DIR}/work/etat_cycle_vie'"

# ---- 9ter. MUTANTS PERMANENTS DU REGISTRE (cinquieme tour) : aucun ne doit
# conclure « avant mutation » ni « deja arrete » sans generation exacte.
# (a) registre PERDU pendant la campagne, handoff valide : arret via handoff.
rc=0; run_scenario ok campagne_supprime_registre || rc=$?
check_true "mutant registre perdu + handoff valide : UN arret cible, jamais refus avant mutation" \
  bash -c "[ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q -- '--expected-last-start-timestamp ${FAKE_GEN}' '${FAKE_CALLS}' \
    && ! grep -q 'refus avant demarrage' '${SCENARIO_DIR}/stdout.log' '${SCENARIO_DIR}/work/session.log' 2>/dev/null \
    && d=\$(ls -d '${SCENARIO_DIR}'/recu/s_* | head -1) \
    && grep -qE '^issue=(arret_tente|arret_certifie_par_le_garde)' \"\$d/RECU_SESSION.txt\""
# (a2) CAUSAL pour la branche « registre ABSENT au cleanup, handoff valide »
# (contre-exemple n° 1 du sixieme tour) : le premier arret EFFACE le registre
# et rend sa republication impossible (dossier fige), puis echoue — le
# cleanup voit un registre absent AVEC generation verrouillee : il DOIT
# re-tenter l'arret (l'ancien code concluait refus avant mutation).
export FAKE_STOP_VANISH_STATE=1 FAKE_STOP_FAIL_FIRST=1
rc=0; run_scenario ok || rc=$?
unset FAKE_STOP_VANISH_STATE FAKE_STOP_FAIL_FIRST
chmod 755 "${SCENARIO_DIR}/work" 2>/dev/null || true
check_true "mutant registre ABSENT au cleanup + handoff valide : le cleanup re-tente (deux arrets), jamais refus avant mutation" \
  bash -c "[ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 2 ] \
    && [ \"\$(grep -c -- '--expected-last-start-timestamp ${FAKE_GEN}' '${FAKE_CALLS}')\" -eq 2 ] \
    && d=\$(ls -d '${SCENARIO_DIR}'/recu/s_* | head -1) \
    && ! grep -q '^issue=refus_avant_mutation' \"\$d/RECU_SESSION.txt\""
# (b) cle dupliquee : snapshot ILLISIBLE -> arret via handoff.
rc=0; run_scenario ok campagne_registre_duplique || rc=$?
check_true "mutant cle dupliquee : registre illisible, UN arret cible via le handoff" \
  bash -c "[ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q -- '--expected-last-start-timestamp ${FAKE_GEN}' '${FAKE_CALLS}'"
# (c) schema manquant : idem.
rc=0; run_scenario ok campagne_registre_sans_schema || rc=$?
check_true "mutant schema manquant : registre illisible, UN arret cible via le handoff" \
  bash -c "[ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q -- '--expected-last-start-timestamp ${FAKE_GEN}' '${FAKE_CALLS}'"
# (d) fichier tronque (derniere ligne non terminee) : idem.
rc=0; run_scenario ok campagne_registre_tronque || rc=$?
check_true "mutant registre tronque : illisible, UN arret cible via le handoff" \
  bash -c "[ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q -- '--expected-last-start-timestamp ${FAKE_GEN}' '${FAKE_CALLS}'"
# (e) targeted_stopped d'une AUTRE generation, handoff valide : fast-path
# REFUSE, arret sur la generation VERROUILLEE (jamais celle du registre).
rc=0; run_scenario stopped_wrong_generation || rc=$?
check_true "mutant targeted_stopped d'une autre generation : fast-path refuse, arret sur la generation verrouillee" \
  bash -c "[ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q -- '--expected-last-start-timestamp ${FAKE_GEN}' '${FAKE_CALLS}' \
    && ! grep -q -- '--expected-last-start-timestamp generation-perimee' '${FAKE_CALLS}' \
    && ! grep -q 'arret deja certifie par le garde' '${SCENARIO_DIR}/stdout.log'"
# (f) PREMIERE PUBLICATION INTERROMPUE : seul un temporaire .partial existe,
# aucun registre final, aucun handoff — la publication atomique garantit
# qu'aucun start n'a ete demande : refus avant mutation, 0 arret, 0 blocage.
rc=0; run_scenario publication_interrompue || rc=$?
check_true "mutant publication interrompue : refus avant mutation, aucun arret, aucun blocage" \
  bash -c "[ '${rc}' -eq 3 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 0 ] \
    && ! grep -q 'BLOCAGE' '${SCENARIO_DIR}/stderr.log' \
    && d=\$(ls -d '${SCENARIO_DIR}'/recu/s_* | head -1) \
    && grep -q '^issue=refus_avant_mutation' \"\$d/RECU_SESSION.txt\""

# (g) P0 sixieme tour : registre PRESENT mais ILLISIBLE (permission), sans
# handoff ni generation en memoire — une erreur de lecture ne prouve pas
# l'absence : BLOCAGE 71 exige, jamais « refus avant mutation », zero arret.
rc=0; run_scenario registre_illisible || rc=$?
chmod 600 "${SCENARIO_DIR}/work/etat_cycle_vie" 2>/dev/null || true
check_true "mutant registre illisible sans handoff : BLOCAGE 71, jamais refus avant mutation, aucun arret" \
  bash -c "[ '${rc}' -eq 71 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 0 ] \
    && grep -q 'registre de cycle de vie ILLISIBLE' '${SCENARIO_DIR}/stderr.log' \
    && ! grep -q 'refus avant demarrage' '${SCENARIO_DIR}/stdout.log'"

# (h) P0 septieme tour : surcharge temporelle hors bornes refusee AVANT
# toute garde (SSH_STEP_TIMEOUT_S=0 desactivait GNU timeout).
export SSH_STEP_TIMEOUT_S=0
rc=0; run_scenario ok || rc=$?
unset SSH_STEP_TIMEOUT_S
check_true "surcharge SSH_STEP_TIMEOUT_S=0 : refus rc=2 avant toute garde" \
  bash -c "[ '${rc}' -eq 2 ] && ! grep -qE '^(SETMAX|START|STOP) ' '${FAKE_CALLS}'"
# (i) trop tard, ne pas lancer : generation dans le PASSE — le build refuse
# (temps restant insuffisant, jamais remonte), UN arret cible, recu grave.
_OLD_FAKE_GEN="${FAKE_GEN}"
FAKE_GEN="$(date -u -d '-28000 seconds' +%Y-%m-%dT%H:%M:%SZ)"
rc=0; run_scenario ok || rc=$?
FAKE_GEN="${_OLD_FAKE_GEN}"
check_true "trop tard (generation passee) : refus rc=77 AVANT toute operation initiale (zero SSH/SCP), UN arret cible" \
  bash -c "[ '${rc}' -eq 77 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && ! grep -q 'compute ssh' '${FAKE_CALLS}' && ! grep -q 'compute scp' '${FAKE_CALLS}'"
# (j) describe BLOQUE : borne par DESCRIBE_TIMEOUT_S — la session echoue
# proprement et l'arret cible est atteint (jamais un blocage infini).
export FAKE_DESCRIBE_HANG=1 DESCRIBE_TIMEOUT_S=10
rc=0; run_scenario ok || rc=$?
unset FAKE_DESCRIBE_HANG DESCRIBE_TIMEOUT_S
check_true "describe bloque : borne, session en echec propre, UN arret cible" \
  bash -c "[ '${rc}' -ne 0 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ]"
# (k) P1 septieme tour : registre ETRANGER strict apres l'arret (publication
# du terminal empechee) — INCOHERENCE DE PREUVE gravee sous sa propre issue,
# exit 78, jamais le message « recu non publie ».
export FAKE_STOP_FOREIGN_STATE=1
rc=0; run_scenario ok || rc=$?
unset FAKE_STOP_FOREIGN_STATE
chmod 755 "${SCENARIO_DIR}/work" 2>/dev/null || true
check_true "registre etranger post-arret : exit 78, issue=incoherence_registre_post_arret, re-tentative puis preuve refusee" \
  bash -c "[ '${rc}' -eq 78 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 2 ] \
    && d=\$(ls -d '${SCENARIO_DIR}'/recu/s_* | head -1) \
    && grep -q '^issue=incoherence_registre_post_arret' \"\$d/RECU_SESSION.txt\" \
    && ! grep -q 'RECU NON PUBLIE' '${SCENARIO_DIR}/stderr.log'"

# (l) grace protocolaire FIXE : 29 et 31 refuses avant toute garde.
for g in 29 31; do
  export GRACE_S="${g}"
  rc=0; run_scenario ok || rc=$?
  unset GRACE_S
  check_true "grace protocolaire : GRACE_S=${g} refuse avant toute garde" \
    bash -c "[ '${rc}' -eq 2 ] && ! grep -qE '^(SETMAX|START|STOP) ' '${FAKE_CALLS}'"
done
# (m) relation invite/GCE testee AVANT set_max : zero SETMAX.
export GUEST_SHUTDOWN_MINUTES=480
rc=0; run_scenario ok ok 0 3600 || rc=$?
unset GUEST_SHUTDOWN_MINUTES
check_true "relation invite/GCE violee : refus rc=2 sans AUCUN SETMAX" \
  bash -c "[ '${rc}' -eq 2 ] && ! grep -qE '^(SETMAX|START|STOP) ' '${FAKE_CALLS}' \
    && grep -q 'relation invite/GCE violee' '${SCENARIO_DIR}/stderr.log'"
# (m2/m3) FRONTIERES du budget d'armement (§ 5.16/5.18.1, premier depart
# brule) : MAX_RUN=28800 => GUEST*60 + 300 (reserve GCE) + 120 (tolerance
# systemd) + 480 (budget) <= 28800 <=> GUEST <= 465 min. 465 accepte (SETMAX
# atteint, relation muette), 466 refuse avant toute garde ; puis A LA
# SECONDE : 465 min sous 28800 s = 600 s avant tolerance (480 apres)
# accepte, sous 28799 s = 599 s (479 apres) refuse.
export GUEST_SHUTDOWN_MINUTES=465
rc=0; run_scenario ok ok 0 28800 || rc=$?
unset GUEST_SHUTDOWN_MINUTES
check_true "frontiere budget d'armement : 465 min sous 28800 s ACCEPTE (600 s avant tolerance = 480 apres ; SETMAX atteint)" \
  bash -c "grep -qE '^SETMAX ' '${FAKE_CALLS}' && ! grep -q 'relation invite/GCE violee' '${SCENARIO_DIR}/stderr.log'"
export GUEST_SHUTDOWN_MINUTES=465
rc=0; run_scenario ok ok 0 28799 || rc=$?
unset GUEST_SHUTDOWN_MINUTES
check_true "frontiere budget d'armement A LA SECONDE : 465 min sous 28799 s REFUSE (599 s avant tolerance = 479 apres), zero SETMAX" \
  bash -c "[ '${rc}' -eq 2 ] && ! grep -qE '^(SETMAX|START|STOP) ' '${FAKE_CALLS}' \
    && grep -q 'budget d.armement' '${SCENARIO_DIR}/stderr.log'"
export GUEST_SHUTDOWN_MINUTES=466
rc=0; run_scenario ok ok 0 28800 || rc=$?
unset GUEST_SHUTDOWN_MINUTES
check_true "frontiere budget d'armement : 466 min sous 28800 s REFUSE (budget < 480 s), zero SETMAX" \
  bash -c "[ '${rc}' -eq 2 ] && ! grep -qE '^(SETMAX|START|STOP) ' '${FAKE_CALLS}' \
    && grep -q 'budget d.armement' '${SCENARIO_DIR}/stderr.log'"
# (n) premier arret post-scp en echec : re-tentative IMMEDIATE avant toute
# validation — STOP1 < STOP2 < VALIDATE, terminal targeted_stopped, le
# cleanup ne cree pas de troisieme tentative.
export FAKE_STOP_FAIL_FIRST=1 FAKE_VALIDATOR_STUB=1
rc=0; run_scenario ok || rc=$?
unset FAKE_STOP_FAIL_FIRST FAKE_VALIDATOR_STUB
check_true "reprise post-scp : ordre STOP1 < STOP2 < VALIDATE prouve au ledger, jamais une troisieme tentative" \
  bash -c "[ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 2 ] \
    && [ \"\$(grep -c '^VALIDATE' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && s1=\$(grep -n '^STOP ' '${FAKE_CALLS}' | head -1 | cut -d: -f1) \
    && s2=\$(grep -n '^STOP ' '${FAKE_CALLS}' | tail -1 | cut -d: -f1) \
    && v1=\$(grep -n '^VALIDATE' '${FAKE_CALLS}' | head -1 | cut -d: -f1) \
    && [ \"\${s1}\" -lt \"\${s2}\" ] && [ \"\${s2}\" -lt \"\${v1}\" ] \
    && grep -q 're-tentative IMMEDIATE avant toute validation' '${SCENARIO_DIR}/work/session.log' \
    && grep -q '^state=targeted_stopped' '${SCENARIO_DIR}/work/etat_cycle_vie'"
# (o) DEUX arrets en echec : validation SAUTEE, aucune conclusion de
# campagne, exit 70, deux tentatives et jamais une troisieme.
rc=0; run_scenario ok ok 9 || rc=$?
check_true "double echec d'arret : validation sautee, aucune conclusion, exit 70, deux tentatives seulement" \
  bash -c "[ '${rc}' -eq 70 ] && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 2 ] \
    && [ ! -e '${SCENARIO_DIR}/work/validation.txt' ] \
    && ! grep -q 'campaign_status' '${SCENARIO_DIR}/stdout.log' \
    && grep -q 'validation SAUTEE' '${SCENARIO_DIR}/work/session.log'"

# (p) contre-calendrier du neuvieme tour : profil smoke_v1, MAX=5400,
# invite 75 min (relation exacte 75*60+300+120+480=5400 depuis le budget
# d'armement du § 5.18.1 ; le cutoff effectif reste 4500 s = l'invite, donc
# l'echeance du runner — gen + cutoff - POST - 90 — est inchangee), DESCRIBE=600 — le build
# consomme 90 s reelles, l'echeance tombe pendant : le describe pre-campagne
# NE DOIT PAS s'executer (clamp), la campagne ne part pas, le scp reste
# admis par la garde a DEUX arrets et l'arret est certifie.
export CAMPAIGN_PROFILE=smoke_v1 GUEST_SHUTDOWN_MINUTES=75 DESCRIBE_TIMEOUT_S=600 \
  SCP_STEP_TIMEOUT_S=60 SSH_STEP_TIMEOUT_S=7200
_OLD_FAKE_GEN="${FAKE_GEN}"
FAKE_GEN="$(date -u -d '-595 seconds' +%Y-%m-%dT%H:%M:%SZ)"
export FAKE_BUILD_SLEEP_S=90
rc=0; run_scenario ok build_lent 0 5400 || rc=$?
FAKE_GEN="${_OLD_FAKE_GEN}"
unset CAMPAIGN_PROFILE GUEST_SHUTDOWN_MINUTES DESCRIBE_TIMEOUT_S SCP_STEP_TIMEOUT_S SSH_STEP_TIMEOUT_S FAKE_BUILD_SLEEP_S
check_true "contre-calendrier : rc=77 conserve, describe pre-campagne clampe, campagne non lancee, scp budgete a deux arrets, arret certifie" \
  bash -c "[ '${rc}' -eq 77 ] && grep -q 'avant le controle pre-campagne' '${SCENARIO_DIR}/work/session.log' \
    && [ \"\$(grep -c 'compute ssh' '${FAKE_CALLS}')\" -eq 2 ] \
    && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 1 ] \
    && grep -q '^state=targeted_stopped' '${SCENARIO_DIR}/work/etat_cycle_vie'"
# La garde scp compte LITTERALEMENT deux reserves d'arret et le backoff
# (assertion textuelle sur le script epingle — le chemin dynamique « scp
# refuse » est inatteignable par construction : cutoff - deadline =
# POST_BUDGET + 90 des la derivation).
check_true "garde scp CAUSALE a la frontiere d'une seconde : pire cas 3155 s du contre-calendrier, refus a -1 s, coefficient 2 discriminant" \
  bash -c "
    src=\$(sed -n '/^scp_worst_case_s() {/,/^}/p' '${HERE}/v6_session_lifecycle.sh')
    [ -n \"\$src\" ] || exit 1
    SCP_STEP_TIMEOUT_S=60 GRACE_S=30 DESCRIBE_TIMEOUT_S=600 STOP_RESERVE_S=900
    eval \"\$src\"
    w=\$(scp_worst_case_s 1)
    [ \"\$w\" -eq 3155 ] || exit 1
    cutoff=1003155
    # admis a la frontiere exacte, refuse une seconde trop tard
    [ \$(( 1000000 + w )) -le \"\$cutoff\" ] || exit 1
    [ \$(( 1000001 + w )) -gt \"\$cutoff\" ] || exit 1
    # coefficient 1 (l'ancien defaut) aurait encore admis a +900 s : bande causale
    w1=\$(( w - 900 ))
    [ \$(( 1000900 + w1 )) -le \"\$cutoff\" ] || exit 1"

# ---- Scenarios bootstrap + pin : clone jetable, versions COURANTES du
# protocole synchronisees et committees DANS LE CLONE (commit conditionnel :
# sur un HEAD deja identique, rien a committer n'est PAS un echec — defaut
# du premier tour corrige).
CLONE="${BASE}/clone"
git clone --quiet --shared --no-hardlinks "${ROOT}" "${CLONE}" 2>/dev/null
mkdir -p "${CLONE}/gcp-migration/profils" "${CLONE}/morsehgp3D_v6/tests"
for f in "${PROTOCOL_FILES[@]}"; do
  cp "${ROOT}/${f}" "${CLONE}/${f}"
done
(
  cd "${CLONE}"
  git -c user.name=selftest -c user.email=selftest@local add -- "${PROTOCOL_FILES[@]}" >/dev/null
  git diff --cached --quiet || \
    git -c user.name=selftest -c user.email=selftest@local commit --quiet -m "selftest : protocole courant"
)
CLONE_COMMIT="$(cd "${CLONE}" && git rev-parse HEAD)"
rc=0
( cd "${CLONE}" && ./gcp-migration/v6_campaign_pin.sh "$(mktemp -d "${BASE}/pin.XXXXXX")" "${CLONE_COMMIT}" ) \
  >/dev/null 2>&1 || rc=$?
check_true "pin du clone propre : accepte (prealable des refus)" [ "${rc}" -eq 0 ]

# 11. Pin altere qui neutraliserait son controle + garde alteree : l'etage 1
# du bootstrap materialise le pin DU COMMIT — le pin altere n'est jamais
# execute, et le pin honnete refuse la garde alteree.
sed -i 's/^git diff --quiet/: git diff --quiet/' "${CLONE}/gcp-migration/v6_campaign_pin.sh"
echo "# garde alteree" >> "${CLONE}/gcp-migration/start_and_verify.sh"
rc=0
( cd "${CLONE}" && bash gcp-migration/session_campagne_v6_g4.sh ) >/dev/null 2>"${BASE}/boot11.err" || rc=$?
check_true "pin altere neutralisant + garde alteree : refus par le pin DU COMMIT (rc=2)" \
  bash -c "[ '${rc}' -eq 2 ] && grep -q 'REFUS' '${BASE}/boot11.err'"
( cd "${CLONE}" && git checkout --quiet -- gcp-migration/v6_campaign_pin.sh gcp-migration/start_and_verify.sh )

# 12. Bootstrap altere dans le worktree : l'etage 1 refuse (identite contre
# la copie materialisee du commit).
echo "# bootstrap altere" >> "${CLONE}/gcp-migration/session_campagne_v6_g4.sh"
rc=0
( cd "${CLONE}" && bash gcp-migration/session_campagne_v6_g4.sh ) >/dev/null 2>"${BASE}/boot12.err" || rc=$?
check_true "bootstrap altere : refus d'identite de l'etage 1 (rc=2)" \
  bash -c "[ '${rc}' -eq 2 ] && grep -q 'differe de la version du commit' '${BASE}/boot12.err'"
( cd "${CLONE}" && git checkout --quiet -- gcp-migration/session_campagne_v6_g4.sh )

# 12bis. POINT D'ENTREE DE CONFIANCE MAXIMAL execute REELLEMENT hors du
# depot (troisieme tour : la commande documentee doit fonctionner) : le
# bootstrap est materialise depuis le commit du clone vers /tmp et lance
# depuis / avec la racine EXPLICITE ; PATH empoisonne par le faux gcloud
# (aucun contact GCP possible) — la chaine doit atteindre l'etage 2
# re-authentifie et le pin (source_commit imprime), puis echouer PLUS LOIN
# que le bootstrap (le vrai garde refuse le faux gcloud), sans aucun arret.
BOOT="$(mktemp /tmp/ehgp-boot-selftest.XXXXXX.sh)"
git -C "${CLONE}" show "${CLONE_COMMIT}:gcp-migration/session_campagne_v6_g4.sh" > "${BOOT}"
ENTRYBIN="$(mktemp -d "${BASE}/entrybin.XXXXXX")"
make_fake_bin "${ENTRYBIN}"
export FAKE_CALLS="${BASE}/entry_calls.log"
: > "${FAKE_CALLS}"
rc=0
( cd / && env PATH="${ENTRYBIN}:${PATH}" FAKE_GEN="${FAKE_GEN}" FAKE_CALLS="${FAKE_CALLS}" \
    MHGP6_BOOTSTRAP_REPO_ROOT="${CLONE}" MHGP6_BOOTSTRAP_COMMIT="${CLONE_COMMIT}" \
    bash "${BOOT}" ) > "${BASE}/entry.log" 2>&1 || rc=$?
check_true "point d'entree /tmp hors depot : etage 2 re-authentifie et pin atteints, echec sur (aucun arret)" \
  bash -c "[ '${rc}' -ne 0 ] && grep -q 'etage 2 : bootstrap et pin re-authentifies' '${BASE}/entry.log' \
    && grep -q '^source_commit=${CLONE_COMMIT}' '${BASE}/entry.log' \
    && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}' || true)\" -eq 0 ]"
rm -f "${BOOT}"

# 12ter. ENTREE DIRECTE en etage 2 (marqueur forge) : refusee.
FORGE="$(mktemp -d "${BASE}/forge.XXXXXX")"
rc=0
env MHGP6_BOOTSTRAP_STAGE2=1 MHGP6_BOOTSTRAP_WORK="${FORGE}" \
  MHGP6_BOOTSTRAP_COMMIT="${CLONE_COMMIT}" MHGP6_BOOTSTRAP_REPO_ROOT="${CLONE}" \
  bash "${CLONE}/gcp-migration/session_campagne_v6_g4.sh" > /dev/null 2>"${BASE}/forge.err" || rc=$?
check_true "entree directe en etage 2 : refusee (marqueur forge)" \
  bash -c "[ '${rc}' -eq 2 ] && grep -q 'entree directe en etage 2' '${BASE}/forge.err'"

# 13. Chaque fichier du protocole altere => le pin refuse (code 2).
for f in "${PROTOCOL_FILES[@]}"; do
  echo "# alteration" >> "${CLONE}/${f}"
  rc=0
  ( cd "${CLONE}" && ./gcp-migration/v6_campaign_pin.sh "$(mktemp -d "${BASE}/pin.XXXXXX")" "${CLONE_COMMIT}" ) \
    >/dev/null 2>&1 || rc=$?
  check_true "pin refuse le fichier altere : ${f}" [ "${rc}" -eq 2 ]
  ( cd "${CLONE}" && git checkout --quiet -- "${f}" )
done

if [ "${FAILURES}" -ne 0 ]; then
  echo "selftest cycle de vie v6 : ${FAILURES} echec(s)" >&2
  exit 1
fi
echo "selftest cycle de vie v6 : arret cible ou blocage prouve sur chaque sortie apres demarrage (35 scenarios dont reprise EXECUTEE a deux appels ordonnes et six mutants permanents du registre — perdu, duplique, sans schema, tronque, targeted_stopped d'une autre generation, publication interrompue, registre illisible=blocage, surcharge temporelle refusee, trop-tard sans remontee, describe borne, registre etranger post-arret=78, grace fixe 29/31 refusees, relation invite/GCE avant set_max, STOP1<STOP2<VALIDATE, double echec sans validation, contre-calendrier a describe clampe, ordre STOP1<STOP2<VALIDATE au ledger, sortie pre-SCP a seconde reserve, garde scp causale a la seconde — + 13 refus de pin sur l inventaire repo-relatif a deux repertoires, rejouable depuis un HEAD propre)"
