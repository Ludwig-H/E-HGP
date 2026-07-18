#!/usr/bin/env bash
set -euo pipefail

readonly INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
readonly ZONE="${GCP_ZONE:-europe-west4-a}"
readonly DEFAULT_PROJECT_ID="devpod-gpu-exploration"
readonly EXPECTED_MACHINE_TYPE="g4-standard-48"
readonly EXPECTED_PROVISIONING_MODEL="SPOT"
readonly EXPECTED_MAINTENANCE_POLICY="TERMINATE"
readonly MIN_ALLOWED_RUN_SECONDS=30
readonly MAX_ALLOWED_RUN_SECONDS=28800
readonly START_TIMEOUT_SECONDS=300
readonly SSH_TIMEOUT_SECONDS=300
readonly GCLOUD_READ_TIMEOUT_SECONDS=30
readonly GCLOUD_MUTATION_TIMEOUT_SECONDS=180
readonly GCLOUD_SSH_CALL_TIMEOUT_SECONDS=30
readonly GCLOUD_KILL_AFTER_SECONDS=10
readonly TIMESTAMP_TOLERANCE_SECONDS=300
readonly EXPECTED_SSH_KEY_ALGORITHM="ssh-ed25519"
readonly SSH_KEY_TTL_SLACK_SECONDS=660
readonly MAX_PUBLICKEY_DENIALS=6

PROJECT_ID="${GCP_PROJECT_ID:-${DEFAULT_PROJECT_ID}}"
GUEST_SHUTDOWN_MINUTES="${GCP_GUEST_SHUTDOWN_MINUTES:-240}"
SSH_KEY_FILE="${GCP_SSH_KEY_FILE:-}"
SSH_KEY_EXPIRATION_UTC=""
ASSUME_YES=0
HANDOFF_FILE=""
VERIFIED_LAST_START_TIMESTAMP=""
TARGET_LAST_START_TIMESTAMP=""
PRE_START_LAST_START_TIMESTAMP=""
START_REQUEST_EPOCH=""

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/start_and_verify.sh [--yes] [--guest-shutdown-minutes MINUTES] [--handoff-file CHEMIN]

Démarre la VM ciblée seulement si son coupe-circuit GCE est borné à huit heures,
certifie l'échéance après démarrage, puis arme et vérifie un arrêt dans l'OS invité.
Le mode est interactif par défaut. --yes est réservé à une exécution explicitement
autorisée et ne désactive aucun contrôle. --handoff-file publie atomiquement un
témoin de génération dès que la garde GCE post-démarrage est certifiée ou qu'une
préemption immédiate est observée; ce témoin reste utilisable pour un arrêt ciblé.

GCP_SSH_KEY_FILE doit désigner une clé de session ED25519 privée non chiffrée,
déjà inscrite dans OS Login avec une expiration bornée. Le script la vérifie
avant toute mutation GCE, mémorise cette expiration UTC exacte et les transmet
toutes deux explicitement à la garde invitée.
EOF
}

