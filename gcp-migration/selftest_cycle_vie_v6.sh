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
# EHGP_SELFTEST_KEEP=1 : conserver BASE pour diagnostic (jamais en CI).
trap '[ "${EHGP_SELFTEST_KEEP:-0}" = "1" ] || rm -rf "${BASE}"' EXIT

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
# deux listes normatives (pin + lifecycle) — dix-sept fichiers dans DEUX
# repertoires (le juge du pilote vit sous morsehgp3D_v6/tests/). Les profils
# ajoutes apres coup sont mis en FIN de liste : l'ordre est normatif (le
# manifeste canonique en depend), deplacer une entree existante changerait
# le digest de protocole de tous les inventaires a la fois.
PROTOCOL_FILES=(gcp-migration/session_campagne_v6_g4.sh gcp-migration/v6_session_lifecycle.sh
                gcp-migration/v6_campaign_pin.sh gcp-migration/v6_campaign_remote.sh
                gcp-migration/validate_v6_campaign.py gcp-migration/profils/decision_v1.env
                gcp-migration/profils/smoke_v1.env gcp-migration/profils/g4_mesure_v1.env
                gcp-migration/profils/g4_serie_c_v1.env gcp-migration/profils/g4_tests_v1.env
                gcp-migration/profils/g4_tests_v2.env
                morsehgp3D_v6/tests/pilote_juge.py
                gcp-migration/set_max_run_duration_and_verify.sh
                gcp-migration/start_and_verify.sh gcp-migration/stop_and_verify.sh
                gcp-migration/recover_v6_session.sh
                gcp-migration/profils/g4_echelle_v1.env)

# ---- Faux gcloud (PATH) : describe rend la generation ; ssh n1 = handshake
# boot_id seul, n2 = build (portes), n3 = campagne ; scp materialise out/.
make_fake_bin() { # $1 = dossier
  # Faux tee : echoue (FAKE_TEE_FAIL_AFTER_SCP=1) apres une scp de rapatriement
  # locale => erreur LOCALE pre-STOP dans la reprise (dent du funnel d'arret).
  cat > "$1/tee" <<'EOF'
#!/usr/bin/env bash
# Echoue UNE fois (premier tee apres la scp locale) : l'erreur locale est
# injectee, puis le journal de reprise redevient lisible pour le diagnostic.
if [ "${FAKE_TEE_FAIL_AFTER_SCP:-0}" = "1" ] && [ -e "${FAKE_CALLS}.scp_local" ] && [ ! -e "${FAKE_CALLS}.tee_failed" ]; then : > "${FAKE_CALLS}.tee_failed"; cat >/dev/null; exit 1; fi
# FAKE_TEE_FAIL_ON=<motif> : echoue UNE fois sur la premiere entree contenant le motif.
if [ -n "${FAKE_TEE_FAIL_ON:-}" ] && [ ! -e "${FAKE_CALLS}.tee_failed" ]; then
  buf="$(cat)"
  case "${buf}" in *"${FAKE_TEE_FAIL_ON}"*) : > "${FAKE_CALLS}.tee_failed"; exit 1 ;; esac
  printf '%s\n' "${buf}" | exec /usr/bin/tee "$@"
fi
exec /usr/bin/tee "$@"
EOF
  chmod +x "$1/tee"
  # Faux python3 : FAKE_PY_STATE_FAIL=1 => echoue sur le script de publication du
  # registre (lifecycle-state) SEULEMENT ; tout autre script passe au vrai python3.
  cat > "$1/python3" <<'EOF'
#!/usr/bin/env bash
REAL="$(command -v -p python3 2>/dev/null || echo /usr/bin/python3)"
if [ "${FAKE_PY_STATE_FAIL:-0}" = "1" ] && [ "${1:-}" = "-" ]; then
  script="$(cat)"
  case "${script}" in *"lifecycle-state.v1"*"state="*) echo "faux python3 : publication du registre en echec" >&2; exit 1 ;; esac
  printf '%s\n' "${script}" | exec "${REAL}" "$@"
fi
exec "${REAL}" "$@"
EOF
  chmod +x "$1/python3"
  cat > "$1/gcloud" <<'EOF'
