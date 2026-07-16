#!/usr/bin/env bash
set -euo pipefail

readonly INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
readonly ZONE="${GCP_ZONE:-europe-west4-a}"
readonly DEFAULT_PROJECT_ID="devpod-gpu-exploration"
readonly EXPECTED_MACHINE_TYPE="g4-standard-48"
readonly EXPECTED_PROVISIONING_MODEL="SPOT"
readonly MIN_ALLOWED_RUN_SECONDS=30
readonly MAX_ALLOWED_RUN_SECONDS=28800
readonly START_TIMEOUT_SECONDS=300
readonly SSH_TIMEOUT_SECONDS=300
readonly GCLOUD_READ_TIMEOUT_SECONDS=30
readonly GCLOUD_MUTATION_TIMEOUT_SECONDS=180
readonly GCLOUD_SSH_CALL_TIMEOUT_SECONDS=30
readonly GCLOUD_KILL_AFTER_SECONDS=10

PROJECT_ID="${GCP_PROJECT_ID:-${DEFAULT_PROJECT_ID}}"
GUEST_SHUTDOWN_MINUTES="${GCP_GUEST_SHUTDOWN_MINUTES:-240}"
ASSUME_YES=0
HANDOFF_FILE=""
VERIFIED_LAST_START_TIMESTAMP=""

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
témoin de génération dès que la garde GCE post-démarrage est certifiée; ce témoin
reste utilisable pour un arrêt ciblé même si la garde invitée échoue ensuite.
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
    timeout --foreground --kill-after=1s 1s true >/dev/null 2>&1
}

gcloud_read() {
    timeout --foreground --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud "$@"
}

gcloud_mutation() {
    timeout --foreground --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_MUTATION_TIMEOUT_SECONDS}s" gcloud "$@"
}

gcloud_ssh_guard() {
    timeout --foreground --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_SSH_CALL_TIMEOUT_SECONDS}s" gcloud "$@"
}

instance_field() {
    local field="$1"
    gcloud_read compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --format="value(${field})"
}

verify_static_guard() {
    local action configured_seconds label machine_type provisioning_model
    action="$(instance_field 'scheduling.instanceTerminationAction')" || return 1
    configured_seconds="$(instance_field 'scheduling.maxRunDuration.seconds')" || return 1
    label="$(instance_field 'labels.project')" || return 1
    machine_type="$(instance_field 'machineType.basename()')" || return 1
    provisioning_model="$(instance_field 'scheduling.provisioningModel')" || return 1

    [[ "${action}" == "STOP" ]] || return 1
    [[ "${configured_seconds}" =~ ^[0-9]+$ ]] || return 1
    ((configured_seconds >= MIN_ALLOWED_RUN_SECONDS && configured_seconds <= MAX_ALLOWED_RUN_SECONDS)) || return 1
    [[ "${label}" == "e-hgp" ]] || return 1
    [[ "${machine_type}" == "${EXPECTED_MACHINE_TYPE}" ]] || return 1
    [[ "${provisioning_model}" == "${EXPECTED_PROVISIONING_MODEL}" ]] || return 1
    VERIFIED_MAX_RUN_SECONDS="${configured_seconds}"
}

