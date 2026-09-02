#!/usr/bin/env bash
# CYCLE DE VIE de la session G4 v6 — execute EXCLUSIVEMENT comme copie
# materialisee depuis le COMMIT par le bootstrap session_campagne_v6_g4.sh
# (P0-2 de l'audit GCP v6), ou par le selftest de cycle de vie
# (selftest_cycle_vie_v6.sh, fausses gardes + faux gcloud). Il n'execute que
# les gardes du repertoire MHGP6_LIFECYCLE_GUARDS_DIR (copies epinglees en
# production), y compris dans le trap.
#
# GARANTIES P0-1 (audit GCP v6 du 31 aout) :
#  - le trap d'arret est installe AVANT toute mutation GCP ;
#  - la garde de demarrage ecrit un TEMOIN DE MUTATION durable et atomique
#    (--mutation-witness-file) au point exact ou le start GCE commence ;
#  - cleanup commence par neutraliser sa recursion et `set +e`, preserve le
#    code initial, et AUCUNE ecriture de journal ne peut empecher l'arret ;
#  - table de decision : temoin absent => refus avant mutation, propager sans
#    arret ni blocage ; temoin present + generation lisible (handoff relu et
#    valide si besoin) => UNE tentative d'arret ciblee ; temoin present +
#    generation illisible => BLOCAGE explicite (projet, zone, instance,
#    commande de controle), jamais un arret non cible ni un silence.
#
# P1 : echeance derivee du lastStartTimestamp certifie ; preflight budgetaire
# declare ; profil de campagne epingle AVANT la campagne et transmis au
# validateur ; journaux ctest complets avec planchers ; handshake de
# generation et de boot_id autour des SSH ; chemins distants propres a la
# (pin, generation) ; SSH/SCP bornes par timeout ; recu durable apres arret.
set -euo pipefail

WORK="${MHGP6_LIFECYCLE_WORK:?MHGP6_LIFECYCLE_WORK requis}"
GUARDS_DIR="${MHGP6_LIFECYCLE_GUARDS_DIR:?MHGP6_LIFECYCLE_GUARDS_DIR requis}"
SOURCE_COMMIT="${MHGP6_LIFECYCLE_SOURCE_COMMIT:?commit requis}"
SOURCE_PAYLOAD_SHA256="${MHGP6_LIFECYCLE_PAYLOAD_SHA256:?sha du payload requis}"
PROTOCOL_MANIFEST_SHA256="${MHGP6_LIFECYCLE_MANIFEST_SHA256:?manifeste requis}"
for g in set_max_run_duration_and_verify.sh start_and_verify.sh stop_and_verify.sh; do
  [ -x "${GUARDS_DIR}/${g}" ] || { echo "REFUS : garde epinglee absente (${GUARDS_DIR}/${g})" >&2; exit 2; }
done
RUNNER="${WORK}/pinned/gcp-migration/v6_campaign_remote.sh"
VALIDATOR="${WORK}/pinned/gcp-migration/validate_v6_campaign.py"
BUNDLE="${WORK}/bundle.tgz"
for f in "${RUNNER}" "${VALIDATOR}" "${BUNDLE}"; do
  [ -e "${f}" ] || { echo "REFUS : artefact epingle absent (${f})" >&2; exit 2; }
done
echo "${SOURCE_PAYLOAD_SHA256}  ${BUNDLE}" | sha256sum -c - >/dev/null \
  || { echo "REFUS : bundle epingle au mauvais sha256" >&2; exit 2; }

# REVALIDATION DU MANIFESTE CANONIQUE avant toute execution (audit GCP v6,
# deuxieme tour) : le manifeste est RECONSTRUIT depuis les copies
# materialisees (schema + commit + une ligne sha256/taille/chemin par
# fichier, ordre normatif identique au pin) et son SHA-256 doit EGALER le
# digest transmis par la chaine authentifiee du bootstrap.
PROTOCOL_FILES=(
  gcp-migration/session_campagne_v6_g4.sh
  gcp-migration/v6_session_lifecycle.sh
  gcp-migration/v6_campaign_pin.sh
  gcp-migration/v6_campaign_remote.sh
  gcp-migration/validate_v6_campaign.py
  gcp-migration/profils/decision_v1.env
  gcp-migration/profils/smoke_v1.env
  gcp-migration/profils/g4_mesure_v1.env
  gcp-migration/profils/g4_serie_c_v1.env
  gcp-migration/profils/g4_tests_v1.env
  gcp-migration/profils/g4_tests_v2.env
  morsehgp3D_v6/tests/pilote_juge.py
  gcp-migration/set_max_run_duration_and_verify.sh
  gcp-migration/start_and_verify.sh
  gcp-migration/stop_and_verify.sh
  gcp-migration/recover_v6_session.sh
  gcp-migration/profils/g4_echelle_v1.env
)
{
  echo "schema=e-hgp.protocol-manifest.v1"
  echo "commit=${SOURCE_COMMIT}"
  for f in "${PROTOCOL_FILES[@]}"; do
    [ -f "${WORK}/pinned/${f}" ] || { echo "REFUS : copie epinglee absente (${f})" >&2; exit 2; }
    printf '%s\t%s\t%s\n' \
      "$(sha256sum "${WORK}/pinned/${f}" | awk '{print $1}')" \
      "$(wc -c < "${WORK}/pinned/${f}")" \
      "${f}"
  done
} > "${WORK}/manifest_revalide.txt"
REVALIDATED="$(sha256sum "${WORK}/manifest_revalide.txt" | awk '{print $1}')"
if [ "${REVALIDATED}" != "${PROTOCOL_MANIFEST_SHA256}" ]; then
  echo "REFUS : manifeste revalide (${REVALIDATED}) != digest du protocole (${PROTOCOL_MANIFEST_SHA256})" >&2
  exit 2
fi

export GCP_PROJECT_ID="${GCP_PROJECT_ID:-devpod-gpu-exploration}"
export GCP_ZONE="${GCP_ZONE:-europe-west4-a}"
export GCP_INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
# PROFIL CANONIQUE (audit deuxieme tour : le fichier d'autorite est
# versionne et hashe, jamais fabrique par l'environnement). CAMPAIGN_PROFILE
# nomme un profil epingle (decision_v1 par defaut, smoke_v1 pour la fumee) ;
# TOUTE surcharge d'un axe par l'environnement degrade le profil effectif en
# `custom` — un validateur ne peut alors jamais l'appeler decision.
CAMPAIGN_PROFILE="${CAMPAIGN_PROFILE:-decision_v1}"
[[ "${CAMPAIGN_PROFILE}" =~ ^[a-z0-9_]+$ ]] || { echo "REFUS : nom de profil mal forme" >&2; exit 2; }
PROFILE_SRC="${WORK}/pinned/gcp-migration/profils/${CAMPAIGN_PROFILE}.env"
[ -f "${PROFILE_SRC}" ] || { echo "REFUS : profil canonique inconnu (${CAMPAIGN_PROFILE})" >&2; exit 2; }
_ov_CONF_SPECS="${CONF_SPECS:-}"; _ov_BENCH_SPECS="${BENCH_SPECS:-}"
_ov_QUEUE_FAMILIES="${QUEUE_FAMILIES:-}"; _ov_QUEUE_N="${QUEUE_N:-}"
_ov_QUEUE_SEEDS="${QUEUE_SEEDS:-}"; _ov_RUN_TIMEOUT="${RUN_TIMEOUT:-}"
_ov_THREADS_VM="${THREADS_VM:-}"; _ov_V5_GATE_MIN="${V5_GATE_MIN:-}"; _ov_V6_GATE_MIN="${V6_GATE_MIN:-}"
_ov_SWEEP_SPECS="${SWEEP_SPECS:-}"; _ov_SWEEP_REPEATS="${SWEEP_REPEATS:-}"
_ov_GPU_SPECS="${GPU_SPECS:-}"; _ov_FRONTIER_SPECS="${FRONTIER_SPECS:-}"
_ov_FRONTIER_TIMEOUT="${FRONTIER_TIMEOUT:-}"; _ov_GPU_BUILD_TIMEOUT="${GPU_BUILD_TIMEOUT:-}"
_ov_FRONTIER_ULIMIT_KB="${FRONTIER_ULIMIT_KB:-}"; _ov_FRONTIER_LAYOUT="${FRONTIER_LAYOUT:-}"
_ov_MATRICE_POINTS="${MATRICE_POINTS:-}"; _ov_MATRICE_SEQUENCE="${MATRICE_SEQUENCE:-}"
_ov_MATRICE_TIMEOUT="${MATRICE_TIMEOUT:-}"; _ov_ATTRIB_POINTS="${ATTRIB_POINTS:-}"
_ov_ATTRIB_TIMEOUT="${ATTRIB_TIMEOUT:-}"; _ov_GPUV6_GATE_NAMES="${GPUV6_GATE_NAMES:-}"
_ov_GPUV6_BUILD_TIMEOUT="${GPUV6_BUILD_TIMEOUT:-}"; _ov_GPUV6_GATE_TIMEOUT="${GPUV6_GATE_TIMEOUT:-}"
_ov_GPUV6_PILOT_SPECS="${GPUV6_PILOT_SPECS:-}"; _ov_GPUV6_PILOT_MIN_LOTS="${GPUV6_PILOT_MIN_LOTS:-}"
_ov_GPUV6_PILOT_TIMEOUT="${GPUV6_PILOT_TIMEOUT:-}"; _ov_GPUV6_OBJET_DIGESTS="${GPUV6_OBJET_DIGESTS:-}"
_ov_MATRICE_OBJET_DIGESTS="${MATRICE_OBJET_DIGESTS:-}"
_ov_BIN_MATRICE="${BIN_MATRICE:-}"; _ov_BIN_ATTRIB="${BIN_ATTRIB:-}"; _ov_BIN_PILOTE="${BIN_PILOTE:-}"
_ov_SESSION_MAX_RUN_SECONDS="${SESSION_MAX_RUN_SECONDS:-}"; _ov_SESSION_INVITE_MINUTES="${SESSION_INVITE_MINUTES:-}"
# ROUTE DE STOCKAGE du fold pour la FRONTIERE (pin KeyCSR, axe du 2
# septembre) : VIDE = axe non demande, le runner reste alors en plan v1 et
# n'ecrit aucun jeton `--layout` (octets des recus anterieurs). Les profils
# qui ne declarent pas l'axe heritent donc du comportement d'avant l'axe.
FRONTIER_LAYOUT=""
# Valeurs par defaut des NOUVEAUX axes serie C (profils anterieurs sans ces
# cles : sentinelle `aucun` = phases sautees, plans runs=0).
MATRICE_POINTS="aucun"; MATRICE_SEQUENCE="aller retour aller"; MATRICE_TIMEOUT="2400"
ATTRIB_POINTS="aucun"; ATTRIB_TIMEOUT="2400"
GPUV6_GATE_NAMES="aucun"; GPUV6_BUILD_TIMEOUT="1800"; GPUV6_GATE_TIMEOUT="3600"
GPUV6_PILOT_SPECS="aucun"; GPUV6_PILOT_MIN_LOTS="2"; GPUV6_PILOT_TIMEOUT="3600"
GPUV6_OBJET_DIGESTS="aucun"
# Fixture d'egalite de la MATRICE (2 septembre) : famille:n:smax:digest_all,
# liee aux points --digest (cles == points, jamais une fixture decorative).
MATRICE_OBJET_DIGESTS="aucun"
# § 5.18.3 : CHEMINS des binaires executes, lies par le profil (le
# validateur exige l'egalite exacte, jamais un simple basename).
BIN_MATRICE="./build-v6/mhgp6"; BIN_ATTRIB="./build-v6/mhgp6_profile"; BIN_PILOTE="./build-v6-cuda/mhgp6_cuda"
# § 5.13.4 : axes de duree de SESSION — pilotes par le profil, avec les
# anciennes valeurs par defaut pour les profils qui ne les declarent pas.
SESSION_MAX_RUN_SECONDS="28800"; SESSION_INVITE_MINUTES="465"
# shellcheck disable=SC1090
source "${PROFILE_SRC}"
EFFECTIVE_PROFILE="${CAMPAIGN_PROFILE}"
for v in CONF_SPECS BENCH_SPECS QUEUE_FAMILIES QUEUE_N QUEUE_SEEDS RUN_TIMEOUT THREADS_VM V5_GATE_MIN V6_GATE_MIN \
         SWEEP_SPECS SWEEP_REPEATS GPU_SPECS FRONTIER_SPECS FRONTIER_TIMEOUT \
         GPU_BUILD_TIMEOUT FRONTIER_ULIMIT_KB FRONTIER_LAYOUT \
         MATRICE_POINTS MATRICE_SEQUENCE MATRICE_TIMEOUT ATTRIB_POINTS ATTRIB_TIMEOUT \
         GPUV6_GATE_NAMES GPUV6_BUILD_TIMEOUT GPUV6_GATE_TIMEOUT \
         GPUV6_PILOT_SPECS GPUV6_PILOT_MIN_LOTS GPUV6_PILOT_TIMEOUT GPUV6_OBJET_DIGESTS MATRICE_OBJET_DIGESTS \
         BIN_MATRICE BIN_ATTRIB BIN_PILOTE \
         SESSION_MAX_RUN_SECONDS SESSION_INVITE_MINUTES; do
  ov="_ov_${v}"
  if [ -n "${!ov}" ] && [ "${!ov}" != "${!v}" ]; then
    EFFECTIVE_PROFILE="custom"
    printf -v "${v}" '%s' "${!ov}"
  fi