#!/usr/bin/env bash
echo "GCLOUD $*" >> "${FAKE_CALLS}"
echo "CLOUDSDK ${CLOUDSDK_CONFIG:-aucun}" >> "${FAKE_CALLS}"
case "$*" in
  *"instances describe"*)
    if [ "${FAKE_DESCRIBE_HANG:-0}" = "1" ]; then sleep 3600; fi
    # Generation observee : FAKE_DESCRIBE_GEN (concurrente des le depart) ou
    # FAKE_DESCRIBE_GEN_AFTER_SCP (change pendant le rapatriement) ; statut
    # TERMINATED des qu'un `instances stop` a ete journalise (la VRAIE garde
    # d'arret epinglee, exercee par la reprise, relit ces champs un a un).
    gen="${FAKE_GEN}"
    [ -z "${FAKE_DESCRIBE_GEN:-}" ] || gen="${FAKE_DESCRIBE_GEN}"
    # « apres la scp » = apres une scp de RAPATRIEMENT local (marqueur pose par
    # le faux scp quand la destination est un chemin local), jamais la scp
    # d'envoi du bundle du superviseur.
    if [ -n "${FAKE_DESCRIBE_GEN_AFTER_SCP:-}" ] && [ -e "${FAKE_CALLS}.scp_local" ]; then gen="${FAKE_DESCRIBE_GEN_AFTER_SCP}"; fi
    # describe en echec APRES la scp locale : seulement la lecture en TUPLE de
    # la reprise (la vraie garde d'arret relit ses champs un a un et doit
    # pouvoir certifier l'arret).
    case "$*" in *"value(status,lastStartTimestamp)"*)
      if [ "${FAKE_DESCRIBE_FAIL_AFTER_SCP:-0}" = "1" ] && [ -e "${FAKE_CALLS}.scp_local" ]; then echo "describe indisponible (faux)" >&2; exit 1; fi ;;
    esac
    # TERMINATED seulement apres un `instances stop` REUSSI (marqueur), jamais
    # apres une tentative en echec — sinon la vraie garde conclurait « deja
    # arretee » sans arret lors d'un rejeu stop-first.
    status=RUNNING
    if [ -e "${FAKE_CALLS}.stopped" ]; then status=TERMINATED; fi
    case "$*" in
      *"value(status,lastStartTimestamp)"*) printf '%s\t%s\n' "${status}" "${gen}"; exit 0 ;;
      *"value(status)"*) echo "${status}"; exit 0 ;;
      *"value(labels.project)"*) echo "e-hgp"; exit 0 ;;
      *"value(lastStartTimestamp)"*) echo "${gen}"; exit 0 ;;
    esac
    echo "${gen}"; exit 0 ;;
  *"config get-value project"*) echo "${GCP_PROJECT_ID}"; exit 0 ;;
  *"instances list"*)
    case "$*" in
      *"labels.project=e-hgp"*)
        st=RUNNING; [ -e "${FAKE_CALLS}.stopped" ] && st=TERMINATED
        echo "${GCP_INSTANCE_NAME},${GCP_ZONE},${st}"; exit 0 ;;
      *) echo "${GCP_INSTANCE_NAME}"; exit 0 ;;
    esac ;;
  *"instances stop"*)
    if [ "${FAKE_GCLOUD_STOP_RC:-0}" = "0" ]; then : > "${FAKE_CALLS}.stopped"; exit 0; fi
    exit "${FAKE_GCLOUD_STOP_RC}" ;;
  *"compute ssh"*)
    n=$(grep -c "compute ssh" "${FAKE_CALLS}" || true)
    if [ "${n}" -le 1 ]; then
      echo "aaaabbbb-cccc-dddd-eeee-ffff00001111"
      exit 0
    fi
    if [ "${n}" -eq 2 ]; then
      if [ "${FAKE_SSH_MODE:-ok}" = "build_fail" ]; then echo "build casse" >&2; exit 1; fi
      if [ "${FAKE_SSH_MODE:-ok}" = "build_lent" ]; then sleep "${FAKE_BUILD_SLEEP_S:-90}"; fi
      # build_bloque (§ 5.18.6) : le build ne rend jamais la main — le
      # selftest tue toute la session du superviseur pendant ce blocage.
      if [ "${FAKE_SSH_MODE:-ok}" = "build_bloque" ]; then echo "$$" > "${FAKE_SSH_BLOQUE_PID:-/dev/null}"; sleep 3600; exit 1; fi
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
    # Destination HONOREE quand le dernier argument est un chemin local (la
    # reprise rapatrie dans un staging) ; sinon la destination historique.
    dest="${FAKE_SCP_DEST}"
    for last; do :; done
    case "${last}" in /*) dest="${last%/}"; : > "${FAKE_CALLS}.scp_local" ;; esac
    case "${FAKE_SCP_MODE:-ok}" in
      echec) exit 1 ;;
      partiel) mkdir -p "${dest}/out"; echo "partiel" > "${dest}/out/bench_resume.txt"; printf 'leurre  x\n' > "${dest}/out/SHA256SUMS"; exit 0 ;;
    esac
    mkdir -p "${dest}/out"
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
handoff=""; state=""; marks=""
while [ $# -gt 0 ]; do
  case "$1" in
    --handoff-file) handoff="$2"; shift 2 ;;
    --lifecycle-state-file) state="$2"; shift 2 ;;
    --guard-mark-dir) marks="$2"; shift 2 ;;
    *) shift ;;
  esac
done
write_mark() { # $1 = nom, $2 = generation (marque O_EXCL : jamais reecrite)
  [ -n "${marks}" ] || return 0
  [ ! -e "${marks}/$1" ] || return 1
  printf 'schema=e-hgp.guard-mark.v1\nmark=%s\nproject=%s\nzone=%s\ninstance=%s\ngeneration=%s\nmax_run_seconds=%s\nguest_shutdown_minutes=%s\ndate_utc=%s\n' \
    "$1" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}" "$2" "${FAKE_MAX_RUN:-28800}" "470" "$(date -u +%Y-%m-%dT%H:%M:%SZ)" > "${marks}/$1"
}
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
  ok_bloque)
    # § 5.18.6 : la premiere marque est publiee puis le garde ne rend jamais
    # la main (armement invite « en cours ») — la seconde marque n'existe
    # pas quand le superviseur est tue.
    write_handoff "${FAKE_GEN}"; write_state targeted_running "${FAKE_GEN}"; write_mark guest_guard_pending "${FAKE_GEN}"
    echo "$$" > "${FAKE_START_BLOQUE_PID:-/dev/null}"; sleep 3600; exit 1 ;;
  ok) write_handoff "${FAKE_GEN}"; write_state targeted_running "${FAKE_GEN}"; write_mark guest_guard_pending "${FAKE_GEN}"; write_mark double_guard_verified "${FAKE_GEN}"; exit 0 ;;
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
SCEN_W=""; SCEN_COMMIT=""; SCEN_PAYLOAD=""; SCEN_MANIFEST=""
# prepare_scenario : tout sauf le lancement. PIN_SOURCE=clone => les copies
# epinglees viennent de `git show CLONE_COMMIT:` (commit REEL des fichiers
# courants) : indispensable a la reprise, qui se re-authentifie contre le
# commit ; sinon copies du worktree et commit factice.
prepare_scenario() {
  local start_mode="$1" ssh_mode="${2:-ok}" stop_rc="${3:-0}" max_run="${4:-28800}"
  SCENARIO_DIR="$(mktemp -d "${BASE}/scenario.XXXXXX")"
  local W="${SCENARIO_DIR}/work"
  mkdir -p "${W}/pinned/gcp-migration/profils" "${W}/pinned/morsehgp3D_v6/tests" \
           "${SCENARIO_DIR}/bin" "${SCENARIO_DIR}/guards" "${SCENARIO_DIR}/recu"
  chmod 700 "${W}"  # § 5.18.6 : WORK 0700 exige par le cycle de vie
  make_fake_bin "${SCENARIO_DIR}/bin"
  make_fake_guards "${SCENARIO_DIR}/guards"
  for f in "${PROTOCOL_FILES[@]}"; do
    if [ "${PIN_SOURCE:-worktree}" = "clone" ]; then
      git -C "${CLONE}" show "${CLONE_COMMIT}:${f}" > "${W}/pinned/${f}"; chmod +x "${W}/pinned/${f}"
    else
      cp "${ROOT}/${f}" "${W}/pinned/${f}"
    fi
  done
  if [ "${FAKE_VALIDATOR_STUB:-0}" = "1" ]; then
    # Substitue AVANT le calcul du manifeste (la copie epinglee reste donc
    # coherente) : le stub journalise VALIDATE dans le meme ledger que STOP.
    printf '#!/usr/bin/env python3\nimport os\nwith open(os.environ["FAKE_CALLS"], "a") as fh:\n    fh.write("VALIDATE\\n")\nprint("campaign_status=stub")\nraise SystemExit(1)\n' \
      > "${W}/pinned/gcp-migration/validate_v6_campaign.py"
  fi
  echo "bundle factice" > "${W}/bundle.tgz"
  local payload_sha commit="0000000000000000000000000000000000000000"
  [ "${PIN_SOURCE:-worktree}" != "clone" ] || commit="${CLONE_COMMIT}"
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
  export FAKE_SSH_BLOQUE_PID="${SCENARIO_DIR}/ssh_bloque.pid" FAKE_START_BLOQUE_PID="${SCENARIO_DIR}/start_bloque.pid"
  export FAKE_MAX_RUN="${max_run}"
  SCEN_W="${W}"; SCEN_COMMIT="${commit}"; SCEN_PAYLOAD="${payload_sha}"; SCEN_MANIFEST="${manifest_sha}"
  SCEN_START_MODE="${start_mode}"; SCEN_SSH_MODE="${ssh_mode}"; SCEN_STOP_RC="${stop_rc}"; SCEN_MAX_RUN="${max_run}"
}
launch_scenario_env() { # imprime la commande env du cycle de vie (partagee premier plan / arriere-plan)
  local lifecycle="${LIFECYCLE}"
  [ "${PIN_SOURCE:-worktree}" != "clone" ] || lifecycle="${SCEN_W}/pinned/gcp-migration/v6_session_lifecycle.sh"
  env PATH="${SCENARIO_DIR}/bin:${PATH}" \
    FAKE_START_MODE="${SCEN_START_MODE}" FAKE_SSH_MODE="${SCEN_SSH_MODE}" FAKE_STOP_RC="${SCEN_STOP_RC}" \
    MAX_RUN_SECONDS="${SCEN_MAX_RUN}" \
    DURABLE_RECEIPT_BASE="${SCENARIO_DIR}/recu" DURABLE_RECEIPT_PREFIX="s" \
    MHGP6_LIFECYCLE_WORK="${SCEN_W}" \
    MHGP6_LIFECYCLE_GUARDS_DIR="${SCENARIO_DIR}/guards" \
    MHGP6_LIFECYCLE_SOURCE_COMMIT="${SCEN_COMMIT}" \
    MHGP6_LIFECYCLE_PAYLOAD_SHA256="${SCEN_PAYLOAD}" \
    MHGP6_LIFECYCLE_MANIFEST_SHA256="${SCEN_MANIFEST}" \
    MHGP6_BOOTSTRAP_REPO_ROOT="${CLONE:-${ROOT}}" \
    GCP_PROJECT_ID=projet-factice GCP_ZONE=zone-factice GCP_INSTANCE_NAME=instance-factice \
    setsid -w bash "${lifecycle}" "$@"
}
run_scenario() {
  prepare_scenario "$@"
  local rc=0
  launch_scenario_env > "${SCENARIO_DIR}/stdout.log" 2> "${SCENARIO_DIR}/stderr.log" || rc=$?
  return "${rc}"
}
# run_scenario_bg : lancement en session de processus PROPRE (setsid) ; le
# superviseur se grave lui-meme dans superviseur.pid — la session entiere
# (garde, timeout, faux gcloud endormis) est tuee par `pkill -s <pid>`.
SUP_PID=""
run_scenario_bg() {
  prepare_scenario "$@"
  launch_scenario_env > "${SCENARIO_DIR}/stdout.log" 2> "${SCENARIO_DIR}/stderr.log" &
  SUP_WAIT_PID=$!
}
wait_for_line() { # $1 = fichier, $2 = motif, $3 = secondes
  local i=0
  while [ "${i}" -lt "$3" ]; do
    [ -f "$1" ] && grep -q "$2" "$1" && return 0
    sleep 1; i=$((i + 1))
  done
  return 1
}
kill_session() { # tue toute la session de processus du superviseur (pid du fichier)
  local sid
  sid="$(awk '{print $1}' "${SCEN_W}/superviseur.pid" 2>/dev/null || true)"
  [ -n "${sid}" ] || return 1
  pkill -9 -s "${sid}" 2>/dev/null || true
  wait "${SUP_WAIT_PID}" 2>/dev/null || true
  for f in "${SCENARIO_DIR}/ssh_bloque.pid" "${SCENARIO_DIR}/start_bloque.pid"; do
    [ -f "${f}" ] && kill -9 "$(cat "${f}")" 2>/dev/null || true
  done
  sleep 1
  # Mort du conteneur simulee : tout processus referencant encore WORK
  # (hors cette commande) est tue, et liste avant s'il en restait.
  local surv
  surv="$(pgrep -f -- "${SCEN_W}/[p]inned|MHGP6_LIFECYCLE_WORK=${SCEN_W%?}[${SCEN_W: -1}]|[-]-ssh-key-file=${SCEN_W}" 2>/dev/null | grep -vx "$$" || true)"
  if [ -n "${surv}" ]; then
    echo "kill_session : survivants apres pkill -s ${sid} :" >&2
    ps -o pid,ppid,sid,pgid,args --no-headers -p "${surv//$'\n'/,}" >&2 || true
    kill -9 ${surv} 2>/dev/null || true
    sleep 1
  fi
  ! kill -0 "${sid}" 2>/dev/null
}
run_recovery() { # point d'entree de confiance : git show CLONE_COMMIT puis bash <copie> WORK ; [$1 = suffixe de journal]
  local entry="${SCENARIO_DIR}/rec_entree${1:+_$1}.sh" rc=0 sfx="${1:-}"
  git -C "${CLONE}" show "${CLONE_COMMIT}:gcp-migration/recover_v6_session.sh" > "${entry}"
  # FAKE_SCP_MODE_REPRISE : mode du faux scp pour la REPRISE seulement (le
  # superviseur, lui, doit reussir son scp de bundle avant d'etre tue).
  # FAKE_GCLOUD_STOP_RC_REPRISE : code du faux `instances stop` vu par la
  # VRAIE garde d'arret epinglee (contre-audit : jamais une fausse garde).
  env PATH="${SCENARIO_DIR}/bin:${PATH}" FAKE_GCLOUD_STOP_RC="${FAKE_GCLOUD_STOP_RC_REPRISE:-0}" \
    FAKE_SCP_MODE="${FAKE_SCP_MODE_REPRISE:-${FAKE_SCP_MODE:-ok}}" \
    bash "${entry}" "${SCEN_W}" > "${SCENARIO_DIR}/reprise_stdout${sfx:+_$sfx}.log" 2> "${SCENARIO_DIR}/reprise_stderr${sfx:+_$sfx}.log" || rc=$?
  return "${rc}"
}
stops_since() { # $1 = N0 : nombre d'arrets GCP journalises depuis la ligne N0
  tail -n +"$(($1 + 1))" "${FAKE_CALLS}" | grep -c 'compute instances stop' || true
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
# AXE FRONTIER_LAYOUT (alerte G4 du 2 septembre, P1) : le cycle de vie doit
# le CAPTURER du canon, le GRAVER au profil epingle (meme vide : « axe non
# demande » se distingue alors de « axe perdu ») et le TRANSMETTRE au runner
# distant — sans ce trajet, un profil qui declare la route serait refuse comme
# portant un axe inconnu, ou la route serait choisie par defaut sans preuve.
check_true "nominal mecanique : axe FRONTIER_LAYOUT capture, grave au profil epingle et transmis au runner" \
  bash -c "d=\$(ls -d '${SCENARIO_DIR}'/recu/s_* 2>/dev/null | head -1); [ -n \"\$d\" ] \
    && grep -q '^frontier_layout=' \"\$d/profil_campagne.txt\" \
    && grep -q \"FRONTIER_LAYOUT='\" '${FAKE_CALLS}'"
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

# ---- R. REPRISE PERSISTANTE (§ 5.18.6) : copies epinglees tirees du COMMIT
# du clone (la reprise se re-authentifie contre lui), superviseur lance en
# session propre puis SIGKILL de toute sa session apres le handshake.
export PIN_SOURCE=clone
# R2+R1 : superviseur vivant => refus ; puis kill -9 de la session, reprise
# avec seconde marque : scp partielle, UN STOP sur la generation exacte,
# validateur reel (partiel), classification forcee, recu de reprise.
# SSH_STEP_TIMEOUT_S=7200 : le build bloque ne se debloque jamais seul (un
# superviseur qui conclurait pendant l'attente ecrirait son propre recu et
# la reprise refuserait a bon droit « deja conclue »).
export FAKE_SCP_MODE=partiel SSH_STEP_TIMEOUT_S=7200
run_scenario_bg ok build_bloque 0 28800
check_true "reprise : handshake atteint, superviseur.pid grave (pid + starttime + boot_id)" \
  bash -c "$(declare -f wait_for_line); wait_for_line '${SCEN_W}/session.log' 'handshake : boot_id=' 60 \
    && [ \"\$(awk 'NF==5{print 1}' '${SCEN_W}/superviseur.pid')\" = '1' ] \
    && [ -f '${SCEN_W}/marques/guest_guard_pending' ] && [ -f '${SCEN_W}/marques/double_guard_verified' ] \
    && grep -q 'generation=${FAKE_GEN}' '${SCEN_W}/marques/double_guard_verified' \
    && grep -q '^GUARDS_DIR=' '${SCEN_W}/session.env'"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "R2 : reprise pendant que le superviseur vit => REFUS 2, aucun appel gcloud/garde" \
  bash -c "[ '${rc}' -eq 2 ] && grep -q 'superviseur vivant' '${SCENARIO_DIR}/reprise_stderr.log' \
    && [ \"\$(wc -l < '${FAKE_CALLS}')\" -eq '${N0}' ]"
check_true "R1 : SIGKILL de toute la session du superviseur (aucun processus ne reference WORK)" \
  bash -c "$(declare -f kill_session); SCEN_W='${SCEN_W}'; SUP_WAIT_PID='${SUP_WAIT_PID}'; SCENARIO_DIR='${SCENARIO_DIR}'; kill_session \
    && [ -z \"\$(pgrep -f -- '${SCEN_W}/[p]inned' || true)\" ]"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "R1 : reprise avec double_guard_verified => rc 0, 0 SETMAX/START/ssh, 1 scp, 1 STOP sur la generation exacte, avant validation" \
  bash -c "[ '${rc}' -eq 0 ] && tail -n +$((N0 + 1)) '${FAKE_CALLS}' > '${SCENARIO_DIR}/reprise_calls.log' \
    && ! grep -qE '^(SETMAX|START) ' '${SCENARIO_DIR}/reprise_calls.log' \
    && ! grep -q 'compute ssh' '${SCENARIO_DIR}/reprise_calls.log' \
    && [ \"\$(grep -c 'compute scp' '${SCENARIO_DIR}/reprise_calls.log')\" -eq 1 ] \
    && [ \"\$(grep -c 'compute instances stop' '${SCENARIO_DIR}/reprise_calls.log')\" -eq 1 ] \
    && grep -q -- 'arret cible : ${SCEN_W}/pinned/gcp-migration/stop_and_verify.sh --yes --expected-last-start-timestamp ${FAKE_GEN}' '${SCENARIO_DIR}/reprise_stdout.log' \
    && grep -q 'compute instances describe .*value(lastStartTimestamp)' '${SCENARIO_DIR}/reprise_calls.log' \
    && grep -q '^CLOUDSDK ${SCEN_W}/gcloud-config' '${SCENARIO_DIR}/reprise_calls.log'"
RECU_R1="$(ls -d "${SCENARIO_DIR}"/recu/s_*_reprise_* 2>/dev/null | head -1 || true)"
tail -n 12 "${SCENARIO_DIR}/reprise_stderr.log" "${SCENARIO_DIR}/reprise_stdout.log" >&2 || true
check_true "R1 : recu de reprise (issue, classification forcee, jamais une decision, marques, journaux, SHA256SUMS), registre targeted_stopped, credentials purges" \
  bash -c "[ -n '${RECU_R1}' ] && grep -q '^issue=reprise_apres_perte_superviseur' '${RECU_R1}/RECU_SESSION.txt' \
    && grep -q '^classification=partiel_ou_invalide' '${RECU_R1}/RECU_SESSION.txt' \
    && grep -q '^decision=aucune' '${RECU_R1}/RECU_SESSION.txt' && grep -q '^scp_rc=0' '${RECU_R1}/RECU_SESSION.txt' \
    && grep -q '^validate_rc=1' '${RECU_R1}/RECU_SESSION.txt' && grep -q 'campaign_status=partial_or_failed' '${RECU_R1}/validation.txt' \
    && grep -q '^marques=.*double_guard_verified' '${RECU_R1}/RECU_SESSION.txt' && grep -q '^verrou=flock' '${RECU_R1}/RECU_SESSION.txt' \
    && [ -f '${RECU_R1}/marques/guest_guard_pending' ] && [ -f '${RECU_R1}/reprise.log' ] \
    && grep -q 'handshake : boot_id=' '${RECU_R1}/session.log' && [ -f '${RECU_R1}/out/bench_resume.txt' ] \
    && grep -q ' \./out/SHA256SUMS$' '${RECU_R1}/SHA256SUMS' && [ -f '${SCEN_W}/out.promotion' ] && grep -qx 'generation=${FAKE_GEN}' '${SCEN_W}/out.promotion' \
    && ( cd '${RECU_R1}' && sha256sum -c --quiet SHA256SUMS ) && [ ! -d '${RECU_R1}/ssh' ] && [ ! -d '${RECU_R1}/gcloud-config' ] \
    && grep -q '^state=targeted_stopped' '${SCEN_W}/etat_cycle_vie' && grep -q '^generation=${FAKE_GEN}' '${SCEN_W}/etat_cycle_vie' \
    && [ -f '${SCEN_W}/recu_publie' ] && [ ! -e '${SCEN_W}/ssh/id_ed25519' ] && [ ! -d '${SCEN_W}/gcloud-config' ]"
rc=0; run_recovery || rc=$?
check_true "R1bis : seconde reprise apres session conclue => REFUS 2 (recu_publie), aucun appel" \
  bash -c "[ '${rc}' -eq 2 ] && grep -q 'deja conclue' '${SCENARIO_DIR}/reprise_stderr.log'"
unset FAKE_SCP_MODE

# R3 : superviseur tue ENTRE les deux marques (garde bloque avant l'armement
# invite) => stop immediat sur la generation exacte, aucune scp.
run_scenario_bg ok_bloque ok 0 28800
check_true "R3 : premiere marque publiee, seconde absente (garde bloque)" \
  bash -c "$(declare -f wait_for_line); wait_for_line '${SCEN_W}/marques/guest_guard_pending' 'generation=${FAKE_GEN}' 60 \
    && [ ! -e '${SCEN_W}/marques/double_guard_verified' ]"
bash -c "$(declare -f kill_session); SCEN_W='${SCEN_W}'; SUP_WAIT_PID='${SUP_WAIT_PID}'; SCENARIO_DIR='${SCENARIO_DIR}'; kill_session" || true
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
RECU_R3="$(ls -d "${SCENARIO_DIR}"/recu/s_*_reprise_* 2>/dev/null | head -1 || true)"
tail -n 6 "${SCENARIO_DIR}/reprise_stderr.log" >&2 || true
check_true "R3 : reprise sans double_guard_verified => arret immediat (1 STOP exact), 0 scp, issue=reprise_sans_double_garde" \
  bash -c "[ '${rc}' -eq 0 ] && tail -n +$((N0 + 1)) '${FAKE_CALLS}' > '${SCENARIO_DIR}/reprise_calls.log' \
    && ! grep -q 'compute scp' '${SCENARIO_DIR}/reprise_calls.log' && ! grep -qE '^(SETMAX|START) ' '${SCENARIO_DIR}/reprise_calls.log' \
    && [ \"\$(grep -c 'compute instances stop' '${SCENARIO_DIR}/reprise_calls.log')\" -eq 1 ] \
    && grep -q -- 'arret cible : .*--expected-last-start-timestamp ${FAKE_GEN}' '${SCENARIO_DIR}/reprise_stdout.log' \
    && [ -n '${RECU_R3}' ] && grep -q '^issue=reprise_sans_double_garde' '${RECU_R3}/RECU_SESSION.txt' \
    && ! grep -q 'double_guard_verified' '${RECU_R3}/RECU_SESSION.txt'"

# R4 : mutants de la reprise (chacun sur une session tuee apres handshake) ;
# le scp en echec ne concerne que la reprise (R5), jamais le bundle du superviseur.
export FAKE_SCP_MODE_REPRISE=echec
run_scenario_bg ok build_bloque 0 28800
check_true "R4 : handshake atteint puis session du superviseur tuee (aucun recu du superviseur)" \
  bash -c "$(declare -f wait_for_line kill_session); SCEN_W='${SCEN_W}'; SUP_WAIT_PID='${SUP_WAIT_PID}'; SCENARIO_DIR='${SCENARIO_DIR}'; \
    wait_for_line '${SCEN_W}/session.log' 'handshake : boot_id=' 60 && kill_session \
    && [ -z \"\$(ls -d '${SCENARIO_DIR}'/recu/s_* 2>/dev/null)\" ] && [ ! -e '${SCEN_W}/recu_publie' ]"
# R4a : marque 2 d'une autre generation => BLOCAGE 71, aucun STOP.
cp "${SCEN_W}/marques/double_guard_verified" "${SCENARIO_DIR}/m2.sauv"
sed -i 's/^generation=.*/generation=generation-perimee/' "${SCEN_W}/marques/double_guard_verified"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "R4a : generations discordantes registre/marque => BLOCAGE 71, zero STOP" \
  bash -c "[ '${rc}' -eq 71 ] && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop' || true)\" -eq 0 ]"