verify_running_guard() {
    local status action configured_seconds start_timestamp start_epoch
    local termination_timestamp termination_epoch now computed_deadline maximum_deadline

    status="$(instance_field 'status')" || return 1
    action="$(instance_field 'scheduling.instanceTerminationAction')" || return 1
    configured_seconds="$(instance_field 'scheduling.maxRunDuration.seconds')" || return 1
    start_timestamp="$(instance_field 'lastStartTimestamp')" || return 1
    termination_timestamp="$(instance_field 'terminationTimestamp')" || return 1

    [[ "${status}" == "RUNNING" && "${action}" == "STOP" ]] || return 1
    [[ "${configured_seconds}" =~ ^[0-9]+$ ]] || return 1
    ((configured_seconds >= MIN_ALLOWED_RUN_SECONDS && configured_seconds <= MAX_ALLOWED_RUN_SECONDS)) || return 1
    [[ -n "${start_timestamp}" ]] || return 1
    [[ -n "${termination_timestamp}" ]] || return 1
    [[ "${start_timestamp}" != *$'\n'* && "${start_timestamp}" != *$'\r'* ]] || return 1
    if [[ -n "${VERIFIED_LAST_START_TIMESTAMP}" && \
        "${start_timestamp}" != "${VERIFIED_LAST_START_TIMESTAMP}" ]]; then
        return 1
    fi

    start_epoch="$(timestamp_to_epoch "${start_timestamp}")" || return 1
    now="$(date +%s)"
    computed_deadline=$((start_epoch + configured_seconds))
    maximum_deadline=$((now + MAX_ALLOWED_RUN_SECONDS + 300))
    ((computed_deadline > now && computed_deadline <= maximum_deadline)) || return 1

    termination_epoch="$(timestamp_to_epoch "${termination_timestamp}")" || return 1
    ((termination_epoch > now && termination_epoch <= maximum_deadline)) || return 1
    ((termination_epoch >= computed_deadline - 300 && termination_epoch <= computed_deadline + 300)) || return 1

    VERIFIED_TERMINATION_EPOCH="${termination_epoch}"
    VERIFIED_LAST_START_TIMESTAMP="${start_timestamp}"

    printf '[GARDE-FOU GCE] action=%s, maxRunDuration=%ss, échéance calculée=%s' \
        "${action}" "${configured_seconds}" "${computed_deadline}"
    printf ', terminationTimestamp=%s' "${termination_timestamp}"
    printf '\n'
}

instance_status() {
    instance_field 'status'
}