done
_param_re='^[A-Za-z0-9_:, -]*$'
for v in CONF_SPECS BENCH_SPECS QUEUE_FAMILIES QUEUE_N QUEUE_SEEDS RUN_TIMEOUT THREADS_VM \
         SWEEP_SPECS SWEEP_REPEATS GPU_SPECS FRONTIER_SPECS FRONTIER_TIMEOUT \
         GPU_BUILD_TIMEOUT FRONTIER_ULIMIT_KB FRONTIER_LAYOUT \
         MATRICE_POINTS MATRICE_SEQUENCE MATRICE_TIMEOUT ATTRIB_POINTS ATTRIB_TIMEOUT \
         GPUV6_GATE_NAMES GPUV6_BUILD_TIMEOUT GPUV6_GATE_TIMEOUT \
         GPUV6_PILOT_SPECS GPUV6_PILOT_MIN_LOTS GPUV6_PILOT_TIMEOUT GPUV6_OBJET_DIGESTS MATRICE_OBJET_DIGESTS \
         SESSION_MAX_RUN_SECONDS SESSION_INVITE_MINUTES; do
  [[ "${!v}" =~ ${_param_re} ]] || { echo "REFUS : parametre ${v} avec caractere hors alphabet sur" >&2; exit 2; }
done
for v in BIN_MATRICE BIN_ATTRIB BIN_PILOTE; do
  [[ "${!v}" =~ ^\./[A-Za-z0-9_./-]+$ ]] || { echo "REFUS : ${v}='${!v}' hors alphabet (chemin relatif ./… sans espace)" >&2; exit 2; }
done
echo "profil canonique : ${CAMPAIGN_PROFILE} ($(sha256sum "${PROFILE_SRC}" | awk '{print $1}')) — profil effectif : ${EFFECTIVE_PROFILE}"

# § 5.13.4 : les axes de duree du PROFIL pilotent les vrais coupe-circuits
# AVANT tout garde-fou (bloc profil deplace en amont pour cela) ; une
# surcharge directe de MAX_RUN_SECONDS / GUEST_SHUTDOWN_MINUTES par
# l'environnement degrade aussi le profil effectif en `custom`. Le plafond
# 8 h d'AGENTS.md reste verifie par _check_range ci-dessous.
if [ -n "${MAX_RUN_SECONDS:-}" ] && [ "${MAX_RUN_SECONDS}" != "${SESSION_MAX_RUN_SECONDS}" ]; then
  EFFECTIVE_PROFILE="custom"
fi
if [ -n "${GUEST_SHUTDOWN_MINUTES:-}" ] && [ "${GUEST_SHUTDOWN_MINUTES}" != "${SESSION_INVITE_MINUTES}" ]; then
  EFFECTIVE_PROFILE="custom"
fi
MAX_RUN_SECONDS="${MAX_RUN_SECONDS:-${SESSION_MAX_RUN_SECONDS}}"
GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES:-${SESSION_INVITE_MINUTES}}"
# GRACE PROTOCOLAIRE FIXE (huitieme tour) : 30 s, des deux cotes, liee a la
# queue SSH de +60 s apres l'echeance — toute surcharge est REFUSEE (29
# comme 31) pour ne jamais rouvrir implicitement cette relation.
GRACE_S="${GRACE_S:-30}"
[ "${GRACE_S}" = "30" ] || { echo "REFUS : GRACE_S=${GRACE_S} — la grace du protocole est fixee a 30 s" >&2; exit 2; }
DESCRIBE_TIMEOUT_S="${DESCRIBE_TIMEOUT_S:-60}"
VALIDATOR_TIMEOUT_S="${VALIDATOR_TIMEOUT_S:-300}"
SCP_ATTEMPTS="${SCP_ATTEMPTS:-1}"
STOP_RESERVE_S="${STOP_RESERVE_S:-900}"
# Enveloppes DERIVEES (neuvieme tour) : echeance du runner = cutoff effectif
# (min borne GCE / arret invite) moins le budget post-campagne — UNE
# tentative scp a backoff, describes bornes et DEUX reserves d'arret ; la
# validation est locale et HORS budget VM (elle court apres l'arret).
SSH_STEP_TIMEOUT_S="${SSH_STEP_TIMEOUT_S:-3600}"
SCP_STEP_TIMEOUT_S="${SCP_STEP_TIMEOUT_S:-900}"
# VALIDATION CONJOINTE des parametres temporels AVANT toute commande GCP
# (septieme tour : SSH_STEP_TIMEOUT_S=0 desactivait GNU timeout ; une
# surcharge invite courte arretait la VM avant l'echeance supposee).
# Bornes FERMEES des deux cotes (revue en vol du septieme tour : base 10,
# longueur et plafonds — un `08` octal ou un entier enorme sont refuses).
_check_range() { # $1 = nom, $2 = min, $3 = max
  [[ "${!1}" =~ ^[1-9][0-9]{0,5}$ ]] || { echo "REFUS : parametre temporel $1='${!1}' non entier decimal" >&2; exit 2; }
  [ "${!1}" -ge "$2" ] && [ "${!1}" -le "$3" ] || { echo "REFUS : $1=${!1} hors de [$2, $3]" >&2; exit 2; }
}
_check_range MAX_RUN_SECONDS 900 28800
_check_range GUEST_SHUTDOWN_MINUTES 10 480
_check_range DESCRIBE_TIMEOUT_S 10 600
_check_range VALIDATOR_TIMEOUT_S 60 3600
_check_range SCP_ATTEMPTS 1 1
# Reserve d'arret NON REDUCTIBLE : le pire cas de stop_and_verify (lectures,
# mutation, polling, controles finaux) approche 735 s — plancher 900.
_check_range STOP_RESERVE_S 900 3600
_check_range SSH_STEP_TIMEOUT_S 60 7200
_check_range SCP_STEP_TIMEOUT_S 60 3600
# RELATION de la garde de demarrage, testee AVANT set_max (huitieme tour :
# seule la garde suivante la refusait, apres une reconfiguration mutante).
# BUDGET D'ARMEMENT (audit serie C § 5.16 + premier depart brule) : la garde
# invitee exige scheduled <= echeance SURE = start + MAX_RUN - 300 ; le
# shutdown est arme ~5 min apres le demarrage (SSH + OS Login). La relation
# a l'egalite (GUEST*60 + 300 == MAX_RUN) laissait 0 s de marge de boot et a
# brule un depart. Budget minimal certifiable : 480 s (600 s moins la
# tolerance systemd de 120 s, comptee EXPLICITEMENT : § 5.18.1). Predicat :
# GUEST*60 + 300 (reserve GCE) + 120 (tolerance systemd) + 480 (budget) <=
# MAX_RUN. Frontieres gravees au selftest du cycle de vie : 465/466 min
# sous 28800 s, et a la seconde pres 600/599 s avant tolerance (= 480/479 s
# apres) via MAX_RUN 28800/28799.
readonly GCE_RESERVE_S=300
readonly SYSTEMD_TOLERANCE_S=120
readonly MIN_BOOT_BUDGET_S=480
if [ $(( GUEST_SHUTDOWN_MINUTES * 60 + GCE_RESERVE_S + SYSTEMD_TOLERANCE_S + MIN_BOOT_BUDGET_S )) -gt "${MAX_RUN_SECONDS}" ]; then
  echo "REFUS : relation invite/GCE violee (${GUEST_SHUTDOWN_MINUTES} min * 60 + ${GCE_RESERVE_S} s de reserve GCE + ${SYSTEMD_TOLERANCE_S} s de tolerance systemd + ${MIN_BOOT_BUDGET_S} s de budget d'armement > ${MAX_RUN_SECONDS} s) — aucune garde appelee" >&2
  exit 2
fi
# Budget POST-CAMPAGNE reserve au pire cas DECLARE : SCP_ATTEMPTS tentatives
# completes (timeout + grace) + attentes + describes bornes autour des scp
# + validateur borne + arret cible.
SCP_BUDGET_S=$(( SCP_ATTEMPTS * (SCP_STEP_TIMEOUT_S + GRACE_S) + 5 * SCP_ATTEMPTS * (SCP_ATTEMPTS + 1) / 2 ))
# L'arret cible part JUSTE APRES le scp ; la validation est locale et HORS
# du budget VM. Le budget post-campagne reserve DEUX arrets (huitieme tour :
# le chemin echec-puis-reprise en execute deux) — scp, describes, 2 x arret,
# jamais le validateur.
POST_BUDGET_S=$(( SCP_BUDGET_S + 2 * SCP_ATTEMPTS * (DESCRIBE_TIMEOUT_S + GRACE_S) + 2 * STOP_RESERVE_S ))
# Le cutoff est le PLUS PROCHE des deux coupe-circuits (borne GCE dure,
# arret invite) — jamais la seule borne GCE.
EFFECTIVE_CUTOFF_S=$(( GUEST_SHUTDOWN_MINUTES * 60 < MAX_RUN_SECONDS ? GUEST_SHUTDOWN_MINUTES * 60 : MAX_RUN_SECONDS ))
# Echeance du runner derivee du cutoff : campagne (queue du manifeste +60 et
# grace comprises) puis POST_BUDGET doivent tenir avant le cutoff.
RAPATRIEMENT_MARGE_S=$(( POST_BUDGET_S + 60 + GRACE_S + (MAX_RUN_SECONDS - EFFECTIVE_CUTOFF_S) ))
[ $(( EFFECTIVE_CUTOFF_S - POST_BUDGET_S - 60 - GRACE_S )) -ge 900 ] || {
  echo "REFUS : fenetre insuffisante — cutoff effectif ${EFFECTIVE_CUTOFF_S}s, budget post-campagne ${POST_BUDGET_S}s (rien ne serait mesurable)" >&2
  exit 2
}