cp -f "${SCENARIO_DIR}/m2.sauv" "${SCEN_W}/marques/double_guard_verified"
# R4b : copie epinglee de la reprise alteree => refus 2 a l'etage 1, aucun appel.
echo "# altere" >> "${SCEN_W}/pinned/gcp-migration/recover_v6_session.sh"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "R4b : copie epinglee de la reprise alteree => refus 2 avant tout appel" \
  bash -c "[ '${rc}' -eq 2 ] && grep -q 'alteree' '${SCENARIO_DIR}/reprise_stderr.log' && [ \"\$(wc -l < '${FAKE_CALLS}')\" -eq '${N0}' ]"
git -C "${CLONE}" show "${CLONE_COMMIT}:gcp-migration/recover_v6_session.sh" > "${SCEN_W}/pinned/gcp-migration/recover_v6_session.sh"
# R4c : session.env d'une autre cible => refus/blocage sans appel.
sed -i 's/^GCP_INSTANCE_NAME=.*/GCP_INSTANCE_NAME=autre-instance/' "${SCEN_W}/session.env"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "R4c : cible de session.env != registre => blocage 71, zero STOP" \
  bash -c "[ '${rc}' -eq 71 ] && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop' || true)\" -eq 0 ]"
sed -i 's/^GCP_INSTANCE_NAME=.*/GCP_INSTANCE_NAME=instance-factice/' "${SCEN_W}/session.env"
# R4d : pid recycle simule (starttime faux) => la reprise PROCEDE ; R5 : scp
# en echec => STOP quand meme, scp_rc=1 au recu.
sed -i 's/^\([0-9]*\) [0-9]* /\1 999999999 /' "${SCEN_W}/superviseur.pid"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
RECU_R5="$(ls -d "${SCENARIO_DIR}"/recu/s_*_reprise_* 2>/dev/null | head -1 || true)"
tail -n 6 "${SCENARIO_DIR}/reprise_stderr.log" >&2 || true
check_true "R4d/R5 : pid recycle (starttime different) => reprise executee ; scp en echec => 1 STOP exact, scp_rc=1 grave" \
  bash -c "[ '${rc}' -eq 0 ] && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ] \
    && [ -n '${RECU_R5}' ] && grep -q '^scp_rc=1' '${RECU_R5}/RECU_SESSION.txt' \
    && [ -z \"\$(ls -A '${SCEN_W}/out' 2>/dev/null)\" ] && [ -z \"\$(ls -d '${SCEN_W}'/out.partiel_* 2>/dev/null)\" ] \
    && grep -q '^issue=reprise_apres_perte_superviseur' '${RECU_R5}/RECU_SESSION.txt'"