while (($# > 0)); do
    case "$1" in
        --yes)
            ASSUME_YES=1
            shift
            ;;
        --guest-shutdown-minutes)
            (($# >= 2)) || die "Valeur manquante après --guest-shutdown-minutes."
            GUEST_SHUTDOWN_MINUTES="$2"
            shift 2
            ;;
        --handoff-file)
            (($# >= 2)) || die "Valeur manquante après --handoff-file."
            [[ -z "${HANDOFF_FILE}" ]] || die "--handoff-file ne peut être fourni qu'une fois."
            HANDOFF_FILE="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "Option inconnue : $1"
            ;;
    esac
done

timestamp_to_epoch() {
    python3 - "$1" <<'PY'
from datetime import datetime
import sys

value = sys.argv[1]
if value.endswith("Z"):
    value = value[:-1] + "+00:00"
print(int(datetime.fromisoformat(value).timestamp()))
PY
}

verify_gnu_timeout() {
    local version=""
    version="$(timeout --version 2>/dev/null | sed -n '1p')" || return 1
    [[ "${version}" == timeout\ \(GNU\ coreutils\)* ]] || return 1
    timeout --kill-after=1s 1s true >/dev/null 2>&1
}

gcloud_read() {
    timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud "$@"
}

gcloud_mutation() {
    timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_MUTATION_TIMEOUT_SECONDS}s" gcloud "$@"
}

gcloud_ssh_guard() {
    timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_SSH_CALL_TIMEOUT_SECONDS}s" gcloud "$@"
}

verify_batch_ssh_key() {
    local declared_public=""
    local derived_public=""
    local fingerprint=""
    local key_mode=""

    [[ -n "${SSH_KEY_FILE}" ]] || {
        printf '[ERREUR] GCP_SSH_KEY_FILE est obligatoire avant tout démarrage; utilisez une clé ED25519 de session expirante.\n' >&2
        return 1
    }
    case "${SSH_KEY_FILE}" in
        /*) ;;
        *)
            printf '[ERREUR] GCP_SSH_KEY_FILE doit être un chemin absolu.\n' >&2
            return 1
            ;;
    esac
    [[ -f "${SSH_KEY_FILE}" && ! -L "${SSH_KEY_FILE}" ]] || {
        printf '[ERREUR] Clé privée de session absente, non régulière ou symbolique : %s.\n' \
            "${SSH_KEY_FILE}" >&2
        return 1
    }
    [[ -f "${SSH_KEY_FILE}.pub" && ! -L "${SSH_KEY_FILE}.pub" ]] || {
        printf '[ERREUR] Clé publique de session absente, non régulière ou symbolique : %s.pub.\n' \
            "${SSH_KEY_FILE}" >&2
        return 1
    }

    key_mode="$(stat -c '%a' -- "${SSH_KEY_FILE}" 2>/dev/null)" || return 1
    [[ "${key_mode}" =~ ^[0-7]{3,4}$ ]] || return 1
    if [[ "${key_mode}" != "600" ]]; then
        printf '[ERREUR] La clé privée de session doit être exactement en mode 600 (mode reçu=%s).\n' \
            "${key_mode}" >&2
        return 1
    fi

    declared_public="$(awk 'NF >= 2 {print $1 " " $2; exit}' "${SSH_KEY_FILE}.pub")" || return 1
    [[ "${declared_public}" == "${EXPECTED_SSH_KEY_ALGORITHM} "* ]] || {
        printf '[ERREUR] La clé de session doit utiliser %s.\n' \
            "${EXPECTED_SSH_KEY_ALGORITHM}" >&2
        return 1
    }
    if ! derived_public="$(ssh-keygen -y -P '' -f "${SSH_KEY_FILE}" 2>/dev/null)"; then
        printf '[ERREUR] La clé privée de session est chiffrée ou illisible en BatchMode; aucun démarrage GCE ne sera tenté.\n' >&2
        return 1
    fi
    derived_public="$(awk 'NF >= 2 {print $1 " " $2; exit}' <<<"${derived_public}")"
    [[ "${derived_public}" == "${declared_public}" ]] || {
        printf '[ERREUR] Les moitiés privée et publique de la clé de session ne correspondent pas.\n' >&2
        return 1
    }
    fingerprint="$(ssh-keygen -lf "${SSH_KEY_FILE}.pub" -E sha256 2>/dev/null | awk 'NF >= 2 {print $2; exit}')" || return 1
    [[ "${fingerprint}" == SHA256:* ]] || return 1
    printf '[GARDE SSH] Clé de session %s utilisable sans interaction (%s).\n' \
        "${EXPECTED_SSH_KEY_ALGORITHM}" "${fingerprint}"
}

verify_oslogin_session_key() {
    local declared_algorithm=""
    local declared_blob=""
    local expiration_fields=""
    local profile_json=""
    local remaining_seconds=""

    read -r declared_algorithm declared_blob _ <"${SSH_KEY_FILE}.pub" || return 1
    [[ "${declared_algorithm}" == "${EXPECTED_SSH_KEY_ALGORITHM}" && \
        -n "${declared_blob}" ]] || return 1
    profile_json="$(gcloud_read compute os-login describe-profile \
        --project="${PROJECT_ID}" \
        --format=json)" || return 1
    expiration_fields="$(python3 - \
        "${declared_algorithm}" "${declared_blob}" \
        "${VERIFIED_MAX_RUN_SECONDS}" "${SSH_KEY_TTL_SLACK_SECONDS}" \
        "${profile_json}" <<'PY'
from datetime import datetime, timezone
import json
import sys
import time

algorithm = sys.argv[1]
blob = sys.argv[2]
minimum = int(sys.argv[3])
maximum = minimum + int(sys.argv[4])
value = json.loads(sys.argv[5])
keys = value.get("sshPublicKeys") if isinstance(value, dict) else None
if not isinstance(keys, dict):
    raise SystemExit("profil OS Login sans clés")
matches = []
for record in keys.values():
    if not isinstance(record, dict):
        continue
    fields = str(record.get("key", "")).split()
    if len(fields) < 2 or fields[0] != algorithm or fields[1] != blob:
        continue
    expiration = record.get("expirationTimeUsec")
    if isinstance(expiration, bool):
        continue
    try:
        expiration_usec = int(expiration)
    except (TypeError, ValueError):
        continue
    matches.append(expiration_usec)
if len(matches) != 1:
    raise SystemExit("clé de session absente, dupliquée ou sans expiration OS Login")
expiration_usec = matches[0]
remaining = (expiration_usec - time.time_ns() // 1_000) // 1_000_000
if remaining < minimum or remaining > maximum:
    raise SystemExit(
        f"durée OS Login restante hors borne: {remaining}s, attendu {minimum}..{maximum}s"
    )
seconds, microseconds = divmod(expiration_usec, 1_000_000)
expiration = datetime.fromtimestamp(seconds, timezone.utc).replace(
    microsecond=microseconds
)
expiration_utc = expiration.isoformat(timespec="microseconds").replace("+00:00", "Z")
print(f"{remaining}\t{expiration_utc}")
PY
)" || return 1
    IFS=$'\t' read -r remaining_seconds SSH_KEY_EXPIRATION_UTC <<<"${expiration_fields}"
    [[ "${remaining_seconds}" =~ ^[0-9]+$ && \
        "${SSH_KEY_EXPIRATION_UTC}" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{6}Z$ ]] || return 1
    printf '[GARDE SSH] Clé OS Login unique, expiration fixe=%s, durée restante=%ss.\n' \
        "${SSH_KEY_EXPIRATION_UTC}" "${remaining_seconds}"
}

instance_field() {
    local field="$1"
    gcloud_read compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --format="value(${field})"
}

verify_static_guard() {
    local action automatic_restart configured_seconds label machine_type
    local maintenance_policy provisioning_model
    action="$(instance_field 'scheduling.instanceTerminationAction')" || return 1
    automatic_restart="$(instance_field 'scheduling.automaticRestart')" || return 1
    configured_seconds="$(instance_field 'scheduling.maxRunDuration.seconds')" || return 1
    label="$(instance_field 'labels.project')" || return 1
    machine_type="$(instance_field 'machineType.basename()')" || return 1
    maintenance_policy="$(instance_field 'scheduling.onHostMaintenance')" || return 1
    provisioning_model="$(instance_field 'scheduling.provisioningModel')" || return 1

    [[ "${action}" == "STOP" ]] || return 1
    [[ "${automatic_restart,,}" == "false" ]] || return 1
    [[ "${configured_seconds}" =~ ^[0-9]+$ ]] || return 1
    ((configured_seconds >= MIN_ALLOWED_RUN_SECONDS && configured_seconds <= MAX_ALLOWED_RUN_SECONDS)) || return 1
    [[ "${label}" == "e-hgp" ]] || return 1
    [[ "${machine_type}" == "${EXPECTED_MACHINE_TYPE}" ]] || return 1
    [[ "${maintenance_policy}" == "${EXPECTED_MAINTENANCE_POLICY}" ]] || return 1
    [[ "${provisioning_model}" == "${EXPECTED_PROVISIONING_MODEL}" ]] || return 1
    VERIFIED_MAX_RUN_SECONDS="${configured_seconds}"
}

verify_running_guard() {
    local status action automatic_restart configured_seconds label machine_type
    local maintenance_policy provisioning_model start_timestamp start_epoch
    local termination_timestamp termination_epoch now computed_deadline maximum_deadline

    status="$(instance_field 'status')" || return 1
    action="$(instance_field 'scheduling.instanceTerminationAction')" || return 1
    automatic_restart="$(instance_field 'scheduling.automaticRestart')" || return 1
    configured_seconds="$(instance_field 'scheduling.maxRunDuration.seconds')" || return 1
    label="$(instance_field 'labels.project')" || return 1
    machine_type="$(instance_field 'machineType.basename()')" || return 1
    maintenance_policy="$(instance_field 'scheduling.onHostMaintenance')" || return 1
    provisioning_model="$(instance_field 'scheduling.provisioningModel')" || return 1
    start_timestamp="$(instance_field 'lastStartTimestamp')" || return 1
    termination_timestamp="$(instance_field 'terminationTimestamp')" || return 1

    [[ "${status}" == "RUNNING" && "${action}" == "STOP" ]] || return 1
    [[ "${automatic_restart,,}" == "false" ]] || return 1
    [[ "${configured_seconds}" =~ ^[0-9]+$ ]] || return 1
    ((configured_seconds >= MIN_ALLOWED_RUN_SECONDS && configured_seconds <= MAX_ALLOWED_RUN_SECONDS)) || return 1
    [[ "${label}" == "e-hgp" ]] || return 1
    [[ "${machine_type}" == "${EXPECTED_MACHINE_TYPE}" ]] || return 1
    [[ "${maintenance_policy}" == "${EXPECTED_MAINTENANCE_POLICY}" ]] || return 1
    [[ "${provisioning_model}" == "${EXPECTED_PROVISIONING_MODEL}" ]] || return 1
    [[ "${configured_seconds}" == "${VERIFIED_MAX_RUN_SECONDS}" ]] || return 1
    [[ -n "${start_timestamp}" ]] || return 1
    [[ "${START_REQUEST_EPOCH}" =~ ^[0-9]+$ ]] || return 1
    [[ "${termination_timestamp}" != *$'\n'* && \
        "${termination_timestamp}" != *$'\r'* ]] || return 1
    [[ "${start_timestamp}" != *$'\n'* && "${start_timestamp}" != *$'\r'* ]] || return 1
    [[ -n "${TARGET_LAST_START_TIMESTAMP}" ]] || return 1
    [[ "${start_timestamp}" == "${TARGET_LAST_START_TIMESTAMP}" ]] || return 1

    start_epoch="$(timestamp_to_epoch "${start_timestamp}")" || return 1
    now="$(date +%s)"
    ((start_epoch >= START_REQUEST_EPOCH - TIMESTAMP_TOLERANCE_SECONDS)) || return 1
    ((start_epoch <= now + TIMESTAMP_TOLERANCE_SECONDS)) || return 1
    computed_deadline=$((start_epoch + configured_seconds))
    maximum_deadline=$((now + configured_seconds + TIMESTAMP_TOLERANCE_SECONDS))
    ((computed_deadline > now && computed_deadline <= maximum_deadline)) || return 1

    VERIFIED_SAFE_DEADLINE_EPOCH=$((computed_deadline - TIMESTAMP_TOLERANCE_SECONDS))
    ((VERIFIED_SAFE_DEADLINE_EPOCH > now)) || return 1
    if [[ -n "${termination_timestamp}" ]]; then
        termination_epoch="$(timestamp_to_epoch "${termination_timestamp}")" || return 1
        ((termination_epoch > now && termination_epoch <= maximum_deadline)) || return 1
        ((termination_epoch >= computed_deadline - TIMESTAMP_TOLERANCE_SECONDS && termination_epoch <= computed_deadline + TIMESTAMP_TOLERANCE_SECONDS)) || return 1
        ((VERIFIED_SAFE_DEADLINE_EPOCH <= termination_epoch)) || return 1
    else
        [[ "${ZONE}" == *-ai* ]] || return 1
    fi

    VERIFIED_LAST_START_TIMESTAMP="${start_timestamp}"

    printf '[GARDE-FOU GCE] action=%s, maxRunDuration=%ss, échéance calculée=%s' \
        "${action}" "${configured_seconds}" "${computed_deadline}"
    printf ', échéance sûre=%s' "${VERIFIED_SAFE_DEADLINE_EPOCH}"
    if [[ -n "${termination_timestamp}" ]]; then
        printf ', terminationTimestamp=%s' "${termination_timestamp}"
    else
        printf ', terminationTimestamp non exposé; échéance calculée certifiée'
    fi
    printf '\n'
}

capture_started_generation() {
    local observed_generation=""

    observed_generation="$(instance_field 'lastStartTimestamp')" || return 1
    [[ -n "${observed_generation}" ]] || return 1
    [[ "${observed_generation}" != *$'\n'* && "${observed_generation}" != *$'\r'* ]] || return 1
    if [[ -n "${TARGET_LAST_START_TIMESTAMP}" ]]; then
        [[ "${observed_generation}" == "${TARGET_LAST_START_TIMESTAMP}" ]] || return 2
        return 0
    fi
    [[ "${observed_generation}" != "${PRE_START_LAST_START_TIMESTAMP}" ]] || return 1
    TARGET_LAST_START_TIMESTAMP="${observed_generation}"
}

instance_status() {
    instance_field 'status'
}

start_attempted=0
start_certified=0
emergency_stop_on_exit() {
    local exit_code=$?
    local emergency_stop_status=0
    trap - EXIT HUP INT TERM
    if ((exit_code != 0 && start_attempted == 1 && start_certified == 0)); then
        if [[ -z "${TARGET_LAST_START_TIMESTAMP}" ]]; then
            printf '[URGENCE] Démarrage non certifié de %s et génération lastStartTimestamp inconnue; aucun arrêt automatique non versionné n’est autorisé.\n' \
                "${INSTANCE_NAME}" >&2
            printf '[URGENCE] Projet=%s zone=%s instance=%s; dernier état certifié avant mutation=TERMINATED.\n' \
                "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" >&2
            printf 'Commande de contrôle : gcloud compute instances describe %q --project=%q --zone=%q --format=%q\n' \
                "${INSTANCE_NAME}" "${PROJECT_ID}" "${ZONE}" \
                'value(status,lastStartTimestamp)' >&2
            return 0
        fi
        if [[ -n "${HANDOFF_FILE}" && ! -e "${HANDOFF_FILE}" && ! -L "${HANDOFF_FILE}" ]]; then
            if ! publish_targeted_handoff "targeted_stopping"; then
                printf '[URGENCE] Génération connue mais publication du handoff ciblé impossible : %s\n' \
                    "${HANDOFF_FILE}" >&2
            fi
        fi
        printf '[URGENCE] Démarrage non certifié : tentative d’arrêt de la génération %s sur %s.\n' \
            "${TARGET_LAST_START_TIMESTAMP}" "${INSTANCE_NAME}" >&2
        if GCP_PROJECT_ID="${PROJECT_ID}" \
            GCP_INSTANCE_NAME="${INSTANCE_NAME}" \
            GCP_ZONE="${ZONE}" \
            "$(dirname "${BASH_SOURCE[0]}")/stop_and_verify.sh" --yes \
                --expected-last-start-timestamp "${TARGET_LAST_START_TIMESTAMP}"; then
            emergency_stop_status=0
        else
            emergency_stop_status=$?
        fi
        if ((emergency_stop_status == 0)); then
            if [[ -n "${HANDOFF_FILE}" && -e "${HANDOFF_FILE}" ]]; then
                rm -f -- "${HANDOFF_FILE}" || \
                    printf '[URGENCE] Arrêt certifié mais témoin impossible à retirer : %s\n' \
                        "${HANDOFF_FILE}" >&2
            fi
            printf '[URGENCE] Arrêt ciblé certifié pour la génération %s.\n' \
                "${TARGET_LAST_START_TIMESTAMP}" >&2
        else
            printf '[URGENCE] Arrêt non vérifié. Projet=%s zone=%s instance=%s. Contrôlez GCP immédiatement.\n' \
                "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" >&2
            if [[ -n "${HANDOFF_FILE}" && -e "${HANDOFF_FILE}" ]]; then
                printf '[HANDOFF CONSERVÉ] Preuve ciblée à reprendre : %s\n' \
                    "${HANDOFF_FILE}" >&2
            fi
        fi
    fi
}

publish_targeted_handoff() {
    local handoff_status="${1:-targeted_running}"
    [[ -n "${HANDOFF_FILE}" ]] || return 0
    python3 - "${HANDOFF_FILE}" "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" \
        "${GUEST_SHUTDOWN_MINUTES}" "${TARGET_LAST_START_TIMESTAMP}" \
        "${handoff_status}" <<'PY'
import json
import os
from pathlib import Path
import sys
import tempfile

path = Path(sys.argv[1])
record = {
    "guest_shutdown_minutes": int(sys.argv[5]),
    "instance": sys.argv[4],
    "last_start_timestamp": sys.argv[6],
    "project": sys.argv[2],
    "schema": "e-hgp.start-handoff.v3",
    "status": sys.argv[7],
    "zone": sys.argv[3],
}
encoded = (json.dumps(record, sort_keys=True, separators=(",", ":")) + "\n").encode()
descriptor, temporary_name = tempfile.mkstemp(
    prefix=f".{path.name}.", suffix=".partial", dir=path.parent
)
temporary = Path(temporary_name)
try:
    offset = 0
    while offset < len(encoded):
        offset += os.write(descriptor, encoded[offset:])
    os.fsync(descriptor)
    os.close(descriptor)
    descriptor = -1
    os.link(temporary, path, follow_symlinks=False)
    os.unlink(temporary)
    directory = os.open(path.parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
    try:
        os.fsync(directory)
    finally:
        os.close(directory)
finally:
    if descriptor >= 0:
        os.close(descriptor)
    temporary.unlink(missing_ok=True)
PY
}

fail_started_generation() {
    local message="$1"

    if [[ -n "${TARGET_LAST_START_TIMESTAMP}" && -n "${HANDOFF_FILE}" && \
        ! -e "${HANDOFF_FILE}" && ! -L "${HANDOFF_FILE}" ]]; then
        publish_targeted_handoff "targeted_stopping" || \
            die "La génération non certifiée est connue, mais son témoin ciblé n’a pas pu être publié."
    fi
    die "${message}"
}

trap emergency_stop_on_exit EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

command -v gcloud >/dev/null 2>&1 || die "gcloud est introuvable."
command -v python3 >/dev/null 2>&1 || die "python3 est requis pour certifier l’échéance de la VM."
command -v ssh-keygen >/dev/null 2>&1 || die "ssh-keygen est requis pour valider la clé de session."
command -v timeout >/dev/null 2>&1 || die "GNU timeout est requis avant toute mutation GCP."
verify_gnu_timeout || die "timeout doit être l'implémentation GNU compatible avec la gestion du groupe de processus et --kill-after."
if [[ -n "${HANDOFF_FILE}" ]]; then
    case "${HANDOFF_FILE}" in
        /*) ;;
        *) die "--handoff-file doit être un chemin absolu." ;;
    esac
    handoff_parent="$(dirname -- "${HANDOFF_FILE}")"
    handoff_base="$(basename -- "${HANDOFF_FILE}")"
    [[ -n "${handoff_base}" && "${handoff_base}" != "." && "${handoff_base}" != ".." ]] || \
        die "Nom de témoin de handoff invalide."
    [[ -d "${handoff_parent}" ]] || die "Parent du témoin de handoff absent : ${handoff_parent}."
    handoff_parent="$(cd -- "${handoff_parent}" && pwd -P)" || die "Parent du handoff illisible."
    HANDOFF_FILE="${handoff_parent}/${handoff_base}"
    [[ ! -e "${HANDOFF_FILE}" && ! -L "${HANDOFF_FILE}" ]] || \
        die "Le témoin de handoff doit être inexistant : ${HANDOFF_FILE}."
fi
[[ "${GUEST_SHUTDOWN_MINUTES}" =~ ^[1-9][0-9]*$ ]] || die "Le délai invité doit être un entier positif en minutes."
(( ${#GUEST_SHUTDOWN_MINUTES} <= 3 )) || die "Le coupe-circuit invité ne peut pas dépasser 480 minutes."
GUEST_SHUTDOWN_MINUTES=$((10#${GUEST_SHUTDOWN_MINUTES}))
((GUEST_SHUTDOWN_MINUTES <= 480)) || die "Le coupe-circuit invité ne peut pas dépasser 480 minutes."

configured_project="$(gcloud_read config get-value project 2>/dev/null || true)"
[[ "${configured_project}" == "${PROJECT_ID}" ]] || \
    die "Projet actif « ${configured_project:-non configuré} » différent de « ${PROJECT_ID} »."
account="$(gcloud_read config get-value account 2>/dev/null || true)"
[[ -n "${account}" && "${account}" != "(unset)" ]] || die "Aucun compte gcloud actif."

if ! existing_instance="$(gcloud_read compute instances list \
    --project="${PROJECT_ID}" \
    --zones="${ZONE}" \
    --filter="name=${INSTANCE_NAME}" \
    --limit=1 \
    --format='value(name)')"; then
    die "Impossible de vérifier l’existence de ${INSTANCE_NAME}; démarrage refusé."
fi
[[ "${existing_instance}" == "${INSTANCE_NAME}" ]] || \
    die "Instance ${INSTANCE_NAME} introuvable dans ${PROJECT_ID}/${ZONE}."

status="$(instance_status)" || die "Impossible de lire l’état de ${INSTANCE_NAME}."
[[ "${status}" == "TERMINATED" ]] || \
    die "État ${status} : le script ne démarre qu’une VM explicitement arrêtée (TERMINATED)."
verify_static_guard || \
    die "Préconditions absentes : cible g4-standard-48 Spot, maintenance TERMINATE, redémarrage automatique désactivé, label project=e-hgp, action STOP et maxRunDuration entre 30 s et 8 h sont obligatoires."
((GUEST_SHUTDOWN_MINUTES * 60 + TIMESTAMP_TOLERANCE_SECONDS <= VERIFIED_MAX_RUN_SECONDS)) || \
    die "Le coupe-circuit invité (${GUEST_SHUTDOWN_MINUTES} min) et sa marge de ${TIMESTAMP_TOLERANCE_SECONDS} s dépassent maxRunDuration (${VERIFIED_MAX_RUN_SECONDS} s)."
verify_batch_ssh_key || \
    die "La clé SSH de session n'est pas utilisable de manière non interactive; démarrage refusé."
verify_oslogin_session_key || \
    die "La clé SSH de session n'est pas inscrite une seule fois dans OS Login avec une expiration restante comprise entre maxRunDuration et maxRunDuration + ${SSH_KEY_TTL_SLACK_SECONDS} secondes; démarrage refusé."
PRE_START_LAST_START_TIMESTAMP="$(instance_field 'lastStartTimestamp')" || \
    die "Impossible de lire la génération avant démarrage."
[[ "${PRE_START_LAST_START_TIMESTAMP}" != *$'\n'* && \
    "${PRE_START_LAST_START_TIMESTAMP}" != *$'\r'* ]] || \
    die "Génération avant démarrage ambiguë."

printf '%s\n' \
    "[START] Démarrage protégé demandé :" \
    "  compte                 : ${account}" \
    "  projet                 : ${PROJECT_ID}" \
    "  instance               : ${INSTANCE_NAME}" \
    "  zone                   : ${ZONE}" \
    "  machine/provisioning   : ${EXPECTED_MACHINE_TYPE} / ${EXPECTED_PROVISIONING_MODEL}" \
    "  maxRunDuration GCE     : ${VERIFIED_MAX_RUN_SECONDS} s" \
    "  arrêt invité secondaire : ${GUEST_SHUTDOWN_MINUTES} min"

if ((ASSUME_YES == 0)); then
    [[ -t 0 ]] || die "Confirmation interactive requise; utilisez --yes seulement après autorisation explicite."
    expected_confirmation="DEMARRER ${INSTANCE_NAME} DANS ${PROJECT_ID}"
    read -r -p "Tapez exactement « ${expected_confirmation} » : " confirmation
    [[ "${confirmation}" == "${expected_confirmation}" ]] || die "Démarrage annulé."
fi

START_REQUEST_EPOCH="$(date +%s)" || die "Impossible d'horodater la demande de démarrage."
[[ "${START_REQUEST_EPOCH}" =~ ^[0-9]+$ ]] || die "Horodatage de démarrage invalide."
start_attempted=1
if ! gcloud_mutation compute instances start "${INSTANCE_NAME}" \
    --project="${PROJECT_ID}" \
    --zone="${ZONE}" \
    --quiet; then
    die "La commande de démarrage GCP a échoué ou dépassé ${GCLOUD_MUTATION_TIMEOUT_SECONDS} s."
fi

deadline=$((SECONDS + START_TIMEOUT_SECONDS))
status="UNKNOWN"
while ((SECONDS < deadline)); do
    if status="$(instance_status 2>/dev/null)"; then
        capture_status=0
        capture_started_generation 2>/dev/null || capture_status=$?
        if ((capture_status == 2)); then
            fail_started_generation \
                "Une génération concurrente a remplacé ${TARGET_LAST_START_TIMESTAMP}; aucun arrêt non versionné n’est autorisé."
        fi
        if [[ "${status}" == "RUNNING" ]]; then
            if [[ -n "${TARGET_LAST_START_TIMESTAMP}" ]]; then
                break
            fi
            printf '[ATTENTE] RUNNING observé, génération lastStartTimestamp non encore matérialisée.\n'
        fi
        if [[ -n "${TARGET_LAST_START_TIMESTAMP}" && \
            ("${status}" == "STOPPING" || "${status}" == "TERMINATED") ]]; then
            fail_started_generation \
                "La VM Spot a été préemptée pendant le démarrage; la génération ${TARGET_LAST_START_TIMESTAMP} sera certifiée arrêtée."
        fi
    fi
    printf '[ATTENTE] État GCE : %s\n' "${status:-inconnu}"
    sleep 5
done
[[ "${status}" == "RUNNING" && -n "${TARGET_LAST_START_TIMESTAMP}" ]] || \
    fail_started_generation "La VM n’a pas atteint RUNNING dans le délai imparti."
verify_running_guard || fail_started_generation \
    "La garde post-démarrage g4-standard-48/SPOT/TERMINATE/STOP n’a pas pu être certifiée."
publish_targeted_handoff "targeted_running" || \
    die "La génération démarrée est certifiée mais son témoin ciblé n’a pas pu être publié."

printf '[GARDE-FOU INVITÉ] Armement de shutdown -P +%s via SSH.\n' "${GUEST_SHUTDOWN_MINUTES}"
ssh_deadline=$((SECONDS + SSH_TIMEOUT_SECONDS))
guest_guard_output=""
publickey_denials=0
guest_guard_script="$(printf '%s; ' \
    'set -euo pipefail' \
    'readonly requested_minutes="$1"' \
    'readonly gce_deadline_epoch="$2"' \
    'readonly tolerance_seconds=120' \
    'readonly scheduled_file=/run/systemd/shutdown/scheduled' \
    '[[ "${requested_minutes}" =~ ^[1-9][0-9]*$ ]]' \
    '[[ "${gce_deadline_epoch}" =~ ^[0-9]+$ ]]' \
    'shutdown -c >/dev/null 2>&1 || true' \
    'shutdown -P "+${requested_minutes}" "Coupe-circuit E-HGP"' \
    '[[ -r "${scheduled_file}" ]]' \
    'mode="$(sed -n "s/^MODE=\\(.*\\)$/\\1/p" "${scheduled_file}")"' \
    'scheduled_usec="$(sed -n "s/^USEC=\\([0-9][0-9]*\\)$/\\1/p" "${scheduled_file}")"' \
    '[[ "${mode}" == "poweroff" ]]' \
    '[[ "${scheduled_usec}" =~ ^[0-9]{1,18}$ ]]' \
    'now_epoch="$(date +%s)"' \
    'scheduled_epoch=$((10#${scheduled_usec} / 1000000))' \
    'expected_epoch=$((now_epoch + requested_minutes * 60))' \
    'minimum_expected=$((expected_epoch - tolerance_seconds))' \
    'maximum_expected=$((expected_epoch + tolerance_seconds))' \
    '((scheduled_epoch > now_epoch))' \
    '((scheduled_epoch >= minimum_expected && scheduled_epoch <= maximum_expected))' \
    '((scheduled_epoch <= gce_deadline_epoch))' \
    'printf "MODE=%s\nUSEC=%s\n" "${mode}" "${scheduled_usec}"' \
    'printf "__EHGP_GUEST_GUARD_VERIFIED__\n"' \
)" || die "Impossible de construire la commande du coupe-circuit invité."
[[ "${guest_guard_script}" != *"'"* ]] || \
    die "La commande du coupe-circuit invité contient une apostrophe non transportable."
guest_guard_command="sudo -n bash -c '${guest_guard_script}' -- '${GUEST_SHUTDOWN_MINUTES}' '${VERIFIED_SAFE_DEADLINE_EPOCH}'"
while ((SECONDS < ssh_deadline)); do
    if guest_guard_output="$(gcloud_ssh_guard compute ssh "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --quiet \
        --ssh-key-file="${SSH_KEY_FILE}" \
        --ssh-key-expiration="${SSH_KEY_EXPIRATION_UTC}" \
        --ssh-flag='-o ConnectTimeout=15' \
        --ssh-flag='-o BatchMode=yes' \
        --command="${guest_guard_command}" 2>&1)"; then
        break
    fi
    if [[ "${guest_guard_output}" == *"Permission denied (publickey)"* ]]; then
        ((publickey_denials += 1))
        if ((publickey_denials >= MAX_PUBLICKEY_DENIALS)); then
            printf '[ERREUR] Authentification SSH refusée %s fois malgré la propagation OS Login; arrêt des nouvelles tentatives.\n' \
                "${publickey_denials}" >&2
            break
        fi
    else
        publickey_denials=0
    fi
    printf '[ATTENTE] SSH ou systemd indisponible; nouvel essai dans 10 s.\n' >&2
    sleep 10
done
if [[ "${guest_guard_output}" != *"__EHGP_GUEST_GUARD_VERIFIED__"* ]]; then
    printf '[DIAGNOSTIC GARDE INVITÉE] %s\n' \
        "${guest_guard_output:-aucune sortie SSH reçue}" >&2
    die "Le coupe-circuit invité n’a pas pu être armé puis relu de manière certaine."
fi
printf '%s\n' "${guest_guard_output/__EHGP_GUEST_GUARD_VERIFIED__/}"

verify_running_guard || die "Le garde-fou GCE n’est plus certifiable après l’armement invité."
start_certified=1
printf '[SUCCÈS] VM démarrée avec deux coupe-circuits vérifiés. Fermez la session avec stop_and_verify.sh.\n'