# REPRISE PERSISTANTE (§ 5.18.6) : WORK vit dans une base 0700 persistante
# (bootstrap) ; tout ecrit de la session est prive (umask 077) ; le
# materiel de reprise (session.env, superviseur.pid, marques/) est publie
# AVANT toute mutation pour qu'un second processus puisse conclure sans
# jamais redemarrer la VM.
umask 077
[ ! -L "${WORK}" ] && [ -d "${WORK}" ] && [ "$(stat -c '%a %u' "${WORK}")" = "700 $(id -u)" ] \
  || { echo "REFUS : WORK (${WORK}) doit etre un repertoire 0700 non symbolique du proprietaire" >&2; exit 2; }
HANDOFF="${WORK}/handoff.json"
STATE_FILE="${WORK}/etat_cycle_vie"
LOG="${WORK}/session.log"
PROFILE="${WORK}/profil_campagne.txt"
MARKS_DIR="${WORK}/marques"
PID_FILE="${WORK}/superviseur.pid"
WITNESS_RC=0  # § 5.21 : temoin non publie => code 68 (domine un succes)
PURGE_RC=0    # § 5.22 : purge des credentials incomplete => code 67 (domine 68 et les succes)
SESSION_ENV="${WORK}/session.env"
: > "${LOG}"
DURABLE_RECEIPT_BASE="${DURABLE_RECEIPT_BASE:?DURABLE_RECEIPT_BASE requis (recu durable obligatoire)}"
DURABLE_RECEIPT_PREFIX="${DURABLE_RECEIPT_PREFIX:?DURABLE_RECEIPT_PREFIX requis}"

# Lecture de l'enregistrement de cycle de vie partage avec le garde.
# state_field n'est utilise QUE pour l'affichage (recu) — toute DECISION du
# cleanup passe par state_snapshot (cinquieme tour : schema, unicite,
# completude, ensemble d'etats autorises).
state_field() { # $1 = cle ; vide si fichier absent
  [ -s "${STATE_FILE}" ] || return 0
  sed -n "s/^$1=//p" "${STATE_FILE}" 2>/dev/null | head -1
}
# state_snapshot : parse STRICTEMENT le registre en un enregistrement unique.
#   sortie « ok <etat> <projet> <zone> <instance> <generation> » et rc=0 ;
#   rc=3 si le fichier est ABSENT (FileNotFoundError SEULEMENT — sixieme
#   tour : une erreur de lecture ne prouve pas l'absence) ; rc=4 s'il est
#   ILLISIBLE (permission, EIO, lien symbolique, objet non regulier, schema
#   faux ou manquant, cle dupliquee ou inconnue, champ manquant, ligne
#   tronquee, etat hors de l'ensemble autorise, alphabet viole). generation
#   peut etre vide (etats anterieurs a la capture) — jamais pour
#   targeted_stopped. Alphabets FERMES sur les champs transportes par mots
#   (le tuple est relu par `read`) : [A-Za-z0-9._:-] seulement.
state_snapshot() {
  python3 - "${STATE_FILE}" <<'PY'
import os
import re
import sys
ALLOWED = {"start_may_have_been_requested", "targeted_running", "targeted_stopping",
           "targeted_stopped", "targeted_stop_failed"}
KEYS = ("schema", "state", "project", "zone", "instance", "generation")
WORD = re.compile(r"^[A-Za-z0-9._:-]+$")
path = sys.argv[1]
if os.path.islink(path):
    sys.exit(4)
try:
    if os.path.exists(path) and not os.path.isfile(path):
        sys.exit(4)
except OSError:
    sys.exit(4)
try:
    with open(path, encoding="utf-8") as fh:
        text = fh.read()
except FileNotFoundError:
    sys.exit(3)
except OSError:
    sys.exit(4)
if not text.endswith("\n"):
    sys.exit(4)  # tronque : la derniere ligne n'est pas terminee
record = {}
for line in text.splitlines():
    if "=" not in line:
        sys.exit(4)
    key, value = line.split("=", 1)
    if key not in KEYS or key in record:
        sys.exit(4)
    record[key] = value
if set(record) != set(KEYS):
    sys.exit(4)
if record["schema"] != "e-hgp.lifecycle-state.v1":
    sys.exit(4)
if record["state"] not in ALLOWED:
    sys.exit(4)
if any(not record[k] for k in ("project", "zone", "instance")):
    sys.exit(4)
if record["state"] == "targeted_stopped" and not record["generation"]:
    sys.exit(4)
for key in ("project", "zone", "instance"):
    if not WORD.match(record[key]):
        sys.exit(4)
if record["generation"] and not WORD.match(record["generation"]):
    sys.exit(4)
print("ok", record["state"], record["project"], record["zone"], record["instance"],
      record["generation"])
PY
}
# PUBLICATION de transition par le cleanup EXTERIEUR (audit troisieme tour :
# le registre doit decrire aussi l'arret nominal) — meme schema et meme
# patron atomique que le garde ; best-effort (jamais bloquant pour l'arret).
lifecycle_publish_state() { # $1 = etat
  python3 - "${STATE_FILE}" "$1" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}" \
    "${GENERATION}" 2>/dev/null <<'PY' || true
import os
import sys
import tempfile
from pathlib import Path

path = Path(sys.argv[1])
state, project, zone, instance, generation = sys.argv[2:7]
if not path.is_absolute() or path.is_symlink():
    sys.exit(1)
data = ("schema=e-hgp.lifecycle-state.v1\n"
        f"state={state}\n"
        f"project={project}\n"
        f"zone={zone}\n"
        f"instance={instance}\n"
        f"generation={generation}\n").encode()
descriptor, temporary = tempfile.mkstemp(prefix=f".{path.name}.", suffix=".partial", dir=str(path.parent))
try:
    offset = 0
    while offset < len(data):
        offset += os.write(descriptor, data[offset:])
    os.fsync(descriptor)
finally:
    os.close(descriptor)
os.replace(temporary, path)
parent = os.open(str(path.parent), os.O_DIRECTORY)
try:
    os.fsync(parent)
finally:
    os.close(parent)
PY
}

log() { printf '%s\n' "$*" | tee -a "${LOG}"; }
log_safe() { { printf '%s\n' "$*" >> "${LOG}"; } 2>/dev/null || true; }

# Lecture STRICTE du handoff (schema, cible exacte, generation non vide).
parse_handoff() {
  python3 - "${HANDOFF}" "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}" <<'PY'
import json, sys
try:
    with open(sys.argv[1], encoding="utf-8") as fh:
        record = json.load(fh)
except Exception:
    sys.exit(1)
if record.get("schema") != "e-hgp.start-handoff.v3":
    sys.exit(1)
if (record.get("project"), record.get("zone"), record.get("instance")) != tuple(sys.argv[2:5]):
    sys.exit(1)
import re
generation = record.get("last_start_timestamp", "")
if not isinstance(generation, str) or not re.match(r"^[A-Za-z0-9._:-]+$", generation):
    sys.exit(1)
print(generation)
PY
}