# R6 : ARRET EN ECHEC pendant la reprise (la dent revelee par le bug de
# PIPESTATUS) : rc 70, registre targeted_stop_failed, AUCUN temoin de
# conclusion ni purge, seconde reprise permise et cette fois certifiee.
unset FAKE_SCP_MODE_REPRISE
run_scenario_bg ok build_bloque 0 28800
check_true "R6 : handshake atteint puis session tuee" \
  bash -c "$(declare -f wait_for_line kill_session); SCEN_W='${SCEN_W}'; SUP_WAIT_PID='${SUP_WAIT_PID}'; SCENARIO_DIR='${SCENARIO_DIR}'; \
    wait_for_line '${SCEN_W}/session.log' 'handshake : boot_id=' 60 && kill_session"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; FAKE_GCLOUD_STOP_RC_REPRISE=1 run_recovery || rc=$?
RECU_R6="$(ls -d "${SCENARIO_DIR}"/recu/s_*_reprise_* 2>/dev/null | head -1 || true)"
check_true "R6 : stop en echec (vraie garde, faux instances stop rc=1) => rc 70, registre targeted_stop_failed, aucun recu_publie ni purge, temoin MINIMAL (ni out/ ni validation), ARRET NON CERTIFIE annonce" \
  bash -c "[ '${rc}' -eq 70 ] && grep -q '^state=targeted_stop_failed' '${SCEN_W}/etat_cycle_vie' \
    && [ ! -e '${SCEN_W}/recu_publie' ] && [ -f '${SCEN_W}/ssh/id_ed25519' ] && [ -d '${SCEN_W}/gcloud-config' ] \
    && grep -q 'ARRET NON CERTIFIE' '${SCENARIO_DIR}/reprise_stderr.log' \
    && [ -n '${RECU_R6}' ] && grep -q '^stop_rc=1' '${RECU_R6}/RECU_SESSION.txt' && grep -q '^temoin_minimal=1' '${RECU_R6}/RECU_SESSION.txt' \
    && [ ! -d '${RECU_R6}/out' ] && [ ! -d '${RECU_R6}/marques' ] && [ ! -f '${RECU_R6}/validation.txt' ] && [ ! -f '${SCEN_W}/validation.txt' ] \
    && [ -f '${RECU_R6}/reprise.tail.log' ] && [ \"\$(stat -c %s '${RECU_R6}/reprise.tail.log')\" -le 65536 ] && [ ! -f '${RECU_R6}/reprise.log' ]"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