start_attempted=0
start_certified=0
emergency_stop_on_exit() {
    local exit_code=$?
    local emergency_stop_status=0
    if ((exit_code != 0 && start_attempted == 1 && start_certified == 0)); then
        if [[ -z "${VERIFIED_LAST_START_TIMESTAMP}" ]]; then
            printf '[URGENCE] Démarrage non certifié de %s et génération lastStartTimestamp inconnue; aucun arrêt automatique non versionné n’est autorisé.\n' \
                "${INSTANCE_NAME}" >&2
            printf '[URGENCE] Projet=%s zone=%s instance=%s; dernier état certifié avant mutation=TERMINATED.\n' \
                "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" >&2
            printf 'Commande de contrôle : gcloud compute instances describe %q --project=%q --zone=%q --format=%q\n' \
                "${INSTANCE_NAME}" "${PROJECT_ID}" "${ZONE}" \
                'value(status,lastStartTimestamp)' >&2
            return 0
        fi
        printf '[URGENCE] Démarrage non certifié : tentative d’arrêt de la génération %s sur %s.\n' \
            "${VERIFIED_LAST_START_TIMESTAMP}" "${INSTANCE_NAME}" >&2
        if GCP_PROJECT_ID="${PROJECT_ID}" \
            GCP_INSTANCE_NAME="${INSTANCE_NAME}" \
            GCP_ZONE="${ZONE}" \
            "$(dirname "${BASH_SOURCE[0]}")/stop_and_verify.sh" --yes \
                --expected-last-start-timestamp "${VERIFIED_LAST_START_TIMESTAMP}"; then
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
                "${VERIFIED_LAST_START_TIMESTAMP}" >&2
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
trap emergency_stop_on_exit EXIT

publish_targeted_handoff() {
    [[ -n "${HANDOFF_FILE}" ]] || return 0
    python3 - "${HANDOFF_FILE}" "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" \
        "${GUEST_SHUTDOWN_MINUTES}" "${VERIFIED_LAST_START_TIMESTAMP}" <<'PY'
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
    "status": "targeted_running",
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

command -v gcloud >/dev/null 2>&1 || die "gcloud est introuvable."
command -v python3 >/dev/null 2>&1 || die "python3 est requis pour certifier l’échéance de la VM."
command -v timeout >/dev/null 2>&1 || die "GNU timeout est requis avant toute mutation GCP."
verify_gnu_timeout || die "timeout doit être l'implémentation GNU compatible avec --foreground et --kill-after."
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
    die "Préconditions absentes : cible g4-standard-48 Spot, label project=e-hgp, action STOP et maxRunDuration entre 30 s et 8 h sont obligatoires."
((GUEST_SHUTDOWN_MINUTES * 60 <= VERIFIED_MAX_RUN_SECONDS)) || \
    die "Le coupe-circuit invité (${GUEST_SHUTDOWN_MINUTES} min) dépasse maxRunDuration (${VERIFIED_MAX_RUN_SECONDS} s)."

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
    if status="$(instance_status 2>/dev/null)" && [[ "${status}" == "RUNNING" ]]; then
        break
    fi
    printf '[ATTENTE] État GCE : %s\n' "${status:-inconnu}"
    sleep 5
done
[[ "${status}" == "RUNNING" ]] || die "La VM n’a pas atteint RUNNING dans le délai imparti."
verify_running_guard || die "L’échéance GCE post-démarrage n’a pas pu être certifiée."
publish_targeted_handoff || \
    die "La génération démarrée est certifiée mais son témoin ciblé n’a pas pu être publié."

printf '[GARDE-FOU INVITÉ] Armement de shutdown -P +%s via SSH.\n' "${GUEST_SHUTDOWN_MINUTES}"
ssh_deadline=$((SECONDS + SSH_TIMEOUT_SECONDS))
guest_guard_output=""
guest_guard_command="$(cat <<EOF
sudo -n bash -s -- '${GUEST_SHUTDOWN_MINUTES}' '${VERIFIED_TERMINATION_EPOCH}' <<'__EHGP_GUEST_GUARD__'
set -euo pipefail

readonly requested_minutes="\$1"
readonly gce_deadline_epoch="\$2"
readonly tolerance_seconds=120
readonly scheduled_file=/run/systemd/shutdown/scheduled

[[ "\${requested_minutes}" =~ ^[1-9][0-9]*\$ ]]
[[ "\${gce_deadline_epoch}" =~ ^[0-9]+\$ ]]

shutdown -c >/dev/null 2>&1 || true
shutdown -P "+\${requested_minutes}" 'Coupe-circuit E-HGP'
[[ -r "\${scheduled_file}" ]]
scheduled="\$(cat -- "\${scheduled_file}")"
mode="\$(sed -n 's/^MODE=\(.*\)\$/\1/p' <<<"\${scheduled}")"
scheduled_usec="\$(sed -n 's/^USEC=\([0-9][0-9]*\)\$/\1/p' <<<"\${scheduled}")"
[[ "\${mode}" == 'poweroff' ]]
[[ "\${scheduled_usec}" =~ ^[0-9]{1,18}\$ ]]

now_epoch="\$(date +%s)"
scheduled_epoch=\$((10#\${scheduled_usec} / 1000000))
expected_epoch=\$((now_epoch + requested_minutes * 60))
minimum_expected=\$((expected_epoch - tolerance_seconds))
maximum_expected=\$((expected_epoch + tolerance_seconds))

((scheduled_epoch > now_epoch))
((scheduled_epoch >= minimum_expected && scheduled_epoch <= maximum_expected))
((scheduled_epoch <= gce_deadline_epoch))

printf 'MODE=%s\nUSEC=%s\n' "\${mode}" "\${scheduled_usec}"
printf '__EHGP_GUEST_GUARD_VERIFIED__\n'
__EHGP_GUEST_GUARD__
EOF
)"
while ((SECONDS < ssh_deadline)); do
    if guest_guard_output="$(gcloud_ssh_guard compute ssh "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --quiet \
        --ssh-flag='-o ConnectTimeout=15' \
        --ssh-flag='-o BatchMode=yes' \
        --command="${guest_guard_command}" 2>&1)"; then
        break
    fi
    printf '[ATTENTE] SSH ou systemd indisponible; nouvel essai dans 10 s.\n' >&2
    sleep 10
done
[[ "${guest_guard_output}" == *"__EHGP_GUEST_GUARD_VERIFIED__"* ]] || \
    die "Le coupe-circuit invité n’a pas pu être armé puis relu de manière certaine."
printf '%s\n' "${guest_guard_output/__EHGP_GUEST_GUARD_VERIFIED__/}"

verify_running_guard || die "Le garde-fou GCE n’est plus certifiable après l’armement invité."
start_certified=1
printf '[SUCCÈS] VM démarrée avec deux coupe-circuits vérifiés. Fermez la session avec stop_and_verify.sh.\n'
