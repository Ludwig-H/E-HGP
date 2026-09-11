#!/usr/bin/env bash
set -euo pipefail

readonly INSTANCE_NAME="${GCP_INSTANCE_NAME:-ehgp-blackwell-spot}"
readonly ZONE="${GCP_ZONE:-europe-west4-a}"
readonly DEFAULT_PROJECT_ID="devpod-gpu-exploration"
readonly STOP_TIMEOUT_SECONDS=180
readonly GCLOUD_READ_TIMEOUT_SECONDS=30
readonly GCLOUD_MUTATION_TIMEOUT_SECONDS=180
readonly GCLOUD_KILL_AFTER_SECONDS=10

PROJECT_ID="${GCP_PROJECT_ID:-${DEFAULT_PROJECT_ID}}"
ASSUME_YES=0
EXPECTED_LAST_START_TIMESTAMP=""
EXPECTED_GENERATION_PROVIDED=0

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/stop_and_verify.sh [--yes] [--expected-last-start-timestamp HORODATAGE]

Vérifie le label project=e-hgp, arrête uniquement la VM ciblée et attend l'état
GCE TERMINATED. Les autres VM labellisées sont inventoriées sans être modifiées.
Le mode est interactif par défaut. --yes ne désactive aucune vérification. L'option
de génération est utilisée par les orchestrateurs; l'usage manuel peut l'omettre.
EOF
}