# ---- ARRET CERTIFIE : trap installe AVANT toute mutation GCP (P0-1).
GENERATION=""
SESSION_RC=0
STOP_ATTEMPTED=0
# Fonction d'arret PARTAGEE (huitieme tour) entre le chemin nominal
# post-scp et le cleanup — publie les etats, tolere un journal en panne,
# compte les tentatives (JAMAIS plus de deux au total).
STOP_TRIES=0
LAST_STOP_RC=1
attempt_targeted_stop() {
  STOP_TRIES=$((STOP_TRIES + 1))
  local so="${LOG}"
  ( : >> "${so}" ) 2>/dev/null || so="$(mktemp 2>/dev/null || echo /dev/null)"
  lifecycle_publish_state "targeted_stopping"
  LAST_STOP_RC=0
  "${GUARDS_DIR}/stop_and_verify.sh" --yes \
    --expected-last-start-timestamp "${GENERATION}" >> "${so}" 2>&1 || LAST_STOP_RC=$?
  if [ "${LAST_STOP_RC}" -eq 0 ]; then
    lifecycle_publish_state "targeted_stopped"
  else
    lifecycle_publish_state "targeted_stop_failed"
  fi
  return 0
}
finalize_receipt() { # $1 = issue, $2 = stop_rc, $3 = rc ; rend 0 ssi le recu COMPLET est publie
  # RUN UNIQUE incluant la generation (audit troisieme tour) : construction
  # dans un temporaire, publication atomique et coherente apres execution,
  # dossier preexistant REFUSE ; `sync` final pour la durabilite apres crash
  # (sixieme tour : sans lui, pas de claim de durabilite).
  local run_id="${DURABLE_RECEIPT_PREFIX}_${GEN_EPOCH:-avorte_$(date +%s)}"
  local dir="${DURABLE_RECEIPT_BASE}/${run_id}"
  local tmp
  {
    [ ! -e "${dir}" ] &&
    mkdir -p "${DURABLE_RECEIPT_BASE}" &&
    tmp="$(mktemp -d "${dir}.partial.XXXXXX")" &&
    {
      printf 'schema=e-hgp.v6-session-receipt.v3\nrun_id=%s\nissue=%s\nrc=%s\nstop_rc=%s\n' \
        "${run_id}" "$1" "$3" "${2:-na}"
      printf 'profil=%s profil_canonique=%s\n' "${EFFECTIVE_PROFILE:-inconnu}" "${CAMPAIGN_PROFILE:-inconnu}"
      printf 'generation=%s\nprojet=%s zone=%s instance=%s\n' "${GENERATION:-inconnue}" \
        "${GCP_PROJECT_ID}" "${GCP_ZONE}" "${GCP_INSTANCE_NAME}"
      printf 'source_commit=%s\nsource_payload_sha256=%s\nprotocol_manifest_sha256=%s\n' \
        "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}"
      printf 'max_run_seconds=%s guest_shutdown_minutes=%s\n' "${MAX_RUN_SECONDS}" "${GUEST_SHUTDOWN_MINUTES}"
      printf 'etat_cycle_vie=%s\n' "$(state_field state)"
      printf 'marques=%s\n' "$(ls "${MARKS_DIR}" 2>/dev/null | tr '\n' ' ')"
      printf 'date_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    } > "${tmp}/RECU_SESSION.txt" &&
    { [ ! -d "${MARKS_DIR}" ] || cp -r "${MARKS_DIR}" "${tmp}/marques"; } &&
    cp -f "${LOG}" "${tmp}/session.log" &&
    { [ ! -f "${PROFILE}" ] || cp -f "${PROFILE}" "${tmp}/profil_campagne.txt"; } &&
    { [ ! -f "${WORK}/validation.txt" ] || cp -f "${WORK}/validation.txt" "${tmp}/validation.txt"; } &&
    { [ ! -f "${WORK}/manifest_revalide.txt" ] || cp -f "${WORK}/manifest_revalide.txt" "${tmp}/"; } &&
    { _res_ok=1
      for _res in bench_resume queue_resume sweep_resume gpu_resume frontier_resume matrice_resume gpuv6_resume; do
        if [ -f "${WORK}/${_res}.txt" ]; then cp -f "${WORK}/${_res}.txt" "${tmp}/" || _res_ok=0; fi
      done
      [ "${_res_ok}" -eq 1 ]; } &&
    { [ ! -d "${WORK}/out" ] || cp -r "${WORK}/out" "${tmp}/out"; } &&
    ( cd "${tmp}" && { find . -type f ! -path ./SHA256SUMS ! -path ./SHA256SUMS.tmp -print0 | sort -z | xargs -0 sha256sum; } > SHA256SUMS.tmp \
      && mv SHA256SUMS.tmp SHA256SUMS \
      && sha256sum -c --quiet SHA256SUMS >/dev/null ) &&
    mv -Tn "${tmp}" "${dir}" &&
    [ ! -e "${tmp}" ] &&
    sync &&
    {
      # Temoin de session CONCLUE (la reprise refuse) seulement si l'arret
      # est certifie au registre ; alors les credentials copies dans le WORK
      # persistant (gcloud, cle privee) sont detruits — le .pub reste.
      # PURGE VERIFIEE d'abord, TEMOIN ensuite (contre-audit reprise, point
      # 6) : un temoin terminal n'est jamais publie au-dessus de credentials
      # encore presents ; en echec, marqueur purge_incomplete et la reprise
      # re-purge localement sans appel GCP.
      if [ "$(state_field state)" = "targeted_stopped" ]; then
        rm -rf "${WORK}/gcloud-config" 2>/dev/null || true
        if [ -f "${WORK}/ssh/id_ed25519" ]; then
          shred -u "${WORK}/ssh/id_ed25519" 2>/dev/null || rm -f "${WORK}/ssh/id_ed25519" 2>/dev/null || true
        fi
        if [ ! -e "${WORK}/gcloud-config" ] && [ ! -e "${WORK}/ssh/id_ed25519" ]; then
          # § 5.21 : l'echec de la publication atomique du temoin DOMINE le
          # code de succes (code local 68), jamais masque par un `true`.
          if ! python3 - "${WORK}/recu_publie" "${dir}" <<'PY'
import os, sys, tempfile
path, content = sys.argv[1:3]
d = os.path.dirname(path)
fd, tmp = tempfile.mkstemp(prefix=".recu_publie.", suffix=".partial", dir=d)
os.write(fd, (content + "\n").encode()); os.fsync(fd); os.close(fd); os.replace(tmp, path)
dfd = os.open(d, os.O_RDONLY); os.fsync(dfd); os.close(dfd)
PY
          then
            WITNESS_RC=68
            printf '%s\n' "temoin recu_publie non publie (mkstemp/rename/fsync) — rejouer la reprise locale : recover_v6_session.sh ${WORK}" > "${WORK}/temoin_non_publie"
            log "TEMOIN NON PUBLIE : purge faite mais recu_publie non ecrit — code 68 ; relancer la reprise (aucun appel GCP)"
          fi
        else
          PURGE_RC=67
          printf '%s\n' "purge incomplete : credentials encore presents (reprise locale : recover_v6_session.sh ${WORK})" > "${WORK}/purge_incomplete"
          log "PURGE INCOMPLETE : credentials encore presents dans ${WORK} — aucun temoin recu_publie, code 67 ; relancer la reprise (purge locale, aucun appel GCP)"
        fi
      fi
      true
    }
  } 2>/dev/null
}
cleanup() {
  local rc=$?
  trap - EXIT HUP INT TERM
  set +e
  if [ "${SESSION_RC}" -ne 0 ]; then rc="${SESSION_RC}"; fi
  log_safe "--- arret certifie (rc=${rc}) ---"
  # GENERATION VERROUILLEE : memoire de session, sinon handoff relu — JAMAIS
  # adoptee du registre (cinquieme tour : le fast-path comparait la
  # generation du registre a elle-meme).
  if [ -z "${GENERATION}" ] && [ -s "${HANDOFF}" ]; then
    GENERATION="$(parse_handoff 2>/dev/null)" || GENERATION=""
  fi
  # SNAPSHOT STRICT du registre : ok / absent (rc=3) / illisible (rc=4).
  local snap="" snap_rc=0
  snap="$(state_snapshot 2>/dev/null)" || snap_rc=$?
  local lc_state="" lc_project="" lc_zone="" lc_instance="" lc_gen=""
  if [ "${snap_rc}" -eq 0 ]; then
    read -r _ lc_state lc_project lc_zone lc_instance lc_gen <<< "${snap}"
  fi
  local receipt_rc=0
  if [ "${snap_rc}" -eq 3 ] && [ -z "${GENERATION}" ]; then
    # Registre ABSENT et aucune generation independante (ni memoire ni
    # handoff) : rien n'atteste un start — refus avant mutation.
    log_safe "aucun enregistrement de cycle de vie ni handoff : refus avant demarrage, aucun arret a certifier"
    finalize_receipt refus_avant_mutation "" "${rc}" || receipt_rc=66
    if [ "${receipt_rc}" -ne 0 ]; then
      echo "[RECU NON PUBLIE] le recu durable n'a pas pu etre ecrit (${DURABLE_RECEIPT_BASE})" >&2
      exit 66
    fi
    exit "${rc}"
  fi
  if [ "${snap_rc}" -eq 4 ] && [ -z "${GENERATION}" ]; then
    # Registre PRESENT mais illisible, aucune generation independante : une
    # mutation est possible et rien ne permet un arret cible — BLOCAGE.
    {
      echo "[BLOCAGE] registre de cycle de vie ILLISIBLE (schema, unicite ou troncature) et generation indisponible — passage de relais requis."
      echo "[BLOCAGE] projet=${GCP_PROJECT_ID} zone=${GCP_ZONE} instance=${GCP_INSTANCE_NAME}"
      echo "[BLOCAGE] enregistrement : ${STATE_FILE}"
      echo "[BLOCAGE] controle : gcloud compute instances describe ${GCP_INSTANCE_NAME} --project=${GCP_PROJECT_ID} --zone=${GCP_ZONE} --format='value(status,lastStartTimestamp)'"
    } >&2
    finalize_receipt blocage_registre_illisible "" 71 || true
    exit 71
  fi
  # Registre perdu ou illisible AVEC handoff valide (cinquieme tour : ne
  # JAMAIS conclure « avant mutation » ici) : l'arret cible est tente plus
  # bas sur la generation verrouillee.
  if [ "${snap_rc}" -ne 0 ] && [ -n "${GENERATION}" ]; then
    log_safe "registre $( [ "${snap_rc}" -eq 3 ] && echo absent || echo illisible ) mais generation verrouillee disponible : arret cible tente"
  fi
  # TERMINAL PARTAGE : fast-path SEULEMENT sur un snapshot STRICT, cible
  # exacte ET generation exacte : egale a la generation verrouillee
  # independante quand elle existe (cinquieme tour : un registre ancien ne
  # peut plus faire sauter l'arret de la generation courante) ; quand AUCUNE
  # source independante n'existe (garde arrete avant le handoff), le registre
  # strict du garde — cree O_EXCL dans NOTRE WORK — reste la certification
  # acquise au deuxieme tour.
  if [ "${snap_rc}" -eq 0 ] && [ "${lc_state}" = "targeted_stopped" ] \
     && { [ -z "${GENERATION}" ] || [ "${lc_gen}" = "${GENERATION}" ]; } \
     && [ "${lc_project}" = "${GCP_PROJECT_ID}" ] \
     && [ "${lc_zone}" = "${GCP_ZONE}" ] \
     && [ "${lc_instance}" = "${GCP_INSTANCE_NAME}" ]; then
    log_safe "arret deja certifie par le garde (generation ${lc_gen}, source $( [ -n "${GENERATION}" ] && echo independante+registre || echo registre-strict-du-garde )) : aucun second arret"
    echo "arret deja certifie par le garde (generation ${lc_gen})"
    # Le recu porte la generation CERTIFIEE et un run_id derive d'elle
    # (sixieme tour : generation=inconnue et run_id avorte sur ce chemin).
    GENERATION="${lc_gen}"
    if [ -z "${GEN_EPOCH:-}" ]; then
      GEN_EPOCH="$(python3 - "${GENERATION}" <<'PYEPOCH'
from datetime import datetime
import sys
try:
    print(int(datetime.fromisoformat(sys.argv[1].replace("Z", "+00:00")).timestamp()))
except Exception:
    raise SystemExit(1)
PYEPOCH
)" || GEN_EPOCH=""
    fi
    finalize_receipt arret_certifie_par_le_garde 0 "${rc}" || {
      echo "[RECU NON PUBLIE] le recu durable n'a pas pu etre ecrit (${DURABLE_RECEIPT_BASE})" >&2
      exit 66
    }
    # § 5.21 : temoin non publie => 68 aussi sur ce chemin (conclusion normale).
    case "${rc}" in 0|65) if [ "${PURGE_RC}" -ne 0 ]; then rc="${PURGE_RC}"; elif [ "${WITNESS_RC}" -ne 0 ]; then rc="${WITNESS_RC}"; fi ;; esac
    exit "${rc}"
  fi
  if [ "${snap_rc}" -eq 0 ] && [ "${lc_state}" = "targeted_stopped" ] \
     && [ -n "${GENERATION}" ] && [ "${lc_gen}" != "${GENERATION}" ]; then
    log_safe "registre targeted_stopped d'une AUTRE generation (${lc_gen} != ${GENERATION}) : fast-path refuse, arret cible sur la generation verrouillee"
  fi
  # ADOPTION POUR L'ARRET SEULEMENT : sans source independante, la generation
  # d'un snapshot STRICT permet encore un arret cible (jamais un fast-path
  # au-dela du cas certifie ci-dessus) — scenario handoff corrompu. La CIBLE
  # DOIT etre exacte (sixieme tour : une generation d'une autre cible
  # autorisait l'appel mutateur) ; cible discordante sans handoff => BLOCAGE
  # et controle manuel, jamais un appel.
  if [ -z "${GENERATION}" ] && [ "${snap_rc}" -eq 0 ] && [ -n "${lc_gen}" ]; then
    if [ "${lc_project}" = "${GCP_PROJECT_ID}" ] && [ "${lc_zone}" = "${GCP_ZONE}" ] \
       && [ "${lc_instance}" = "${GCP_INSTANCE_NAME}" ]; then
      GENERATION="${lc_gen}"
      log_safe "generation adoptee du registre STRICT (cible exacte) pour l'arret cible : ${GENERATION}"
    else
      {
        echo "[BLOCAGE] registre STRICT pour une AUTRE cible (${lc_project}/${lc_zone}/${lc_instance}) — aucun appel mutateur, passage de relais requis."
        echo "[BLOCAGE] cible configuree : ${GCP_PROJECT_ID}/${GCP_ZONE}/${GCP_INSTANCE_NAME}"
        echo "[BLOCAGE] controle : gcloud compute instances describe ${GCP_INSTANCE_NAME} --project=${GCP_PROJECT_ID} --zone=${GCP_ZONE} --format='value(status,lastStartTimestamp)'"
      } >&2
      finalize_receipt blocage_cible_discordante "" 71 || true
      exit 71
    fi
  fi
  if [ -z "${GENERATION}" ]; then
    {
      echo "[BLOCAGE] mutation de demarrage ATTESTEE (etat ${lc_state}) mais generation illisible — passage de relais requis."
      echo "[BLOCAGE] projet=${GCP_PROJECT_ID} zone=${GCP_ZONE} instance=${GCP_INSTANCE_NAME}"
      echo "[BLOCAGE] dernier etat certifie avant mutation : TERMINATED ; enregistrement : ${STATE_FILE}"
      echo "[BLOCAGE] controle : gcloud compute instances describe ${GCP_INSTANCE_NAME} --project=${GCP_PROJECT_ID} --zone=${GCP_ZONE} --format='value(status,lastStartTimestamp)'"
    } >&2
    finalize_receipt blocage_generation_illisible "" 71 || true
    exit 71
  fi
  # REPRISE BORNEE : une tentative d'arret ciblee par le cleanup exterieur —
  # deliberement RE-tentee si l'arret interne du garde a echoue (etat
  # targeted_stop_failed) ; jamais une unicite globale de l'appel d'arret.
  # Le registre est mis a jour par CE cleanup aussi (audit troisieme tour :
  # l'arret nominal doit y figurer) : targeted_stopping avant, puis
  # targeted_stopped / targeted_stop_failed selon le resultat.
  STOP_ATTEMPTED=1
  local stop_rc=0
  if [ "${STOP_TRIES}" -ge 2 ]; then
    # DEUX tentatives deja executees sur le chemin nominal (huitieme tour :
    # le cleanup ne cree jamais une troisieme) — le coupe-circuit GCE demeure.
    log_safe "tentatives d'arret epuisees (${STOP_TRIES}) : aucune troisieme tentative du cleanup"
    stop_rc="${LAST_STOP_RC}"
  else
    # SORTIES PRE-SCP (dixieme tour) : la seconde reserve d'arret existe
    # pour l'echec TRANSITOIRE — le cleanup boucle jusqu'a DEUX appels
    # totaux ; si la garde de demarrage a deja consomme son arret interne
    # (etat targeted_stop_failed a l'entree), UNE seule reprise maintient
    # la borne de deux appels.
    local cleanup_allowance=2
    if [ "${lc_state}" = "targeted_stop_failed" ]; then cleanup_allowance=1; fi
    attempt_targeted_stop
    stop_rc="${LAST_STOP_RC}"
    if [ "${stop_rc}" -ne 0 ] && [ "${cleanup_allowance}" -ge 2 ] && [ "${STOP_TRIES}" -lt 2 ]; then
      log_safe "premier arret du cleanup en echec (rc=${stop_rc}) : seconde reserve employee"
      attempt_targeted_stop
      stop_rc="${LAST_STOP_RC}"
    fi
  fi
  log_safe "stop_and_verify (generation ${GENERATION}) : rc=${stop_rc}"
  echo "journal complet : ${LOG}"
  echo "resultats rapatries : ${WORK}/out (si l'etape scp a ete atteinte)"
  # TUPLE COMPLET post-arret (septieme tour : le seul deuxieme champ etait
  # compare — un registre etranger syntaxiquement strict passait). Toute
  # discordance est une INCOHERENCE DE PREUVE, distincte d'un recu non
  # publie, gravee sous sa propre issue.
  local snap2="" incoherent=0
  snap2="$(state_snapshot 2>/dev/null)" || snap2=""
  if [ "${stop_rc}" -eq 0 ] && \
     [ "${snap2}" != "ok targeted_stopped ${GCP_PROJECT_ID} ${GCP_ZONE} ${GCP_INSTANCE_NAME} ${GENERATION}" ]; then
    echo "[INCOHERENCE] arret certifie mais le registre (snapshot strict) ne porte pas le tuple attendu (${snap2:-illisible})" >&2
    incoherent=1
  fi
  if [ "${incoherent}" -eq 1 ]; then
    finalize_receipt incoherence_registre_post_arret "${stop_rc}" "${rc}" || receipt_rc=66
    if [ "${receipt_rc}" -ne 0 ]; then
      echo "[RECU NON PUBLIE] le recu durable n'a pas pu etre ecrit (${DURABLE_RECEIPT_BASE})" >&2
      exit 66
    fi
    echo "[INCOHERENCE] recu grave sous issue=incoherence_registre_post_arret — preuve corrompue, arret certifie" >&2
    exit 78
  fi
  finalize_receipt arret_tente "${stop_rc}" "${rc}" || receipt_rc=66
  if [ "${stop_rc}" -ne 0 ]; then
    echo "[ARRET NON CERTIFIE] stop_and_verify a rendu ${stop_rc} — echec bloquant" >&2
    exit 70
  fi
  if [ "${receipt_rc}" -ne 0 ]; then
    echo "[RECU NON PUBLIE] arret certifie mais recu durable manquant (${DURABLE_RECEIPT_BASE})" >&2
    exit 66
  fi
  # § 5.21 : un temoin non publie DOMINE une conclusion normale (0 ou le
  # verdict non decisionnel 65) ; un echec deja signale garde son code.
  case "${rc}" in 0|65) if [ "${PURGE_RC}" -ne 0 ]; then rc="${PURGE_RC}"; elif [ "${WITNESS_RC}" -ne 0 ]; then rc="${WITNESS_RC}"; fi ;; esac
  exit "${rc}"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

