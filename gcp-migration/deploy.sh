#!/usr/bin/env bash
set -euo pipefail

readonly INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
readonly ZONE="${GCP_ZONE:-europe-west4-a}"
readonly REGION="${ZONE%-*}"
readonly MACHINE_TYPE="g4-standard-48"
readonly IMAGE_FAMILY="common-cu129-ubuntu-2204-nvidia-580"
readonly IMAGE_PROJECT="deeplearning-platform-release"
readonly DEFAULT_PROJECT_ID="devpod-gpu-exploration"
readonly EXPECTED_MAINTENANCE_POLICY="TERMINATE"
readonly EXPECTED_PROVISIONING_MODEL="SPOT"
readonly BOOT_DISK_IOPS=3600
readonly BOOT_DISK_THROUGHPUT=290
readonly MIN_ALLOWED_RUN_SECONDS=30
readonly MAX_ALLOWED_RUN_SECONDS=28800
readonly GUARD_TIMEOUT_SECONDS=60
readonly TIMESTAMP_TOLERANCE_SECONDS=300

PROJECT_ID="${GCP_PROJECT_ID:-${DEFAULT_PROJECT_ID}}"
MAX_RUN_DURATION="${GCP_MAX_RUN_DURATION:-8h}"
NETWORK_INTERFACE="${GCP_NETWORK_INTERFACE:-network=default,nic-type=GVNIC}"
RUNTIME_SERVICE_ACCOUNT="${GCP_RUNTIME_SERVICE_ACCOUNT:-}"
CREATE_REQUEST_EPOCH=""

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

