#!/usr/bin/env bash
set -euo pipefail

readonly DEFAULT_PROJECT_ID="devpod-gpu-exploration"
readonly DEFAULT_ZONE="europe-west4-a"
readonly DEFAULT_INSTANCE_NAME="ehgp-blackwell-spot"
readonly AI_CAPACITY_ZONE="europe-west4-ai1a"
readonly AI_CAPACITY_INSTANCE_NAME="ehgp-blackwell-spot-ai1a"
readonly EXPECTED_MACHINE_TYPE="g4-standard-48"
readonly EXPECTED_PROVISIONING_MODEL="SPOT"
readonly EXPECTED_TERMINATION_ACTION="STOP"
readonly EXPECTED_MAINTENANCE_POLICY="TERMINATE"
readonly MIN_ALLOWED_RUN_SECONDS=30
readonly MAX_ALLOWED_RUN_SECONDS=28800
readonly GCLOUD_READ_TIMEOUT_SECONDS=30
readonly GCLOUD_MUTATION_TIMEOUT_SECONDS=180
readonly GCLOUD_KILL_AFTER_SECONDS=10

readonly PROJECT_ID="${GCP_PROJECT_ID:-${DEFAULT_PROJECT_ID}}"
readonly ZONE="${GCP_ZONE:-${DEFAULT_ZONE}}"
readonly INSTANCE_NAME="${GCP_INSTANCE_NAME:-${DEFAULT_INSTANCE_NAME}}"

ASSUME_YES=0
YES_PROVIDED=0
MAX_RUN_DURATION_PROVIDED=0
REQUESTED_MAX_RUN_SECONDS=""
MUTATION_ATTEMPTED=0
MUTATION_CERTIFIED=0
LAST_OBSERVED_STATUS="unreadable"
LAST_OBSERVED_MAX_RUN_SECONDS="unreadable"
PRE_MUTATION_LAST_START_TIMESTAMP=""

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/set_max_run_duration_and_verify.sh --yes --max-run-duration-seconds SECONDES

Reconfigure uniquement maxRunDuration sur l'une des deux cibles E-HGP admises,
déjà arrêtée (TERMINATED). La durée doit être un entier explicite entre 30 et
28800 secondes. Le script vérifie g4-standard-48, project=e-hgp, SPOT, action
STOP, automaticRestart=false et maintenance TERMINATE avant la mutation, appelle
uniquement « gcloud compute instances set-scheduling » avec action STOP, puis
relit et recertifie la cible, la durée et l'absence de nouvelle génération.

--yes est obligatoire. Ce script ne démarre, n'arrête, ne crée et ne supprime
aucune VM.
EOF
}