# ---- MATERIEL DE REPRISE (§ 5.18.6), AVANT toute commande gcloud mutante :
# session.env (cible, pin, budgets, chemins — alphabet strict, parse par un
# lecteur dedie, jamais `source`), superviseur.pid (pid + starttime +
# boot_id : un pid recycle ne passe pas pour vivant), marques/ 0700.
publish_session_env() {
  local tmp="${SESSION_ENV}.tmp.$$"
  {
    echo "schema=e-hgp.session-env.v1"
    echo "REPO_ROOT=${MHGP6_BOOTSTRAP_REPO_ROOT:-${PWD}}"
    echo "GCP_PROJECT_ID=${GCP_PROJECT_ID}"
    echo "GCP_ZONE=${GCP_ZONE}"
    echo "GCP_INSTANCE_NAME=${GCP_INSTANCE_NAME}"
    echo "GUARDS_DIR=${GUARDS_DIR}"
    echo "SOURCE_COMMIT=${SOURCE_COMMIT}"
    echo "SOURCE_PAYLOAD_SHA256=${SOURCE_PAYLOAD_SHA256}"
    echo "PROTOCOL_MANIFEST_SHA256=${PROTOCOL_MANIFEST_SHA256}"
    echo "DURABLE_RECEIPT_BASE=${DURABLE_RECEIPT_BASE}"
    echo "DURABLE_RECEIPT_PREFIX=${DURABLE_RECEIPT_PREFIX}"
    echo "MAX_RUN_SECONDS=${MAX_RUN_SECONDS}"
    echo "GUEST_SHUTDOWN_MINUTES=${GUEST_SHUTDOWN_MINUTES}"
    echo "EFFECTIVE_CUTOFF_S=${EFFECTIVE_CUTOFF_S}"
    echo "GRACE_S=${GRACE_S}"
    echo "SCP_STEP_TIMEOUT_S=${SCP_STEP_TIMEOUT_S}"
    echo "DESCRIBE_TIMEOUT_S=${DESCRIBE_TIMEOUT_S}"
    echo "STOP_RESERVE_S=${STOP_RESERVE_S}"
    echo "VALIDATOR_TIMEOUT_S=${VALIDATOR_TIMEOUT_S}"
    echo "CAMPAIGN_PROFILE=${CAMPAIGN_PROFILE}"
    echo "EFFECTIVE_PROFILE=${EFFECTIVE_PROFILE}"
    echo "GCP_SSH_KEY_EXPIRATION_UTC=${GCP_SSH_KEY_EXPIRATION_UTC:-}"
    echo "REMOTE_DIR=${REMOTE_DIR:-}"
  } > "${tmp}" && sync -f "${tmp}" 2>/dev/null; mv -f "${tmp}" "${SESSION_ENV}"
}
mkdir -m 0700 -p "${MARKS_DIR}"
publish_session_env
printf '%s %s %s %s %s\n' "$$" "$(awk '{print $22}' /proc/$$/stat)" "$(cat /proc/sys/kernel/random/boot_id)" \
  "$(ps -o sid= -p "$$" | tr -d ' ')" "$(ps -o pgid= -p "$$" | tr -d ' ')" > "${PID_FILE}"

# CONFIGURATION GCLOUD PRIVEE a la session (audit deuxieme tour : aucune
# mutation de la configuration partagee) — copie de la configuration
# existante (credentials compris) dans le WORK, puis toute commande gcloud
# de cette session n'ecrit que dans cette copie ; --project/--zone restent
# explicites sur chaque commande.
# TOUJOURS prive (audit troisieme tour) : le repertoire est cree dans tous
# les cas ; la configuration source (CLOUDSDK_CONFIG herite, sinon la
# configuration par defaut) y est copiee si elle existe — credentials
# compris — puis seule la copie est exportee.
_gcloud_src="${CLOUDSDK_CONFIG:-${HOME}/.config/gcloud}"
mkdir -p "${WORK}/gcloud-config"
if [ -d "${_gcloud_src}" ]; then cp -r "${_gcloud_src}/." "${WORK}/gcloud-config/"; fi
export CLOUDSDK_CONFIG="${WORK}/gcloud-config"
gcloud config set project "${GCP_PROJECT_ID}" >/dev/null

# ---- PROFIL DE CAMPAGNE EPINGLE AVANT TOUTE MUTATION (P1 : la matrice est
# fixee independamment des sorties que le validateur jugera ; le nom
# effectif degrade en `custom` des qu'un axe canonique est surcharge).
{
  echo "profil=${EFFECTIVE_PROFILE}"
  echo "profil_canonique=${CAMPAIGN_PROFILE}"
  echo "profil_canonique_sha256=$(sha256sum "${PROFILE_SRC}" | awk '{print $1}')"
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
  # ROUTE DE STOCKAGE DE LA FRONTIERE : gravee MEME VIDE — un profil qui ne
  # demande pas l'axe le dit, et le validateur peut alors distinguer « axe
  # non demande » de « axe perdu en chemin ».
  echo "frontier_layout=${FRONTIER_LAYOUT}"
  echo "matrice_points=${MATRICE_POINTS}"
  echo "matrice_sequence=${MATRICE_SEQUENCE}"
  echo "matrice_timeout=${MATRICE_TIMEOUT}"
  echo "attrib_points=${ATTRIB_POINTS}"
  echo "attrib_timeout=${ATTRIB_TIMEOUT}"
  echo "gpuv6_gate_names=${GPUV6_GATE_NAMES}"
  echo "gpuv6_build_timeout=${GPUV6_BUILD_TIMEOUT}"
  echo "gpuv6_gate_timeout=${GPUV6_GATE_TIMEOUT}"
  echo "gpuv6_pilot_specs=${GPUV6_PILOT_SPECS}"
  echo "gpuv6_pilot_min_lots=${GPUV6_PILOT_MIN_LOTS}"
  echo "gpuv6_pilot_timeout=${GPUV6_PILOT_TIMEOUT}"
  echo "gpuv6_objet_digests=${GPUV6_OBJET_DIGESTS}"
  echo "matrice_objet_digests=${MATRICE_OBJET_DIGESTS}"
  echo "bin_matrice=${BIN_MATRICE}"
  echo "bin_attrib=${BIN_ATTRIB}"
  echo "bin_pilote=${BIN_PILOTE}"
  echo "session_max_run_seconds=${SESSION_MAX_RUN_SECONDS}"
  echo "session_invite_minutes=${SESSION_INVITE_MINUTES}"
  echo "max_run_seconds_effectif=${MAX_RUN_SECONDS}"
  echo "guest_shutdown_minutes_effectif=${GUEST_SHUTDOWN_MINUTES}"
} > "${PROFILE}.tmp"
mv "${PROFILE}.tmp" "${PROFILE}"
log "profil de campagne epingle : $(sha256sum "${PROFILE}" | awk '{print $1}')"