duration_to_seconds() {
    local value="$1"
    local amount unit multiplier

    if [[ ! "${value}" =~ ^([1-9][0-9]*)(s|m|h)$ ]]; then
        die "Durée invalide « ${value} ». Utilisez un entier positif suivi de s, m ou h (par exemple 8h)."
    fi

    amount="${BASH_REMATCH[1]}"
    unit="${BASH_REMATCH[2]}"
    ((${#amount} <= 5)) || die "Durée ${value} supérieure à la limite absolue de 8h."
    amount=$((10#${amount}))
    case "${unit}" in
        s) multiplier=1 ;;
        m) multiplier=60 ;;
        h) multiplier=3600 ;;
    esac
    printf '%s\n' "$((amount * multiplier))"
}

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

verify_runtime_guard() {
    local action automatic_restart configured_seconds label machine_type
    local maintenance_policy provisioning_model start_timestamp start_epoch computed_deadline
    local termination_timestamp termination_epoch now maximum_deadline

    action="$(instance_field 'scheduling.instanceTerminationAction')" || return 1
    automatic_restart="$(instance_field 'scheduling.automaticRestart')" || return 1
    configured_seconds="$(instance_field 'scheduling.maxRunDuration.seconds')" || return 1
    label="$(instance_field 'labels.project')" || return 1
    machine_type="$(instance_field 'machineType.basename()')" || return 1
    maintenance_policy="$(instance_field 'scheduling.onHostMaintenance')" || return 1
    provisioning_model="$(instance_field 'scheduling.provisioningModel')" || return 1
    start_timestamp="$(instance_field 'lastStartTimestamp')" || return 1
    termination_timestamp="$(instance_field 'terminationTimestamp')" || return 1

    [[ "${action}" == "STOP" ]] || return 1
    [[ "${automatic_restart,,}" == "false" ]] || return 1
    [[ "${configured_seconds}" =~ ^[0-9]+$ ]] || return 1
    ((configured_seconds >= MIN_ALLOWED_RUN_SECONDS && configured_seconds <= MAX_ALLOWED_RUN_SECONDS)) || return 1
    ((configured_seconds == MAX_RUN_SECONDS)) || return 1
    [[ "${label}" == "e-hgp" ]] || return 1
    [[ "${machine_type}" == "${MACHINE_TYPE}" ]] || return 1
    [[ "${maintenance_policy}" == "${EXPECTED_MAINTENANCE_POLICY}" ]] || return 1
    [[ "${provisioning_model}" == "${EXPECTED_PROVISIONING_MODEL}" ]] || return 1
    [[ -n "${start_timestamp}" ]] || return 1
    [[ "${CREATE_REQUEST_EPOCH}" =~ ^[0-9]+$ ]] || return 1
    [[ "${start_timestamp}" != *$'\n'* && "${start_timestamp}" != *$'\r'* ]] || return 1
    [[ "${termination_timestamp}" != *$'\n'* && \
        "${termination_timestamp}" != *$'\r'* ]] || return 1

    start_epoch="$(timestamp_to_epoch "${start_timestamp}")" || return 1
    now="$(date +%s)"
    ((start_epoch >= CREATE_REQUEST_EPOCH - TIMESTAMP_TOLERANCE_SECONDS)) || return 1
    ((start_epoch <= now + TIMESTAMP_TOLERANCE_SECONDS)) || return 1
    computed_deadline=$((start_epoch + configured_seconds))
    maximum_deadline=$((now + configured_seconds + TIMESTAMP_TOLERANCE_SECONDS))
    ((computed_deadline > now && computed_deadline <= maximum_deadline)) || return 1

    if [[ -n "${termination_timestamp}" ]]; then
        termination_epoch="$(timestamp_to_epoch "${termination_timestamp}")" || return 1
        ((termination_epoch > now && termination_epoch <= maximum_deadline)) || return 1
        ((termination_epoch >= computed_deadline - TIMESTAMP_TOLERANCE_SECONDS && termination_epoch <= computed_deadline + TIMESTAMP_TOLERANCE_SECONDS)) || return 1
    else
        [[ "${ZONE}" == *-ai* ]] || return 1
    fi

    printf '[GARDE-FOU] action=%s, maxRunDuration=%ss, échéance calculée=%s' \
        "${action}" "${configured_seconds}" "${computed_deadline}"
    if [[ -n "${termination_timestamp}" ]]; then
        printf ', terminationTimestamp=%s' "${termination_timestamp}"
    else
        printf ', terminationTimestamp non exposé; échéance calculée certifiée'
    fi
    printf '\n'
}

creation_attempted=0
creation_verified=0
emergency_stop_on_exit() {
    local exit_code=$?
    if ((exit_code != 0 && creation_attempted == 1 && creation_verified == 0)); then
        printf '[URGENCE] Création non certifiée : tentative d’arrêt immédiat de %s.\n' "${INSTANCE_NAME}" >&2
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

MAX_RUN_SECONDS="$(duration_to_seconds "${MAX_RUN_DURATION}")"
if ((MAX_RUN_SECONDS < MIN_ALLOWED_RUN_SECONDS)); then
    die "GCP_MAX_RUN_DURATION=${MAX_RUN_DURATION} est inférieur au minimum GCE de 30 secondes."
fi
if ((MAX_RUN_SECONDS > MAX_ALLOWED_RUN_SECONDS)); then
    die "GCP_MAX_RUN_DURATION=${MAX_RUN_DURATION} dépasse la limite absolue de 8h."
fi

configured_project="$(gcloud config get-value project 2>/dev/null || true)"
if [[ -z "${configured_project}" || "${configured_project}" == "(unset)" ]]; then
    die "Aucun projet gcloud configuré. Exécutez : gcloud config set project ${PROJECT_ID}"
fi
if [[ "${configured_project}" != "${PROJECT_ID}" ]]; then
    die "Le projet actif (${configured_project}) diffère de ${PROJECT_ID}. Configurez-le explicitement ou exportez GCP_PROJECT_ID."
fi

account="$(gcloud config get-value account 2>/dev/null || true)"
[[ -n "${account}" && "${account}" != "(unset)" ]] || die "Aucun compte gcloud actif."

if [[ "${ZONE}" == *-ai* ]]; then
    ai_zone_status="$(gcloud compute preview-features describe ai-zones-visibility \
        --project="${PROJECT_ID}" \
        --format='value(activationStatus)')" || \
        die "Impossible de lire l'activation de ai-zones-visibility."
    if [[ "${ai_zone_status}" != "ENABLED" && \
        "${ai_zone_status}" != "ACTIVATION_STATE_ENABLED" ]]; then
        die "La zone IA ${ZONE} exige ai-zones-visibility=enabled; activation automatique interdite."
    fi
fi

require_external_address=0
if [[ "${NETWORK_INTERFACE}" != *"no-address"* ]]; then
    require_external_address=1
fi
GCP_PROJECT_ID="${PROJECT_ID}" GCP_REGION="${REGION}" GCP_ZONE="${ZONE}" \
GCP_REQUIRE_EXTERNAL_ADDRESS="${require_external_address}" \
    "$(dirname "${BASH_SOURCE[0]}")/check_quotas.sh" || \
    die "Les quotas strictement nécessaires à une création G4 Spot ne sont pas disponibles."

if ! resolved_image="$(gcloud compute images describe-from-family "${IMAGE_FAMILY}" \
    --project="${IMAGE_PROJECT}" \
    --format='value(name)')"; then
    die "Impossible de résoudre la famille d'image ${IMAGE_PROJECT}/${IMAGE_FAMILY}."
fi
[[ -n "${resolved_image}" ]] || die "La famille ${IMAGE_FAMILY} n'a renvoyé aucune image."

if ! existing_instance="$(gcloud compute instances list \
    --project="${PROJECT_ID}" \
    --zones="${ZONE}" \
    --filter="name=${INSTANCE_NAME}" \
    --limit=1 \
    --format='value(name)')"; then
    die "Impossible de vérifier si ${INSTANCE_NAME} existe déjà ; création refusée."
fi
if [[ -n "${existing_instance}" ]]; then
    die "L'instance ${INSTANCE_NAME} existe déjà. Ce script ne la supprime ni ne la recrée."
fi

printf '%s\n' \
    "[DEPLOY] Création demandée :" \
    "  compte      : ${account}" \
    "  projet      : ${PROJECT_ID}" \
    "  instance    : ${INSTANCE_NAME}" \
    "  zone        : ${ZONE}" \
    "  machine     : ${MACHINE_TYPE} (48 vCPU, 180 Go RAM, 1 x RTX PRO 6000 Blackwell 96 Go)" \
    "  image       : ${resolved_image} (${IMAGE_FAMILY}, CUDA 12.9 / pilote 580)" \
    "  réseau      : ${NETWORK_INTERFACE}" \
    "  identité VM : ${RUNTIME_SERVICE_ACCOUNT:-aucune}" \
    "  Spot        : oui, action de terminaison STOP" \
    "  coupe-circuit GCE : ${MAX_RUN_DURATION}"

[[ -t 0 ]] || die "Confirmation interactive requise pour créer une ressource facturable."
expected_confirmation="CREER ${INSTANCE_NAME} DANS ${PROJECT_ID}"
read -r -p "Tapez exactement « ${expected_confirmation} » : " confirmation
[[ "${confirmation}" == "${expected_confirmation}" ]] || die "Création annulée."

service_account_args=(--no-service-account --no-scopes)
if [[ -n "${RUNTIME_SERVICE_ACCOUNT}" ]]; then
    service_account_args=(
        "--service-account=${RUNTIME_SERVICE_ACCOUNT}"
        "--scopes=cloud-platform"
    )
fi

CREATE_REQUEST_EPOCH="$(date +%s)" || die "Impossible d'horodater la demande de création."
[[ "${CREATE_REQUEST_EPOCH}" =~ ^[0-9]+$ ]] || die "Horodatage de création invalide."
creation_attempted=1
gcloud compute instances create "${INSTANCE_NAME}" \
    --project="${PROJECT_ID}" \
    --zone="${ZONE}" \
    --machine-type="${MACHINE_TYPE}" \
    --provisioning-model="SPOT" \
    --instance-termination-action="STOP" \
    --max-run-duration="${MAX_RUN_DURATION}" \
    --maintenance-policy="TERMINATE" \
    --no-restart-on-failure \
    --image="${resolved_image}" \
    --image-project="${IMAGE_PROJECT}" \
    --boot-disk-size="100GB" \
    --boot-disk-type="hyperdisk-balanced" \
    --boot-disk-provisioned-iops="${BOOT_DISK_IOPS}" \
    --boot-disk-provisioned-throughput="${BOOT_DISK_THROUGHPUT}" \
    --network-interface="${NETWORK_INTERFACE}" \
    --metadata="enable-oslogin=TRUE" \
    --labels="project=e-hgp,role=gpu-benchmark,managed-by=manual-script" \
    --deletion-protection \
    "${service_account_args[@]}"

guard_deadline=$((SECONDS + GUARD_TIMEOUT_SECONDS))
guard_verified=0
while ((SECONDS < guard_deadline)); do
    if verify_runtime_guard; then
        guard_verified=1
        break
    fi
    printf '[ATTENTE] Matérialisation du coupe-circuit GCE; nouvel essai dans 5 s.\n'
    sleep 5
done
if ((guard_verified == 0)); then
    die "La VM a été créée, mais son action STOP, sa durée ou son échéance n’a pas pu être certifiée."
fi

printf '[SÉCURITÉ] La création démarre la VM : arrêt immédiat avant toute session de calcul.\n'
GCP_PROJECT_ID="${PROJECT_ID}" \
GCP_INSTANCE_NAME="${INSTANCE_NAME}" \
GCP_ZONE="${ZONE}" \
"$(dirname "${BASH_SOURCE[0]}")/stop_and_verify.sh" --yes || \
    die "La VM a été créée, mais son arrêt post-création n’a pas pu être certifié."
creation_verified=1

printf '[SUCCÈS] %s créée, coupe-circuit GCE certifié, puis arrêtée. Utilisez start_and_verify.sh pour ouvrir une session.\n' "${INSTANCE_NAME}"