RECU_R6B="$(ls -td "${SCENARIO_DIR}"/recu/s_*_reprise_* 2>/dev/null | head -1 || true)"
check_true "R6bis : seconde reprise apres echec d'arret => STOP-FIRST (aucun scp ni describe de reprise avant l'arret), 1 STOP exact certifie, registre targeted_stopped, credentials purges, stop_first=1" \
  bash -c "[ '${rc}' -eq 0 ] && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ] \
    && ! tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -q 'compute scp' \
    && ! tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -q 'value(status,lastStartTimestamp)' \
    && grep -q '^state=targeted_stopped' '${SCEN_W}/etat_cycle_vie' && [ -f '${SCEN_W}/recu_publie' ] \
    && [ ! -e '${SCEN_W}/ssh/id_ed25519' ] && [ ! -d '${SCEN_W}/gcloud-config' ] \
    && [ -n '${RECU_R6B}' ] && grep -q '^stop_first=1' '${RECU_R6B}/RECU_SESSION.txt'"

# ---- DENTS DU CONTRE-AUDIT (CONTRE_AUDIT_REPRISE_PERSISTANTE_V6_20260902).
new_killed_session() { # session tuee apres handshake, prete pour une reprise
  run_scenario_bg ok build_bloque 0 28800
  bash -c "$(declare -f wait_for_line kill_session); SCEN_W='${SCEN_W}'; SUP_WAIT_PID='${SUP_WAIT_PID}'; SCENARIO_DIR='${SCENARIO_DIR}'; \
    wait_for_line '${SCEN_W}/session.log' 'handshake : boot_id=' 60 && kill_session" || return 1
  # Les variables de session sont deja celles du scenario courant.
  return 0
}
# D1 : deux reprises SIMULTANEES dans la meme session POSIX => une seule
# franchit le verrou noyau, exactement un arret GCP.
new_killed_session || true
N0="$(wc -l < "${FAKE_CALLS}")"
run_recovery a & PA=$!
run_recovery b & PB=$!
rca=0; wait "${PA}" || rca=$?
rcb=0; wait "${PB}" || rcb=$?
check_true "D1 : deux reprises simultanees (meme sid) => une passe (rc 0), l'autre est refusee par le verrou (rc 2), UN seul arret GCP" \
  bash -c "{ [ '${rca}' -eq 0 ] && [ '${rcb}' -eq 2 ]; } || { [ '${rca}' -eq 2 ] && [ '${rcb}' -eq 0 ]; } \
    && grep -q 'verrou' '${SCENARIO_DIR}/reprise_stderr_a.log' '${SCENARIO_DIR}/reprise_stderr_b.log' \
    && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ]"