# ---- PREFLIGHT BUDGETAIRE (P1) : PREVISION EMPIRIQUE declaree ici, jamais
# ajustee apres mesure — PAS une garantie que tous les runs terminent (les
# bornes dures par run sont les plafonds du profil ; la campagne est
# EXPLORATOIRE TRONQUABLE : toute troncature a l'echeance est gravee et le
# validateur juge partial). Les builds GPU sont credites A LEURS PLAFONDS
# (credit == plafond, sixieme tour) et chaque run porte un petit overhead.
# Refus AVANT toute mutation si la prevision depasse la fenetre utile.
budget_estimate() {
  python3 - "${CONF_SPECS}" "${BENCH_SPECS}" "${QUEUE_FAMILIES}" "${QUEUE_N}" "${QUEUE_SEEDS}" \
    "${SWEEP_SPECS}" "${SWEEP_REPEATS}" "${GPU_SPECS}" "${FRONTIER_SPECS}" \
    "${GPU_BUILD_TIMEOUT}" "${FRONTIER_TIMEOUT}" \
    "${MATRICE_POINTS}" "${MATRICE_SEQUENCE}" "${ATTRIB_POINTS}" \
    "${GPUV6_GATE_NAMES}" "${GPUV6_PILOT_SPECS}" "${GPUV6_BUILD_TIMEOUT}" "${GPUV6_GATE_TIMEOUT}" <<'PY'
import sys
conf = [x for x in sys.argv[1].split() if x != "aucun"]
bench = [x for x in sys.argv[2].split() if x != "aucun"]
queue_f = [x for x in sys.argv[3].split() if x != "aucun"]
queue_n, queue_s = sys.argv[4].split(), sys.argv[5].split()
sweep = [x for x in sys.argv[6].split() if x != "aucun"]
sweep_reps = int(sys.argv[7]) if sys.argv[7].isdigit() else 0
gpu = [x for x in sys.argv[8].split() if x != "aucun"]
frontier = [x for x in sys.argv[9].split() if x != "aucun"]
# secondes par run, VM 48 fils, DECLARE AVANT MESURE (recus v5 G4 a 200k
# ~260s x marge 1,5-2 ; jamais ajuste apres coup) :
conf_ref = {8000: 30, 32000: 60, 50000: 100, 100000: 200, 200000: 450}
bench_ref = {8000: 40, 32000: 80, 50000: 120, 100000: 220, 200000: 480}
queue_ref = {16000: 60, 64000: 150, 128000: 350, 256000: 800}
# FILS : base a 48 fils, cout multiplie par (48/fils)**0.9 (borne : le
# parallelisme ne peut pas faire mieux que lineaire ; l'exposant < 1 encode
# la part sequentielle deja payee). Conservateur, jamais ajuste apres coup.
sweep_base48 = {8000: 15, 16000: 30, 32000: 60, 50000: 100}
# GPU : les deux builds au PLAFOND canonique (credit == plafond) + mutant
# 60 s ; par famille 4 contrats a ~1,2x le cout conf (digest paye partout).
gpu_build_cap = int(sys.argv[10])
frontier_cap = int(sys.argv[11])
gpu_fixed, gpu_ref = 2 * gpu_build_cap + 60, {32000: 80, 50000: 130, 100000: 260}
# FRONTIERE : chaque taille inconnue est creditee au PLAFOND ; 800000 est
# credite au plafond (l'issue attendue est un refus de capacite ou un
# timeout). La table est calibree a K=10 (smax=11) : un spec famille:n:smax
# hors 11 n'y a AUCUN temoin (l'objet calcule est un prefixe, son cout est
# inconnu avant mesure) — il est credite au PLAFOND, jamais a une reference
# d'une autre echelle.
frontier_ref = {200000: 700, 400000: 1700}
# SERIE C (§ 5.12), DECLARE AVANT MESURE : matrice CPU au modele sweep
# (base 48 fils x (48/fils)**0.9, digest `avec` x1,25), un run par point et
# par passage de la sequence ; attribution au binaire instrumente creditee
# 2x le point non instrumente ; build CUDA et phase de portes credites A
# LEURS PLAFONDS (credit == plafond) ; pilote par famille : echauffement +
# 4 repetitions, chaque repetition = route CPU (conf_ref) + route device
# creditee 0,5x la route CPU (transferts domines par H2D, jamais un mur).
matrice = [x for x in sys.argv[12].split() if x != "aucun"]
matrice_passages = max(1, len(sys.argv[13].split()))
attrib = [x for x in sys.argv[14].split() if x != "aucun"]
gpuv6_gates = [x for x in sys.argv[15].split() if x != "aucun"]
gpuv6_pilot = [x for x in sys.argv[16].split() if x != "aucun"]
gpuv6_build_cap, gpuv6_gate_cap = int(sys.argv[17]), int(sys.argv[18])
def matrice_point_cost(spec):
    parts = spec.split(":")
    n, fils = int(parts[1]), int(parts[2])
    digest = len(parts) > 5 and parts[5] == "avec"
    base = sweep_base48.get(n, 900)
    c = base * (48.0 / max(1, fils)) ** 0.9
    return int(c * (1.25 if digest else 1.0) + 1)
OVERHEAD_PER_RUN = 10
total = 0
runs = 0
for spec in conf:
    n = int(spec.split(":")[1])
    total += 2 * conf_ref.get(n, 900)  # reference v5 + juge v6
    runs += 2
for spec in bench:
    n = int(spec.split(":")[1])
    total += 4 * bench_ref.get(n, 900)  # ABBA : quatre runs par paire
    runs += 4
for n in queue_n:
    total += len(queue_f) * len(queue_s) * queue_ref.get(int(n), 1800)
    runs += len(queue_f) * len(queue_s)
for spec in sweep:
    fam, n, tl = spec.split(":", 2)
    base = sweep_base48.get(int(n), 900)
    for t in tl.split(","):
        total += sweep_reps * int(base * (48.0 / max(1, int(t))) ** 0.9 + 1)
        runs += sweep_reps
if gpu:
    total += gpu_fixed
    runs += 3
    for spec in gpu:
        n = int(spec.split(":")[1])
        total += 4 * gpu_ref.get(n, 900)  # cpu + dev + ad + idx
        runs += 4
for spec in frontier:
    parts = spec.split(":")
    n = int(parts[1])
    smax = parts[2] if len(parts) > 2 else "11"
    total += frontier_ref.get(n, frontier_cap) if smax == "11" else frontier_cap
    runs += 1
for spec in matrice:
    total += matrice_passages * matrice_point_cost(spec)
    runs += matrice_passages
for spec in attrib:
    total += 2 * matrice_point_cost(spec)
    runs += 1
if gpuv6_gates or gpuv6_pilot:
    total += gpuv6_build_cap + gpuv6_gate_cap  # credit == plafond
    runs += 2
for spec in gpuv6_pilot:
    n = int(spec.split(":")[1])
    # § 5.14.2 : la route device est creditee >= 1,0x la route CPU AVANT
    # mesure (son gain est precisement inconnu) — jamais un coefficient
    # abaisse pour faire tenir une fenetre.
    total += 5 * 2 * conf_ref.get(n, 900)  # echauffement + 4 reps, cpu + 1,0x device
    runs += 1
print(total + OVERHEAD_PER_RUN * runs)
PY
}
BUILD_ESTIMATE_S=900
ESTIMATE_S="$(budget_estimate)"
WINDOW_S=$((MAX_RUN_SECONDS - RAPATRIEMENT_MARGE_S - BUILD_ESTIMATE_S))
log "preflight budgetaire : ESTIMATION NOMINALE ${ESTIMATE_S}s (+build ${BUILD_ESTIMATE_S}s) pour une fenetre de ${WINDOW_S}s — campagne exploratoire tronquable"
# ENVELOPPE DE TERMINAISON publiee separement (sixieme tour) : somme des
# PLAFONDS par run — elle peut exceder la fenetre ; ce sont les gardes
# d'echeance du runner qui bornent la session, pas cette somme.
count_runs() { python3 - "${CONF_SPECS}" "${BENCH_SPECS}" "${QUEUE_FAMILIES}" "${QUEUE_N}" "${QUEUE_SEEDS}" "${SWEEP_SPECS}" "${SWEEP_REPEATS}" "${GPU_SPECS}" "${FRONTIER_SPECS}" \
  "${MATRICE_POINTS}" "${MATRICE_SEQUENCE}" "${ATTRIB_POINTS}" "${GPUV6_GATE_NAMES}" "${GPUV6_PILOT_SPECS}" <<'PY'
import sys
def ax(t): return [x for x in t.split() if x != "aucun"]
conf, bench, qf = ax(sys.argv[1]), ax(sys.argv[2]), ax(sys.argv[3])
qn, qs, sweep = sys.argv[4].split(), sys.argv[5].split(), ax(sys.argv[6])
reps, gpu, front = int(sys.argv[7]), ax(sys.argv[8]), ax(sys.argv[9])
matrice, attrib = ax(sys.argv[10]), ax(sys.argv[12])
passages = max(1, len(sys.argv[11].split()))
gpuv6_on = 1 if (ax(sys.argv[13]) or ax(sys.argv[14])) else 0
cpu_runs = 2 * len(conf) + 4 * len(bench) + len(qf) * len(qn) * len(qs)     + sum(reps * len(sp.split(":", 2)[2].split(",")) for sp in sweep) + 4 * len(gpu)
print(cpu_runs, len(front), 1 if gpu else 0,
      passages * len(matrice), len(attrib), gpuv6_on, len(ax(sys.argv[14])))
PY
}
read -r _CPU_RUNS _FRONT_RUNS _GPU_ON _MATRICE_RUNS _ATTRIB_RUNS _GPUV6_ON _GPUV6_PILOTS <<< "$(count_runs)"
ENVELOPE_S=$(( _CPU_RUNS * RUN_TIMEOUT + _FRONT_RUNS * FRONTIER_TIMEOUT + _GPU_ON * (2 * GPU_BUILD_TIMEOUT + RUN_TIMEOUT) \
  + _MATRICE_RUNS * MATRICE_TIMEOUT + _ATTRIB_RUNS * ATTRIB_TIMEOUT \
  + _GPUV6_ON * (GPUV6_BUILD_TIMEOUT + GPUV6_GATE_TIMEOUT) + _GPUV6_PILOTS * GPUV6_PILOT_TIMEOUT ))
log "enveloppe de terminaison (somme des plafonds par run) : ${ENVELOPE_S}s — bornee en pratique par l'echeance du runner et les coupe-circuits, jamais une promesse de completude"
if [ "${ESTIMATE_S}" -gt "${WINDOW_S}" ]; then
  echo "REFUS : la matrice declaree (${ESTIMATE_S}s estimes) ne tient pas dans la fenetre (${WINDOW_S}s) — reduire la matrice ou augmenter MAX_RUN_SECONDS" >&2
  exit 2
fi

# ---- 1. Borne de duree persistee sur l'instance ARRETEE (garde epinglee).
"${GUARDS_DIR}/set_max_run_duration_and_verify.sh" --yes \
  --max-run-duration-seconds "${MAX_RUN_SECONDS}" 2>&1 | tee -a "${LOG}"

# ---- 2. Cle OS Login ephemere, TTL borne par la duree de la VM.
export GCP_SSH_KEY_DIR="${WORK}/ssh"
mkdir -p "${GCP_SSH_KEY_DIR}"
chmod 700 "${GCP_SSH_KEY_DIR}"
export GCP_SSH_KEY_FILE="${GCP_SSH_KEY_DIR}/id_ed25519"
ssh-keygen -q -t ed25519 -N '' -C 'e-hgp-v6-session' -f "${GCP_SSH_KEY_FILE}"
chmod 600 "${GCP_SSH_KEY_FILE}"
# TTL OS Login DANS la fenetre acceptee par le preflight du garde (audit
# deuxieme tour : le garde refuse une duree restante > MAX_RUN_SECONDS +
# 660 s ; +600 s laisse 60 s de marge). Garde arithmetique locale : un TTL
# hors fenetre est refuse ICI, avant toute mutation.
SSH_TTL_MIN=$(((MAX_RUN_SECONDS + 600) / 60))
if [ $((SSH_TTL_MIN * 60)) -gt $((MAX_RUN_SECONDS + 660)) ]; then
  echo "REFUS : TTL OS Login (${SSH_TTL_MIN} min) hors de la fenetre du preflight (MAX_RUN_SECONDS + 660 s)" >&2
  exit 2
