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

PROJECT_ID="${GCP_PROJECT_ID:-${DEFAULT_PROJECT_ID}}"
GUEST_SHUTDOWN_MINUTES="${GCP_GUEST_SHUTDOWN_MINUTES:-240}"
ASSUME_YES=0

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/start_and_verify.sh [--yes] [--guest-shutdown-minutes MINUTES]

Démarre la VM ciblée seulement si son coupe-circuit GCE est borné à huit heures,
certifie l'échéance après démarrage, puis arme et vérifie un arrêt dans l'OS invité.
Le mode est interactif par défaut. --yes est réservé à une exécution explicitement
autorisée et ne désactive aucun contrôle.
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

instance_field() {
    local field="$1"
    gcloud compute instances describe "${INSTANCE_NAME}" \
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

    start_epoch="$(timestamp_to_epoch "${start_timestamp}")" || return 1
    now="$(date +%s)"
    computed_deadline=$((start_epoch + configured_seconds))
    maximum_deadline=$((now + MAX_ALLOWED_RUN_SECONDS + 300))
    ((computed_deadline > now && computed_deadline <= maximum_deadline)) || return 1

    termination_epoch="$(timestamp_to_epoch "${termination_timestamp}")" || return 1
    ((termination_epoch > now && termination_epoch <= maximum_deadline)) || return 1
    ((termination_epoch >= computed_deadline - 300 && termination_epoch <= computed_deadline + 300)) || return 1

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
    if ((exit_code != 0 && start_attempted == 1 && start_certified == 0)); then
        printf '[URGENCE] Démarrage non certifié : tentative d’arrêt immédiat de %s.\n' "${INSTANCE_NAME}" >&2
        GCP_PROJECT_ID="${PROJECT_ID}" \
        GCP_INSTANCE_NAME="${INSTANCE_NAME}" \
        GCP_ZONE="${ZONE}" \
        "$(dirname "${BASH_SOURCE[0]}")/stop_and_verify.sh" --yes || \
            printf '[URGENCE] Arrêt non vérifié. Projet=%s zone=%s instance=%s. Contrôlez GCP immédiatement.\n' \
                "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" >&2
    fi
}
trap emergency_stop_on_exit EXIT

command -v gcloud >/dev/null 2>&1 || die "gcloud est introuvable."
command -v python3 >/dev/null 2>&1 || die "python3 est requis pour certifier l’échéance de la VM."
[[ "${GUEST_SHUTDOWN_MINUTES}" =~ ^[1-9][0-9]*$ ]] || die "Le délai invité doit être un entier positif en minutes."
(( ${#GUEST_SHUTDOWN_MINUTES} <= 3 )) || die "Le coupe-circuit invité ne peut pas dépasser 480 minutes."
GUEST_SHUTDOWN_MINUTES=$((10#${GUEST_SHUTDOWN_MINUTES}))
((GUEST_SHUTDOWN_MINUTES <= 480)) || die "Le coupe-circuit invité ne peut pas dépasser 480 minutes."

configured_project="$(gcloud config get-value project 2>/dev/null || true)"
[[ "${configured_project}" == "${PROJECT_ID}" ]] || \
    die "Projet actif « ${configured_project:-non configuré} » différent de « ${PROJECT_ID} »."
account="$(gcloud config get-value account 2>/dev/null || true)"
[[ -n "${account}" && "${account}" != "(unset)" ]] || die "Aucun compte gcloud actif."

if ! existing_instance="$(gcloud compute instances list \
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
gcloud compute instances start "${INSTANCE_NAME}" \
    --project="${PROJECT_ID}" \
    --zone="${ZONE}" \
    --quiet

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

printf '[GARDE-FOU INVITÉ] Armement de shutdown -P +%s via SSH.\n' "${GUEST_SHUTDOWN_MINUTES}"
ssh_deadline=$((SECONDS + SSH_TIMEOUT_SECONDS))
guest_guard_output=""
while ((SECONDS < ssh_deadline)); do
    if guest_guard_output="$(gcloud compute ssh "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --quiet \
        --ssh-flag='-o ConnectTimeout=15' \
        --ssh-flag='-o BatchMode=yes' \
        --command="sudo -n shutdown -c >/dev/null 2>&1 || true; sudo -n shutdown -P +${GUEST_SHUTDOWN_MINUTES} 'Coupe-circuit E-HGP'; guard=\$(sudo -n shutdown --show 2>/dev/null || true); case \"\${guard}\" in ''|*'No scheduled shutdown'*) guard=\$(sudo -n cat /run/systemd/shutdown/scheduled 2>/dev/null || true) ;; esac; test -n \"\${guard}\"; printf '%s\\n' \"\${guard}\"; printf '__EHGP_GUEST_GUARD_VERIFIED__\\n'" 2>&1)"; then
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