while (($# > 0)); do
    case "$1" in
        --yes)
            ASSUME_YES=1
            shift
            ;;
        --expected-last-start-timestamp)
            (($# >= 2)) || die "Valeur manquante après --expected-last-start-timestamp."
            ((EXPECTED_GENERATION_PROVIDED == 0)) || \
                die "--expected-last-start-timestamp ne peut être fourni qu'une fois."
            EXPECTED_LAST_START_TIMESTAMP="$2"
            EXPECTED_GENERATION_PROVIDED=1
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

instance_status() {
    instance_field 'status'
}

instance_label() {
    instance_field 'labels.project'
}

instance_generation() {
    instance_field 'lastStartTimestamp'
}

verify_expected_generation() {
    local observed_generation=""
    ((EXPECTED_GENERATION_PROVIDED == 1)) || return 0
    observed_generation="$(instance_generation)" || \
        die "Génération lastStartTimestamp illisible; arrêt refusé."
    if [[ "${observed_generation}" != "${EXPECTED_LAST_START_TIMESTAMP}" ]]; then
        die "Génération différente : lastStartTimestamp=${observed_generation:-vide}, attendu=${EXPECTED_LAST_START_TIMESTAMP}; arrêt refusé pour protéger une session concurrente."
    fi
}

report_other_labeled_instances() {
    local inventory active_count name zone status
    if ! inventory="$(gcloud_read compute instances list \
        --project="${PROJECT_ID}" \
        --filter='labels.project=e-hgp' \
        --format='csv[no-heading](name,zone.basename(),status)')"; then
        printf '[ATTENTION] Inventaire des autres VM project=e-hgp illisible; aucune autre ressource n’a été modifiée.\n' >&2
        return 0
    fi

    active_count=0
    while IFS=',' read -r name zone status; do
        [[ -n "${name}" ]] || continue
        if [[ "${name}" == "${INSTANCE_NAME}" && "${zone}" == "${ZONE}" ]]; then
            continue
        fi
        if [[ "${status}" != "TERMINATED" ]]; then
            if ((active_count == 0)); then
                printf '[ATTENTION] Autres VM project=e-hgp actives (signalement seulement) :\n' >&2
            fi
            printf '  - projet=%s zone=%s instance=%s état=%s\n' \
                "${PROJECT_ID}" "${zone}" "${name}" "${status}" >&2
            active_count=$((active_count + 1))
        fi
    done <<<"${inventory}"

    if ((active_count > 0)); then
        printf '[ATTENTION] %s autre(s) VM active(s) détectée(s); elles peuvent appartenir à une session concurrente et ne sont pas arrêtées.\n' \
            "${active_count}" >&2
    else
        printf '[INFO] Aucune autre VM project=e-hgp active détectée.\n'
    fi
}

command -v gcloud >/dev/null 2>&1 || die "gcloud est introuvable."
command -v timeout >/dev/null 2>&1 || die "GNU timeout est requis avant toute mutation GCP."
verify_gnu_timeout || die "timeout doit être l'implémentation GNU compatible avec la gestion du groupe de processus et --kill-after."
if ((EXPECTED_GENERATION_PROVIDED == 1)); then
    [[ -n "${EXPECTED_LAST_START_TIMESTAMP}" && \
        "${EXPECTED_LAST_START_TIMESTAMP}" != *$'\n'* && \
        "${EXPECTED_LAST_START_TIMESTAMP}" != *$'\r'* ]] || \
        die "--expected-last-start-timestamp doit être un horodatage non vide sans saut de ligne."
fi

configured_project="$(gcloud_read config get-value project 2>/dev/null || true)"
if [[ "${configured_project}" != "${PROJECT_ID}" ]]; then
    die "Projet actif « ${configured_project:-non configuré} » différent de « ${PROJECT_ID} »."
fi

if ! existing_instance="$(gcloud_read compute instances list \
    --project="${PROJECT_ID}" \
    --zones="${ZONE}" \
    --filter="name=${INSTANCE_NAME}" \
    --limit=1 \
    --format='value(name)')"; then
    die "Impossible de vérifier l’existence de ${INSTANCE_NAME}; fermeture non certifiée."
fi

if [[ -z "${existing_instance}" ]]; then
    report_other_labeled_instances
    die "Instance cible ${INSTANCE_NAME} absente de ${PROJECT_ID}/${ZONE}; l'état TERMINATED ne peut pas être certifié."
fi
[[ "${existing_instance}" == "${INSTANCE_NAME}" ]] || die "Résultat d’inventaire ambigu pour ${INSTANCE_NAME}."

label="$(instance_label)" || die "Label de ${INSTANCE_NAME} illisible; arrêt refusé."
[[ "${label}" == "e-hgp" ]] || \
    die "La cible ${PROJECT_ID}/${ZONE}/${INSTANCE_NAME} ne porte pas le label project=e-hgp; arrêt refusé."
verify_expected_generation

status="$(instance_status)" || die "État de ${INSTANCE_NAME} illisible dans ${PROJECT_ID}/${ZONE}."
if [[ "${status}" == "TERMINATED" ]]; then
    verify_expected_generation
    printf '[OK] Cible %s labellisée project=e-hgp et déjà arrêtée (état GCE TERMINATED).\n' "${INSTANCE_NAME}"
    report_other_labeled_instances
    exit 0
fi

printf '[STOP] État actuel de %s : %s.\n' "${INSTANCE_NAME}" "${status}"
if [[ "${status}" != "STOPPING" ]] && ((ASSUME_YES == 0)); then
    [[ -t 0 ]] || die "Confirmation interactive requise; utilisez --yes seulement après autorisation explicite."
    expected_confirmation="STOPPER ${INSTANCE_NAME}"
    read -r -p "Tapez exactement « ${expected_confirmation} » : " confirmation
    [[ "${confirmation}" == "${expected_confirmation}" ]] || die "Arrêt annulé."
fi

if [[ "${status}" != "STOPPING" ]]; then
    verify_expected_generation
    if ! gcloud_mutation compute instances stop "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --quiet; then
        die "La commande d'arrêt GCP a échoué ou dépassé ${GCLOUD_MUTATION_TIMEOUT_SECONDS} s."
    fi
fi

deadline=$((SECONDS + STOP_TIMEOUT_SECONDS))
while ((SECONDS < deadline)); do
    if current_status="$(instance_status 2>/dev/null)"; then
        status="${current_status}"
        if [[ "${status}" == "TERMINATED" ]]; then
            verify_expected_generation
            printf '[OK] Cible %s arrêtée et vérifiée (état GCE TERMINATED).\n' "${INSTANCE_NAME}"
            report_other_labeled_instances
            exit 0
        fi
        printf '[ATTENTE] État GCE : %s\n' "${status}"
    else
        printf '[ATTENTION] Lecture GCE transitoirement impossible ; nouvel essai dans 5 s.\n' >&2
    fi
    sleep 5
done

die "Délai dépassé : état final ${status}. Vérifiez immédiatement dans la console GCP."