fi
GCP_SSH_KEY_EXPIRATION_UTC="$(python3 -c "from datetime import datetime,timedelta,timezone; print((datetime.now(timezone.utc)+timedelta(minutes=${SSH_TTL_MIN})).isoformat(timespec='seconds').replace('+00:00','Z'))")"
export GCP_SSH_KEY_EXPIRATION_UTC
gcloud compute os-login ssh-keys add --key-file="${GCP_SSH_KEY_FILE}.pub" --ttl="${SSH_TTL_MIN}m" \
  --project="${GCP_PROJECT_ID}" >/dev/null
publish_session_env

# ---- 3. Demarrage garde (epingle), temoin de mutation durable, handoff
# atomique. Le retour est CAPTURE : quel qu'il soit, cleanup decide via la
# table temoin/handoff.
START_RC=0
set +e
"${GUARDS_DIR}/start_and_verify.sh" --yes \
  --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}" \
  --handoff-file "${HANDOFF}" \
  --lifecycle-state-file "${STATE_FILE}" \
  --guard-mark-dir "${MARKS_DIR}" 2>&1 | tee -a "${LOG}"
START_RC=${PIPESTATUS[0]}
set -e
if [ "${START_RC}" -ne 0 ]; then
  SESSION_RC="${START_RC}"
  exit "${START_RC}"
fi
GENERATION="$(parse_handoff)" || { SESSION_RC=72; exit 72; }
log "generation verrouillee : ${GENERATION}"

# ---- Echeance derivee du lastStartTimestamp CERTIFIE (P1), jamais de
# l'horloge locale au moment du demarrage.
GEN_EPOCH="$(python3 - "${GENERATION}" <<'PY'
from datetime import datetime
import sys
value = sys.argv[1]
if value.endswith("Z"):
    value = value[:-1] + "+00:00"
print(int(datetime.fromisoformat(value).timestamp()))
PY
)" || { SESSION_RC=73; exit 73; }
DEADLINE_EPOCH=$((GEN_EPOCH + MAX_RUN_SECONDS - RAPATRIEMENT_MARGE_S))
log "deadline_epoch=${DEADLINE_EPOCH} (generation ${GEN_EPOCH} + ${MAX_RUN_SECONDS} - ${RAPATRIEMENT_MARGE_S})"

# Controle de generation NON MUTANT (describe) : ferme la fenetre entre le
# start certifie et le premier SSH.
# too_late BESOIN : vrai si l'operation de BESOIN secondes ne tient plus
# avant l'echeance (grace comprise) — /usr/bin/date epingle, `now` lu ici.
too_late() {
  local _now
  _now=$(/usr/bin/date +%s)
  [ $(( DEADLINE_EPOCH - _now - GRACE_S )) -lt "$1" ]
}
check_generation() {
  # Describe BORNE (septieme tour : un controle GCP bloque ne doit jamais
  # empecher l'arret cible d'etre atteint).
  local seen
  seen="$(/usr/bin/timeout -k "${GRACE_S}" "${DESCRIBE_TIMEOUT_S}" \
          gcloud compute instances describe "${GCP_INSTANCE_NAME}" --project="${GCP_PROJECT_ID}" \
          --zone="${GCP_ZONE}" --format='value(lastStartTimestamp)' 2>>"${LOG}")" || return 1
  [ "${seen}" = "${GENERATION}" ]
}
if too_late "${DESCRIBE_TIMEOUT_S}"; then
  log "REFUS : echeance atteinte avant la premiere operation — AUCUN SSH/SCP, arret direct"
  SESSION_RC=77
  exit 77
fi
check_generation || { log "REFUS : generation changee avant SSH"; SESSION_RC=74; exit 74; }

SSH=(gcloud compute ssh "${GCP_INSTANCE_NAME}" --project="${GCP_PROJECT_ID}"
     --zone="${GCP_ZONE}" --ssh-key-file="${GCP_SSH_KEY_FILE}"
     --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}" --quiet --command)
SCP=(gcloud compute scp --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}"
     --quiet --ssh-key-file="${GCP_SSH_KEY_FILE}"
     --ssh-key-expiration="${GCP_SSH_KEY_EXPIRATION_UTC}")

# Chemins distants propres a (pin, generation) — jamais reutilises (P1).
REMOTE_TAG="${SOURCE_COMMIT:0:12}_${GEN_EPOCH}"
REMOTE_DIR="v6camp_${REMOTE_TAG}"
publish_session_env  # le repertoire distant devient connu de la reprise
REMOTE_BUNDLE="/tmp/v6bundle_${REMOTE_TAG}.tgz"

# ---- 4a. HANDSHAKE NON MUTANT (audit deuxieme tour) : un premier SSH qui
# ne fait QUE lire boot_id, encadre par DEUX controles de generation ; toute
# commande distante ulterieure REVERIFIE ce boot_id avant de muter quoi que
# ce soit sur la VM.
if too_late "${SCP_STEP_TIMEOUT_S}"; then
  log "REFUS : echeance atteinte avant le handshake — AUCUN SSH/SCP, arret direct"
  SESSION_RC=77
  exit 77
fi
BOOT_ID="$(/usr/bin/timeout -k "${GRACE_S}" "${SCP_STEP_TIMEOUT_S}" "${SSH[@]}" 'cat /proc/sys/kernel/random/boot_id' 2>>"${LOG}")" \
  || { log "REFUS : handshake boot_id impossible"; SESSION_RC=75; exit 75; }
[[ "${BOOT_ID}" =~ ^[0-9a-f-]{36}$ ]] || { log "REFUS : boot_id mal forme (${BOOT_ID})"; SESSION_RC=75; exit 75; }
if too_late "${DESCRIBE_TIMEOUT_S}"; then
  log "REFUS : echeance atteinte apres le handshake — arret direct"
  SESSION_RC=77
  exit 77
fi
check_generation || { log "REFUS : generation changee pendant le handshake"; SESSION_RC=74; exit 74; }
log "handshake : boot_id=${BOOT_ID} generation confirmee"

# ---- 4b. Envoi du BUNDLE pinne (lecture seule cote VM), encadre par deux
# controles de generation.
if too_late "${SCP_STEP_TIMEOUT_S}"; then
  log "REFUS : echeance atteinte avant l'envoi du bundle — arret direct"
  SESSION_RC=77
  exit 77
fi
/usr/bin/timeout -k "${GRACE_S}" "${SCP_STEP_TIMEOUT_S}" "${SCP[@]}" "${BUNDLE}" \
  "${GCP_INSTANCE_NAME}:${REMOTE_BUNDLE}.recu" 2>&1 | tee -a "${LOG}"
if too_late "${DESCRIBE_TIMEOUT_S}"; then
  log "REFUS : echeance atteinte apres l'envoi du bundle — arret direct"
  SESSION_RC=77
  exit 77
fi
check_generation || { log "REFUS : generation changee apres l'envoi du bundle"; SESSION_RC=74; exit 74; }

# ---- 5. Build v6 (+ v5 SEULEMENT si une phase l'exerce, § 5.14.1),
# preconditions, REJEU des portes avec JOURNAL COMPLET et planchers (P1 :
# jamais un tail -4). Le boot_id du handshake est REVERIFIE dans la meme
# commande distante avant toute mutation. La v5 est une lignee seulement
# historique : la session serie C bornee a la v6 ne la repaye pas.
V5_NEEDED=0
for _ax in "${CONF_SPECS}" "${BENCH_SPECS}" "${GPU_SPECS}"; do
  for _tok in ${_ax}; do
    [ "${_tok}" != "aucun" ] && V5_NEEDED=1
  done
done
if [ "${V5_NEEDED}" -eq 1 ]; then
  V5_PREFLIGHT_CMDS='cmake -S morsehgp3D_v5 -B build-v5 -DCMAKE_BUILD_TYPE=Release
  cmake --build build-v5 -j48 2>&1 | tail -3
  ctest --test-dir build-v5 -L gate -j24 --output-on-failure'
else
  V5_PREFLIGHT_CMDS='echo "preflight v5 : saute (aucune phase conf/bench/gpu-v5)"'
fi
BUILD_LOG="${WORK}/build_vm.log"
_now=$(date +%s)
BUILD_TIMEOUT_S=$(( DEADLINE_EPOCH - _now - GRACE_S ))
if [ "${BUILD_TIMEOUT_S}" -gt "${SSH_STEP_TIMEOUT_S}" ]; then BUILD_TIMEOUT_S="${SSH_STEP_TIMEOUT_S}"; fi
if [ "${BUILD_TIMEOUT_S}" -lt 60 ]; then
  log "REFUS : temps restant insuffisant pour le build (${BUILD_TIMEOUT_S}s) — arret sans campagne"
  SESSION_RC=77
  exit 77
fi
/usr/bin/timeout -k "${GRACE_S}" "${BUILD_TIMEOUT_S}" "${SSH[@]}" 'set -euo pipefail
  test "$(cat /proc/sys/kernel/random/boot_id)" = '"'${BOOT_ID}'"' || { echo "REFUS : boot_id different (redemarrage detecte)" >&2; exit 9; }
  mv '"${REMOTE_BUNDLE}.recu"' '"${REMOTE_BUNDLE}"'
  export PATH=$HOME/.local/bin:$PATH
  test -x /usr/bin/time || { echo "REFUS : GNU time absent de la VM" >&2; exit 2; }
  python3 -m pip install --user --quiet --upgrade cmake >/dev/null 2>&1 || true
  export PATH=$HOME/.local/bin:$PATH
  rm -rf ~/'"${REMOTE_DIR}"' && mkdir -p ~/'"${REMOTE_DIR}"' && cd ~/'"${REMOTE_DIR}"'
  echo "'"${SOURCE_PAYLOAD_SHA256}"'  '"${REMOTE_BUNDLE}"'" | sha256sum -c -
  tar xzf '"${REMOTE_BUNDLE}"'
  echo "coeurs=$(nproc)"; grep MemTotal /proc/meminfo; cmake --version | head -1
  '"${V5_PREFLIGHT_CMDS}"'
  cmake -S morsehgp3D_v6 -B build-v6 -DCMAKE_BUILD_TYPE=Release
  cmake --build build-v6 -j48 2>&1 | tail -3
  ctest --test-dir build-v6 -L gate -j24 --output-on-failure