# D2 : registre targeted_stopped a generation VIDE + handoff valide => 71,
# zero temoin, zero purge, zero appel.
new_killed_session || true
printf 'schema=e-hgp.lifecycle-state.v1\nstate=targeted_stopped\nproject=%s\nzone=%s\ninstance=%s\ngeneration=\n' \
  projet-factice zone-factice instance-factice > "${SCEN_W}/etat_cycle_vie"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D2 : targeted_stopped sans generation (handoff valide) => BLOCAGE 71, aucun temoin, credentials intacts, aucun appel" \
  bash -c "[ '${rc}' -eq 71 ] && [ ! -e '${SCEN_W}/recu_publie' ] && [ -f '${SCEN_W}/ssh/id_ed25519' ] \
    && [ \"\$(wc -l < '${FAKE_CALLS}')\" -eq '${N0}' ]"
# D3 : describe = RUNNING <autre generation> AVANT la scp => 71, zero scp,
# zero promotion, zero arret de cette autre generation.
new_killed_session || true
export FAKE_DESCRIBE_GEN=generation-concurrente
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D3 : cible portant une AUTRE generation avant la scp => 71, zero scp, zero out/ promu, zero STOP" \
  bash -c "[ '${rc}' -eq 71 ] && ! tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -q 'compute scp' \
    && [ -z \"\$(ls -A '${SCEN_W}/out' 2>/dev/null)\" ] && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop' || true)\" -eq 0 ] \
    && grep -q 'AUTRE generation' '${SCEN_W}/reprise.log'"
unset FAKE_DESCRIBE_GEN
# D3bis : generation changee PENDANT la scp => 71, staging detruit, rien promu, zero STOP.
new_killed_session || true
export FAKE_DESCRIBE_GEN_AFTER_SCP=generation-concurrente
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D3bis : generation changee pendant le rapatriement => 71, une scp, staging detruit, aucun out/ promu, zero STOP" \
  bash -c "[ '${rc}' -eq 71 ] && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute scp')\" -eq 1 ] \
    && [ -z \"\$(ls -A '${SCEN_W}/out' 2>/dev/null)\" ] && [ ! -d '${SCEN_W}/out.staging' ] \
    && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop' || true)\" -eq 0 ]"
unset FAKE_DESCRIBE_GEN_AFTER_SCP
# D4 : marque double_guard_verified dont mark=guest_guard_pending => 71, ni scp ni STOP.
new_killed_session || true
sed -i 's/^mark=double_guard_verified$/mark=guest_guard_pending/' "${SCEN_W}/marques/double_guard_verified"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D4 : marque double_guard_verified au champ mark=guest_guard_pending => BLOCAGE 71, ni scp ni STOP" \
  bash -c "[ '${rc}' -eq 71 ] && ! tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -qE 'compute scp|compute instances stop'"
# D5 : membre orphelin de la session du superviseur (sid grave) sans WORK
# dans son argv => reprise refusee ; tue => la reprise procede.
new_killed_session || true
setsid -f bash -c 'echo $$ > "$1"; exec sleep 300' _ "${SCENARIO_DIR}/orph.pid"
sleep 1; ORPH="$(cat "${SCENARIO_DIR}/orph.pid")"
ORPH_SID="$(ps -o sid= -p "${ORPH}" | tr -d ' ')"
awk -v s="${ORPH_SID}" '{ $4 = s; print }' "${SCEN_W}/superviseur.pid" > "${SCEN_W}/superviseur.pid.tmp" && mv "${SCEN_W}/superviseur.pid.tmp" "${SCEN_W}/superviseur.pid"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D5 : fils orphelin vivant dans la session (sid) du superviseur, sans WORK dans son argv => REFUS 2, aucun appel" \
  bash -c "[ '${rc}' -eq 2 ] && grep -q 'membres de la session' '${SCENARIO_DIR}/reprise_stderr.log' && [ \"\$(wc -l < '${FAKE_CALLS}')\" -eq '${N0}' ]"
kill -9 "${ORPH}" 2>/dev/null || true; sleep 1
rc=0; run_recovery || rc=$?
check_true "D5bis : orphelin tue => la reprise procede (rc 0, un STOP)" \
  bash -c "[ '${rc}' -eq 0 ] && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ]"
# D6 : PURGE EN ECHEC apres targeted_stopped (repertoire ssh non modifiable)
# => aucun temoin, rc 67 ; puis purge locale re-jouable SANS appel GCP.
new_killed_session || true
chmod 500 "${SCEN_W}/ssh"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D6 : purge des credentials en echec apres arret certifie => rc 67, PURGE INCOMPLETE, aucun temoin recu_publie, registre targeted_stopped" \
  bash -c "[ '${rc}' -eq 67 ] && [ ! -e '${SCEN_W}/recu_publie' ] && [ -f '${SCEN_W}/ssh/id_ed25519' ] \
    && grep -q 'PURGE INCOMPLETE' '${SCEN_W}/reprise.log' && grep -q '^state=targeted_stopped' '${SCEN_W}/etat_cycle_vie'"
chmod 700 "${SCEN_W}/ssh"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D6bis : reprise apres purge en echec => purge locale, temoin publie, rc 0, AUCUN appel GCP (ni describe, ni scp, ni STOP)" \
  bash -c "[ '${rc}' -eq 0 ] && [ -f '${SCEN_W}/recu_publie' ] && [ ! -e '${SCEN_W}/ssh/id_ed25519' ] && [ ! -d '${SCEN_W}/gcloud-config' ] \
    && [ \"\$(wc -l < '${FAKE_CALLS}')\" -eq '${N0}' ]"
# D7 : stop-first des l'entree en targeted_stop_failed, puis TROISIEME rejeu
# conforme a la politique (rejeux manuels, un STOP chacun, jamais de boucle).
new_killed_session || true
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; FAKE_GCLOUD_STOP_RC_REPRISE=1 run_recovery || rc=$?
check_true "D7 : premier rejeu (arret en echec) => rc 70, une scp puis un STOP" \
  bash -c "[ '${rc}' -eq 70 ] && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ]"
N1="$(wc -l < "${FAKE_CALLS}")"
rc=0; FAKE_GCLOUD_STOP_RC_REPRISE=1 run_recovery || rc=$?
check_true "D7bis : deuxieme rejeu depuis targeted_stop_failed => STOP-FIRST : premier appel externe = arret (aucun describe de reprise, aucune scp), rc 70, temoin minimal" \
  bash -c "[ '${rc}' -eq 70 ] && [ \"\$(tail -n +$((N1 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ] \
    && ! tail -n +$((N1 + 1)) '${FAKE_CALLS}' | grep -qE 'compute scp|value\(status,lastStartTimestamp\)' \
    && [ \"\$(tail -n +$((N1 + 1)) '${FAKE_CALLS}' | grep 'GCLOUD' | head -n 1 | grep -c 'config get-value project')\" -eq 1 ]"
N2="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D7ter : troisieme rejeu => admis (politique : rejeux manuels, stop-first, un STOP), certifie, rc 0" \
  bash -c "[ '${rc}' -eq 0 ] && [ \"\$(tail -n +$((N2 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ] \
    && grep -q '^state=targeted_stopped' '${SCEN_W}/etat_cycle_vie' && [ -f '${SCEN_W}/recu_publie' ]"
# D8 : purge en echec dans le CYCLE NOMINAL (repertoire ssh non modifiable
# apres le handshake) => aucun temoin, marqueur purge_incomplete ; la
# reprise re-purge ensuite localement sans appel GCP.
export FAKE_BUILD_SLEEP_S=12
run_scenario_bg ok build_lent 0 28800
bash -c "$(declare -f wait_for_line); wait_for_line '${SCEN_W}/session.log' 'handshake : boot_id=' 60" || true
chmod 500 "${SCEN_W}/ssh"
rc=0; wait "${SUP_WAIT_PID}" 2>/dev/null || rc=$?
check_true "D8 : cycle nominal, purge des credentials en echec apres targeted_stopped => code 67, aucun recu_publie, marqueur purge_incomplete, cle encore presente, registre targeted_stopped" \
  bash -c "[ '${rc}' -eq 67 ] && [ ! -e '${SCEN_W}/recu_publie' ] && [ -f '${SCEN_W}/purge_incomplete' ] && [ -f '${SCEN_W}/ssh/id_ed25519' ] \
    && grep -q '^state=targeted_stopped' '${SCEN_W}/etat_cycle_vie' && grep -q 'PURGE INCOMPLETE' '${SCEN_W}/session.log'"
chmod 700 "${SCEN_W}/ssh"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D8bis : reprise apres purge nominale en echec => purge locale, temoin publie, rc 0, AUCUN appel GCP" \
  bash -c "[ '${rc}' -eq 0 ] && [ -f '${SCEN_W}/recu_publie' ] && [ ! -e '${SCEN_W}/ssh/id_ed25519' ] && [ ! -d '${SCEN_W}/gcloud-config' ] \
    && [ \"\$(wc -l < '${FAKE_CALLS}')\" -eq '${N0}' ]"
unset FAKE_BUILD_SLEEP_S
# D9 (§ 5.21) : temoin NON publiable (recu_publie est un repertoire) apres un
# arret certifie et une purge reussie => code 68 dedie, jamais 0 ; une fois
# l'obstacle retire, la reprise publie le temoin sans appel GCP.
new_killed_session || true
mkdir -p "${SCEN_W}/recu_publie"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D9 : temoin non publiable apres arret certifie et purge => rc 68, TEMOIN NON PUBLIE, credentials purges, registre targeted_stopped" \
  bash -c "[ '${rc}' -eq 68 ] && grep -q 'TEMOIN NON PUBLIE' '${SCEN_W}/reprise.log' && [ ! -e '${SCEN_W}/ssh/id_ed25519' ] \
    && grep -q '^state=targeted_stopped' '${SCEN_W}/etat_cycle_vie' && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ]"
rmdir "${SCEN_W}/recu_publie"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D9bis : obstacle retire => temoin publie, rc 0, AUCUN appel GCP" \
  bash -c "[ '${rc}' -eq 0 ] && [ -f '${SCEN_W}/recu_publie' ] && [ \"\$(wc -l < '${FAKE_CALLS}')\" -eq '${N0}' ]"
# D10 (§ 5.21) : tuple posterieur ILLISIBLE apres la scp => rien promu, partiel
# conserve sous un nom explicite, un seul arret cible de la generation connue.
new_killed_session || true
export FAKE_DESCRIBE_FAIL_AFTER_SCP=1 FAKE_SCP_MODE_REPRISE=partiel
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D10 : describe indisponible apres la scp => staging NON promu (out/ vide, out.partiel_* conserve), un STOP exact, rc 0" \
  bash -c "[ '${rc}' -eq 0 ] && [ -z \"\$(ls -A '${SCEN_W}/out' 2>/dev/null)\" ] && [ -n \"\$(ls -d '${SCEN_W}'/out.partiel_* 2>/dev/null)\" ] \
    && grep -q 'staging NON promu' '${SCEN_W}/reprise.log' \
    && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ] && [ ! -f '${SCEN_W}/validation.txt' ]"
unset FAKE_DESCRIBE_FAIL_AFTER_SCP FAKE_SCP_MODE_REPRISE
# D12 (§ 5.22) : ERREUR LOCALE pre-STOP (faux tee en echec apres la scp) => le
# funnel d'arret execute exactement UN stop cible, registre targeted_stopped,
# rc 74, temoin minimal (issue=reprise_erreur_locale).
new_killed_session || true
export FAKE_TEE_FAIL_AFTER_SCP=1 FAKE_SCP_MODE_REPRISE=partiel
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
RECU_D12="$(ls -td "${SCENARIO_DIR}"/recu/s_*_reprise_* 2>/dev/null | head -1 || true)"
check_true "D12 : erreur locale apres la scp (tee en echec) => funnel : exactement un STOP, registre targeted_stopped, rc 74, issue=reprise_erreur_locale" \
  bash -c "[ '${rc}' -eq 74 ] && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ] \
    && grep -q '^state=targeted_stopped' '${SCEN_W}/etat_cycle_vie' \
    && [ -n '${RECU_D12}' ] && grep -q '^issue=reprise_erreur_locale' '${RECU_D12}/RECU_SESSION.txt' && grep -q '^temoin_minimal=1' '${RECU_D12}/RECU_SESSION.txt'"
unset FAKE_TEE_FAIL_AFTER_SCP FAKE_SCP_MODE_REPRISE
# D13 (§ 5.22) : PROVENANCE de out/ — un out/ non vide laisse par une tentative
# anterieure, scp courante en echec => aucun marqueur out.promotion, aucun
# validateur, classification forcee ; le partiel n'est pas promu.
new_killed_session || true
mkdir -p "${SCEN_W}/out"; echo "ancien" > "${SCEN_W}/out/bench_resume.txt"
printf 'schema=e-hgp.out-promotion.v1\ngeneration=%s\nsource_commit=%s\nscp_rc=0\nattempt=1000000000_1\nepoch=1000000000\n' "${FAKE_GEN}" "${SCEN_COMMIT}" > "${SCEN_W}/out.promotion"
export FAKE_SCP_MODE_REPRISE=echec
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D13 : out/ non vide + marqueur out.promotion VALIDE d'une tentative anterieure + scp en echec => aucun validateur (validation.txt absent), un STOP, rc 0" \
  bash -c "[ '${rc}' -eq 0 ] && [ ! -f '${SCEN_W}/validation.txt' ] \
    && grep -q 'aucun out/ promu par CE rapatriement' '${SCEN_W}/reprise.log' \
    && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ]"
unset FAKE_SCP_MODE_REPRISE
# D14 (§ 5.22 retour WIP) : panne locale des la PREMIERE ligne apres la
# resolution de la generation (faux tee sur « registre= ») => funnel : un STOP,
# registre targeted_stopped, rc 74.
new_killed_session || true
export FAKE_TEE_FAIL_ON="registre="
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D14 : panne de rlog des la generation connue (avant scp) => funnel : exactement un STOP, targeted_stopped, rc 74" \
  bash -c "[ '${rc}' -eq 74 ] && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ] \
    && grep -q '^state=targeted_stopped' '${SCEN_W}/etat_cycle_vie' && grep -q 'ERREUR LOCALE' '${SCENARIO_DIR}/reprise_stderr.log'"
unset FAKE_TEE_FAIL_ON
# D15 : panne de publish_state (faux python3 sur le registre) => la garde
# tourne quand meme exactement une fois (best effort autour), code non nul,
# jamais un second STOP.
new_killed_session || true
export FAKE_PY_STATE_FAIL=1
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D15 : publish_state en echec => garde epinglee executee exactement une fois (best effort), code non nul (${rc}), aucun temoin" \
  bash -c "[ '${rc}' -ne 0 ] && [ \"\$(tail -n +$((N0 + 1)) '${FAKE_CALLS}' | grep -c 'compute instances stop')\" -eq 1 ] && [ ! -e '${SCEN_W}/recu_publie' ]"
unset FAKE_PY_STATE_FAIL
# D11 (§ 5.21) : cycle NOMINAL, temoin non publiable (recu_publie est un
# repertoire cree apres le handshake) => code 68 dedie, marqueur
# temoin_non_publie, purge faite ; la reprise publie ensuite le temoin.
export FAKE_BUILD_SLEEP_S=12
run_scenario_bg ok build_lent 0 28800
RDV=0; bash -c "$(declare -f wait_for_line); wait_for_line '${SCEN_W}/session.log' 'handshake : boot_id=' 60" || RDV=$?
check_true "D11 : rendez-vous du handshake atteint (fatal sinon)" [ "${RDV}" -eq 0 ]
mkdir -p "${SCEN_W}/recu_publie"
rc=0; wait "${SUP_WAIT_PID}" 2>/dev/null || rc=$?
RECU_D11="$(ls -td "${SCENARIO_DIR}"/recu/s_* 2>/dev/null | head -1 || true)"
check_true "D11 : cycle nominal, temoin non publiable => code 68, marqueur temoin_non_publie, TEMOIN NON PUBLIE au journal, credentials purges, registre targeted_stopped, EXACTEMENT un STOP, fast-path « deja certifie par le garde », issue=arret_certifie_par_le_garde" \
  bash -c "[ '${rc}' -eq 68 ] && [ -f '${SCEN_W}/temoin_non_publie' ] && grep -q 'TEMOIN NON PUBLIE' '${SCEN_W}/session.log' \
    && [ ! -e '${SCEN_W}/ssh/id_ed25519' ] && grep -q '^state=targeted_stopped' '${SCEN_W}/etat_cycle_vie' \
    && [ \"\$(grep -c '^STOP ' '${FAKE_CALLS}')\" -eq 1 ] && grep -q 'arret deja certifie par le garde' '${SCENARIO_DIR}/stdout.log' \
    && [ -n '${RECU_D11}' ] && grep -q '^issue=arret_certifie_par_le_garde' '${RECU_D11}/RECU_SESSION.txt'"
rmdir "${SCEN_W}/recu_publie"
N0="$(wc -l < "${FAKE_CALLS}")"
rc=0; run_recovery || rc=$?
check_true "D11bis : reprise apres temoin nominal non publie => temoin publie, rc 0, AUCUN appel GCP" \
  bash -c "[ '${rc}' -eq 0 ] && [ -f '${SCEN_W}/recu_publie' ] && [ \"\$(wc -l < '${FAKE_CALLS}')\" -eq '${N0}' ]"
unset FAKE_BUILD_SLEEP_S
unset SSH_STEP_TIMEOUT_S PIN_SOURCE

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

# 11bis. Base de sessions NON 0700 => refus 2 avant tout `git show`.
BASE755="$(mktemp -d "${BASE}/base755.XXXXXX")"; chmod 755 "${BASE755}"
rc=0
( cd "${CLONE}" && MHGP6_SESSION_BASE="${BASE755}" bash gcp-migration/session_campagne_v6_g4.sh ) >/dev/null 2>"${BASE}/boot11bis.err" || rc=$?
check_true "base de sessions 755 : refus 2 avant materialisation (base 0700 exigee)" \
  bash -c "[ '${rc}' -eq 2 ] && grep -q '0700' '${BASE}/boot11bis.err' && [ -z \"\$(ls -A '${BASE755}')\" ]"

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
echo "selftest cycle de vie v6 : arret cible ou blocage prouve sur chaque sortie apres demarrage (35 scenarios dont reprise EXECUTEE a deux appels ordonnes et six mutants permanents du registre — perdu, duplique, sans schema, tronque, targeted_stopped d'une autre generation, publication interrompue, registre illisible=blocage, surcharge temporelle refusee, trop-tard sans remontee, describe borne, registre etranger post-arret=78, grace fixe 29/31 refusees, relation invite/GCE avant set_max, STOP1<STOP2<VALIDATE, double echec sans validation, contre-calendrier a describe clampe, ordre STOP1<STOP2<VALIDATE au ledger, sortie pre-SCP a seconde reserve, garde scp causale a la seconde — + 17 refus de pin sur l inventaire repo-relatif a deux repertoires, reprise persistante § 5.18.6 : SIGKILL de la session du superviseur apres le handshake puis reprise re-authentifiee — superviseur vivant refuse, un seul STOP exact, scp partielle/echec, classification forcee, sans seconde marque arret immediat, mutants generation/cible/copie alteree/pid recycle/base 755, rejouable depuis un HEAD propre)"