while (($# > 0)); do
    case "$1" in
        --yes)
            ((YES_PROVIDED == 0)) || die "--yes ne peut être fourni qu'une fois."
            ASSUME_YES=1
            YES_PROVIDED=1
            shift
            ;;
        --max-run-duration-seconds)
            (($# >= 2)) || die "Valeur manquante après --max-run-duration-seconds."
            ((MAX_RUN_DURATION_PROVIDED == 0)) || \
                die "--max-run-duration-seconds ne peut être fourni qu'une fois."
            REQUESTED_MAX_RUN_SECONDS="$2"
            MAX_RUN_DURATION_PROVIDED=1
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

((ASSUME_YES == 1)) || \
    die "--yes est obligatoire pour cette mutation GCP explicitement autorisée."
((MAX_RUN_DURATION_PROVIDED == 1)) || \
    die "--max-run-duration-seconds est obligatoire."
[[ "${REQUESTED_MAX_RUN_SECONDS}" =~ ^[1-9][0-9]*$ ]] || \
    die "La durée doit être un entier canonique positif en secondes."
(( ${#REQUESTED_MAX_RUN_SECONDS} <= 5 )) || \
    die "La durée dépasse la limite absolue de huit heures."
REQUESTED_MAX_RUN_SECONDS=$((10#${REQUESTED_MAX_RUN_SECONDS}))
((REQUESTED_MAX_RUN_SECONDS >= MIN_ALLOWED_RUN_SECONDS && \
  REQUESTED_MAX_RUN_SECONDS <= MAX_ALLOWED_RUN_SECONDS)) || \
    die "La durée doit être comprise entre 30 et 28800 secondes."
readonly REQUESTED_MAX_RUN_SECONDS

[[ "${PROJECT_ID}" == "${DEFAULT_PROJECT_ID}" ]] || \
    die "Projet refusé : cette opération cible uniquement ${DEFAULT_PROJECT_ID}."
case "${ZONE}/${INSTANCE_NAME}" in
    "${DEFAULT_ZONE}/${DEFAULT_INSTANCE_NAME}"|\
    "${AI_CAPACITY_ZONE}/${AI_CAPACITY_INSTANCE_NAME}")
        ;;
    *)
        die "Cible hors allowlist : ${PROJECT_ID}/${ZONE}/${INSTANCE_NAME}."
        ;;
esac

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

instance_field() {
    local field="$1"
    gcloud_read compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --format="value(${field})"
}

print_control_command() {
    printf 'Commande de contrôle : gcloud compute instances describe %q --project=%q --zone=%q --format=%q\n' \
        "${INSTANCE_NAME}" "${PROJECT_ID}" "${ZONE}" \
        'value(status,scheduling.maxRunDuration.seconds,scheduling.instanceTerminationAction,scheduling.provisioningModel)' >&2
}

mutation_exit_guard() {
    local exit_code=$?
    local observed_status="unreadable"
    local observed_duration="unreadable"
    trap - EXIT HUP INT TERM
    if ((MUTATION_ATTEMPTED == 1 && MUTATION_CERTIFIED == 0)); then
        if observed_status="$(instance_field 'status' 2>/dev/null)"; then
            LAST_OBSERVED_STATUS="${observed_status}"
        fi
        if observed_duration="$(instance_field 'scheduling.maxRunDuration.seconds' 2>/dev/null)"; then
            LAST_OBSERVED_MAX_RUN_SECONDS="${observed_duration}"
        fi
        printf '[RECONFIGURATION NON CERTIFIÉE] projet=%s zone=%s instance=%s état=%s maxRunDuration=%ss demandé=%ss.\n' \
            "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" \
            "${LAST_OBSERVED_STATUS}" "${LAST_OBSERVED_MAX_RUN_SECONDS}" \
            "${REQUESTED_MAX_RUN_SECONDS}" >&2
        printf '[RECONFIGURATION NON CERTIFIÉE] Aucune VM n’a été démarrée, arrêtée, créée ou supprimée par ce script.\n' >&2
        print_control_command
        if ((exit_code == 0)); then
            exit_code=1
        fi
    fi
    exit "${exit_code}"
}

trap mutation_exit_guard EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

verify_target_guard() {
    local phase="$1"
    local expected_duration="$2"
    local action=""
    local automatic_restart=""
    local configured_seconds=""
    local generation=""
    local label=""
    local machine_type=""
    local maintenance_policy=""
    local provisioning_model=""
    local status=""

    status="$(instance_field 'status')" || return 1
    LAST_OBSERVED_STATUS="${status}"
    action="$(instance_field 'scheduling.instanceTerminationAction')" || return 1
    automatic_restart="$(instance_field 'scheduling.automaticRestart')" || return 1
    configured_seconds="$(instance_field 'scheduling.maxRunDuration.seconds')" || return 1
    LAST_OBSERVED_MAX_RUN_SECONDS="${configured_seconds}"
    label="$(instance_field 'labels.project')" || return 1
    machine_type="$(instance_field 'machineType.basename()')" || return 1
    maintenance_policy="$(instance_field 'scheduling.onHostMaintenance')" || return 1
    provisioning_model="$(instance_field 'scheduling.provisioningModel')" || return 1
    generation="$(instance_field 'lastStartTimestamp')" || return 1

    [[ "${status}" == "TERMINATED" ]] || return 1
    [[ "${action}" == "${EXPECTED_TERMINATION_ACTION}" ]] || return 1
    [[ "${automatic_restart,,}" == "false" ]] || return 1
    [[ "${configured_seconds}" =~ ^[1-9][0-9]*$ ]] || return 1
    (( ${#configured_seconds} <= 5 )) || return 1
    configured_seconds=$((10#${configured_seconds}))
    ((configured_seconds >= MIN_ALLOWED_RUN_SECONDS && \
      configured_seconds <= MAX_ALLOWED_RUN_SECONDS)) || return 1
    [[ "${label}" == "e-hgp" ]] || return 1
    [[ "${machine_type}" == "${EXPECTED_MACHINE_TYPE}" ]] || return 1
    [[ "${maintenance_policy}" == "${EXPECTED_MAINTENANCE_POLICY}" ]] || return 1
    [[ "${provisioning_model}" == "${EXPECTED_PROVISIONING_MODEL}" ]] || return 1
    [[ "${generation}" != *$'\n'* && "${generation}" != *$'\r'* ]] || return 1

    if [[ "${phase}" == "pre-mutation" ]]; then
        PRE_MUTATION_LAST_START_TIMESTAMP="${generation}"
    else
        [[ "${phase}" == "post-mutation" ]] || return 1
        [[ "${configured_seconds}" == "${expected_duration}" ]] || return 1
        [[ "${generation}" == "${PRE_MUTATION_LAST_START_TIMESTAMP}" ]] || return 1
    fi

    printf '[GARDE %s] cible=%s/%s/%s état=%s machine=%s provisioning=%s action=%s maxRunDuration=%ss.\n' \
        "${phase}" "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" \
        "${status}" "${machine_type}" "${provisioning_model}" "${action}" \
        "${configured_seconds}"
}

command -v gcloud >/dev/null 2>&1 || die "gcloud est introuvable."
command -v timeout >/dev/null 2>&1 || \
    die "GNU timeout est requis avant toute mutation GCP."
verify_gnu_timeout || \
    die "timeout doit être GNU coreutils et accepter --kill-after."

configured_project="$(gcloud_read config get-value project 2>/dev/null)" || \
    die "Projet gcloud actif illisible; mutation refusée."
[[ "${configured_project}" == "${DEFAULT_PROJECT_ID}" ]] || \
    die "Projet actif « ${configured_project:-non configuré} » différent de « ${DEFAULT_PROJECT_ID} »."
account="$(gcloud_read config get-value account 2>/dev/null)" || \
    die "Compte gcloud actif illisible; mutation refusée."
[[ -n "${account}" && "${account}" != "(unset)" ]] || \
    die "Aucun compte gcloud actif."

existing_instance="$(gcloud_read compute instances list \
    --project="${PROJECT_ID}" \
    --zones="${ZONE}" \
    --filter="name=${INSTANCE_NAME}" \
    --limit=1 \
    --format='value(name)')" || \
    die "Impossible de vérifier l’existence de ${INSTANCE_NAME}; mutation refusée."
[[ "${existing_instance}" == "${INSTANCE_NAME}" ]] || \
    die "Instance ${INSTANCE_NAME} absente ou inventaire ambigu dans ${PROJECT_ID}/${ZONE}."

verify_target_guard "pre-mutation" "" || \
    die "Garde préalable refusée : la cible doit être TERMINATED, g4-standard-48, project=e-hgp, SPOT, STOP, automaticRestart=false, maintenance TERMINATE et déjà bornée entre 30 s et 8 h."

printf '%s\n' \
    "[SET-SCHEDULING] Reconfiguration protégée demandée :" \
    "  compte              : ${account}" \
    "  projet              : ${PROJECT_ID}" \
    "  zone                : ${ZONE}" \
    "  instance            : ${INSTANCE_NAME}" \
    "  état certifié       : TERMINATED" \
    "  maxRunDuration cible: ${REQUESTED_MAX_RUN_SECONDS} s" \
    "  action              : STOP"

MUTATION_ATTEMPTED=1
if ! gcloud_mutation compute instances set-scheduling "${INSTANCE_NAME}" \
    --project="${PROJECT_ID}" \
    --zone="${ZONE}" \
    --max-run-duration="${REQUESTED_MAX_RUN_SECONDS}s" \
    --instance-termination-action="${EXPECTED_TERMINATION_ACTION}" \
    --quiet; then
    die "set-scheduling a échoué ou dépassé ${GCLOUD_MUTATION_TIMEOUT_SECONDS} s; résultat non certifié."
fi

verify_target_guard "post-mutation" "${REQUESTED_MAX_RUN_SECONDS}" || \
    die "La relecture post-mutation n’a pas recertifié la cible TERMINATED, ses invariants, sa génération et maxRunDuration=${REQUESTED_MAX_RUN_SECONDS}s."
MUTATION_CERTIFIED=1

printf '[OK] maxRunDuration=%ss recertifié sur la cible TERMINATED %s/%s/%s; aucune VM démarrée ou arrêtée.\n' \
    "${REQUESTED_MAX_RUN_SECONDS}" "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}"