' > "${BUILD_LOG}" 2>&1 || { SESSION_RC=$?; log "build/portes VM en echec (rc=${SESSION_RC}, journal ${BUILD_LOG})"; exit "${SESSION_RC}"; }
cat "${BUILD_LOG}" >> "${LOG}"
# Planchers de portes : un bloc 100% par lignee EXERCEE, totaux >= planchers
# (P1 ; § 5.14.1 : v5 conditionnelle, jamais un masquage de plancher — le
# bloc v5 disparait ENTIEREMENT quand aucune phase ne l'exerce).
# Les DEUX libelles de resume CTest sont acceptes (ctest <= 4.3 :
# « 100% tests passed, 0 tests failed out of N » ; ctest 4.4+ :
# « 100% tests passed out of N ») — la session du 1er septembre a ete
# refusee fail-closed sur ce seul changement de format, portes 100% vertes
# (recu session_g4_20260831_2a981bc4b73f_1788212429). Toujours 100% exige.
mapfile -t GATE_TOTALS < <(grep -oE '100% tests passed(, 0 tests failed)? out of [0-9]+' "${BUILD_LOG}" | grep -oE '[0-9]+$')
EXPECTED_GATE_BLOCKS=$((1 + V5_NEEDED))
if [ "${#GATE_TOTALS[@]}" -ne "${EXPECTED_GATE_BLOCKS}" ] \
   || { [ "${V5_NEEDED}" -eq 1 ] && [ "${GATE_TOTALS[0]}" -lt "${V5_GATE_MIN}" ]; } \
   || [ "${GATE_TOTALS[$((EXPECTED_GATE_BLOCKS - 1))]}" -lt "${V6_GATE_MIN}" ]; then
  log "REFUS : rejeu des portes non conforme (blocs=${#GATE_TOTALS[@]}/${EXPECTED_GATE_BLOCKS} totaux=${GATE_TOTALS[*]:-aucun}, planchers ${V5_GATE_MIN}/${V6_GATE_MIN}, v5_exercee=${V5_NEEDED})"
  SESSION_RC=76
  exit 76
fi
if [ "${V5_NEEDED}" -eq 1 ]; then
  log "portes VM : v5=${GATE_TOTALS[0]} v6=${GATE_TOTALS[1]} (journaux complets dans ${BUILD_LOG})"
else
  log "portes VM : v6=${GATE_TOTALS[0]} — preflight v5 SAUTE (aucune phase conf/bench/gpu-v5, § 5.14.1)"
fi

# ---- 6. LA CAMPAGNE. Generation recontrolee, boot_id verifie DANS la meme
# commande distante avant toute execution ; retour CAPTURE sans trap.
# Le describe PRE-CAMPAGNE est clampe LUI AUSSI (neuvieme tour : sans ce
# clamp il pouvait depenser la seconde reserve d'arret) — trop tard ici =
# campagne ET rapatriement non lances, arret direct par le chemin nominal.
CAMPAIGN_SKIPPED=0
if too_late "${DESCRIBE_TIMEOUT_S}"; then
  log "REFUS : echeance atteinte avant le controle pre-campagne — campagne non lancee ; rapatriement et arret restent dans leur budget"
  SESSION_RC=77
  CAMPAIGN_SKIPPED=1
  REMOTE_CAMPAIGN_RC=75
else
  check_generation || { log "REFUS : generation changee avant la campagne"; SESSION_RC=74; exit 74; }
  REMOTE_CAMPAIGN_RC=0
fi
# Enveloppe SANS depassement (septieme tour) : `now` lu UNE fois, la fin
# possible (grace comprise) est <= DEADLINE+60 (60 s = ecriture du manifeste
# distant apres le dernier run tronque par past_deadline). Si le reste est
# sous le minimum sur : NE PAS LANCER — jamais remonter le temps restant.
CAMPAIGN_TIMEOUT=60
if [ "${CAMPAIGN_SKIPPED:-0}" -eq 0 ]; then
  _now=$(/usr/bin/date +%s)
  CAMPAIGN_TIMEOUT=$(( DEADLINE_EPOCH + 60 - _now - GRACE_S ))
  if [ "${CAMPAIGN_TIMEOUT}" -lt 60 ]; then
    log "campagne NON LANCEE : temps restant insuffisant (${CAMPAIGN_TIMEOUT}s < 60s apres grace) — passage direct au rapatriement puis a l'arret"
    CAMPAIGN_SKIPPED=1
    REMOTE_CAMPAIGN_RC=75
  fi
fi
set +e
if [ "${CAMPAIGN_SKIPPED}" -eq 0 ]; then
/usr/bin/timeout -k "${GRACE_S}" "${CAMPAIGN_TIMEOUT}" "${SSH[@]}" "set -euo pipefail
  test \"\$(cat /proc/sys/kernel/random/boot_id)\" = '${BOOT_ID}' || { echo 'REFUS : boot_id different (redemarrage detecte)' >&2; exit 9; }
  export PATH=\$HOME/.local/bin:\$PATH
  cd ~/${REMOTE_DIR}
  TIME_BIN=/usr/bin/time \
  THREADS=${THREADS_VM} DEADLINE_EPOCH=${DEADLINE_EPOCH} RUN_TIMEOUT='${RUN_TIMEOUT}' \
    CONF_SPECS='${CONF_SPECS}' \
    BENCH_SPECS='${BENCH_SPECS}' \
    QUEUE_FAMILIES='${QUEUE_FAMILIES}' QUEUE_N='${QUEUE_N}' QUEUE_SEEDS='${QUEUE_SEEDS}' \
    SWEEP_SPECS='${SWEEP_SPECS}' SWEEP_REPEATS='${SWEEP_REPEATS}' \
    GPU_SPECS='${GPU_SPECS}' FRONTIER_SPECS='${FRONTIER_SPECS}' FRONTIER_TIMEOUT='${FRONTIER_TIMEOUT}' \
    GPU_BUILD_TIMEOUT='${GPU_BUILD_TIMEOUT}' FRONTIER_ULIMIT_KB='${FRONTIER_ULIMIT_KB}' \
    FRONTIER_LAYOUT='${FRONTIER_LAYOUT}' \
    MATRICE_POINTS='${MATRICE_POINTS}' MATRICE_SEQUENCE='${MATRICE_SEQUENCE}' MATRICE_TIMEOUT='${MATRICE_TIMEOUT}' \
    ATTRIB_POINTS='${ATTRIB_POINTS}' ATTRIB_TIMEOUT='${ATTRIB_TIMEOUT}' \
    GPUV6_GATE_NAMES='${GPUV6_GATE_NAMES}' GPUV6_BUILD_TIMEOUT='${GPUV6_BUILD_TIMEOUT}' GPUV6_GATE_TIMEOUT='${GPUV6_GATE_TIMEOUT}' \
    GPUV6_PILOT_SPECS='${GPUV6_PILOT_SPECS}' GPUV6_PILOT_MIN_LOTS='${GPUV6_PILOT_MIN_LOTS}' GPUV6_PILOT_TIMEOUT='${GPUV6_PILOT_TIMEOUT}' \
    V6_BIN='${BIN_MATRICE}' V6_PROFILE_BIN='${BIN_ATTRIB}' GPUV6_PILOT_BIN='${BIN_PILOTE}' \
    GRACE_S='${GRACE_S}' \
    /bin/bash gcp-migration/v6_campaign_remote.sh ${SOURCE_COMMIT} ${SOURCE_PAYLOAD_SHA256} ${PROTOCOL_MANIFEST_SHA256}
" 2>&1 | tee -a "${LOG}"
REMOTE_CAMPAIGN_RC=${PIPESTATUS[0]}
fi
set -e
printf 'remote_campaign_rc=%d\n' "${REMOTE_CAMPAIGN_RC}" | tee -a "${LOG}"

# ---- 7. RAPATRIEMENT TOUJOURS, reprises bornees, generation controlee.
mkdir -p "${WORK}/out"
SCP_RC=1
# Chaque tentative reserve son PIRE CAS : timeout scp + grace + backoff +
# describes bornes autour + DEUX reserves d'arret — le tout avant le CUTOFF
# EFFECTIF (min borne GCE / arret invite), `now` lu une fois par tentative.
# scp_worst_case_s est FACTORISE pour etre teste a la frontiere d'une
# seconde par le selftest (dixieme tour : le coefficient 2 doit etre causal).
scp_worst_case_s() { # $1 = numero de tentative
  echo $(( SCP_STEP_TIMEOUT_S + GRACE_S + 5 * $1 + 2 * (DESCRIBE_TIMEOUT_S + GRACE_S) + 2 * STOP_RESERVE_S ))
}
SCP_CUTOFF=$((GEN_EPOCH + EFFECTIVE_CUTOFF_S))
for attempt in $(seq 1 "${SCP_ATTEMPTS}"); do
  _now=$(date +%s)
  if [ "$(( _now + $(scp_worst_case_s "${attempt}") ))" -gt "${SCP_CUTOFF}" ]; then
    log "rapatriement abandonne avant la tentative ${attempt} : le pire cas deborderait sur le cutoff effectif (${SCP_CUTOFF})"
    break
  fi
  check_generation || { log "generation changee pendant le rapatriement (tentative ${attempt})"; break; }
  set +e
  /usr/bin/timeout -k "${GRACE_S}" "${SCP_STEP_TIMEOUT_S}" "${SCP[@]}" --recurse \
    "${GCP_INSTANCE_NAME}:~/${REMOTE_DIR}/out" "${WORK}/" 2>&1 | tee -a "${LOG}"
  rc=${PIPESTATUS[0]}
  set -e
  if [ "${rc}" -eq 0 ]; then
    # Controle de generation APRES le rapatriement reussi (audit deuxieme
    # tour : chaque SCP encadre avant ET apres).
    if check_generation; then
      SCP_RC=0
    else
      log "generation changee APRES le rapatriement (tentative ${attempt}) : resultat non attribuable"
    fi
    break
  fi
  log "scp tentative ${attempt} echouee (rc=${rc})"
  sleep $((5 * attempt))
done
printf 'scp_rc=%d\n' "${SCP_RC}" | tee -a "${LOG}"

# ---- 7bis. ARRET CIBLE IMMEDIAT apres le rapatriement (revue en vol du
# septieme tour : la validation est locale — la VM n'attend pas). Le meme
# registre est publie ; en cas de succes, le cleanup prend son fast-path
# (aucun second arret) ; en cas d'echec, la reprise bornee du cleanup
# re-tente.
attempt_targeted_stop
if [ "${LAST_STOP_RC}" -ne 0 ]; then
  log "premier arret post-rapatriement en echec (rc=${LAST_STOP_RC}) — re-tentative IMMEDIATE avant toute validation"
  attempt_targeted_stop
fi
EARLY_STOP_RC="${LAST_STOP_RC}"
if [ "${EARLY_STOP_RC}" -eq 0 ]; then
  log "arret cible post-rapatriement certifie (generation ${GENERATION}, tentative ${STOP_TRIES})"
else
  log "ARRET NON CERTIFIE apres ${STOP_TRIES} tentatives — validation SAUTEE, aucune conclusion de campagne"
  SESSION_RC=70
  exit 70
fi

# ---- 8. VALIDATION LOCALE par le validateur EPINGLE, profil epingle joint :
# seule autorite du statut de campagne (HORS budget VM : la cible est deja
# arretee ou en reprise).
set +e
# Validateur BORNE (septieme tour : un validateur bloque ne doit jamais
# empecher le trap d'arret d'etre atteint).
/usr/bin/timeout -k "${GRACE_S}" "${VALIDATOR_TIMEOUT_S}" \
  python3 "${VALIDATOR}" "${WORK}/out" \
  "${SOURCE_COMMIT}" "${SOURCE_PAYLOAD_SHA256}" "${PROTOCOL_MANIFEST_SHA256}" \
  "${REMOTE_CAMPAIGN_RC}" "${SCP_RC}" "${PROFILE}" "${PROFILE_SRC}" "${WORK}/manifest_revalide.txt" 2>&1 | tee "${WORK}/validation.txt" | tee -a "${LOG}"
VALIDATE_RC=${PIPESTATUS[0]}
set -e
# Le validateur ne DEGRADE jamais un refus anterieur (checkpoint dixieme
# tour : 65 ecrasait le 77 du refus pre-campagne).
if [ "${VALIDATE_RC}" -ne 0 ] && [ "${SESSION_RC}" -eq 0 ]; then SESSION_RC=65; fi

echo "session terminee ; l arret certifie est declenche par le trap"
