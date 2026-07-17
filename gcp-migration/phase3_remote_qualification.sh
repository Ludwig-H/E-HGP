#!/usr/bin/env bash
set -Eeuo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly PREFLIGHT_SCRIPT="${SCRIPT_DIR}/blackwell_preflight.sh"
readonly DOCKERFILE_RELATIVE="containers/cuda12.9-sm120.Dockerfile"
readonly ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase3_qualification.py"
readonly BASE_IMAGE_REF="nvidia/cuda:12.9.2-devel-ubuntu24.04@sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
readonly CONTAINER_REPOSITORY="/workspace/repository"
readonly CONTAINER_BUILD="${CONTAINER_REPOSITORY}/build"
readonly CONTAINER_SOURCE="${CONTAINER_REPOSITORY}/morsehgp3d"
readonly CONTAINER_RESULTS="/results"
readonly RUNTIME_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_phase3_runtime"
readonly RUNTIME_PATH="${CONTAINER_REPOSITORY}/${RUNTIME_RELATIVE}"
readonly MODULE_DIR="${CONTAINER_BUILD}/morsehgp3d-cuda-release"
readonly GUEST_GUARD_MIN_REMAINING_SECONDS=1800
readonly GUEST_GUARD_MAX_REMAINING_SECONDS=2820
readonly WORK_RESERVE_SECONDS=1800
readonly FAILURE_LOG_MAX_LINES=240
readonly FAILURE_LOG_MAX_BYTES=65536
readonly DOCKER_INFO_MAX_ATTEMPTS=6
readonly DOCKER_INFO_RETRY_SECONDS=5
readonly DOCKER_PROBE_TIMEOUT_SECONDS=5
readonly DOCKER_DIAGNOSTIC_TIMEOUT_SECONDS=10
readonly SUDO_DOCKER_BIN="/usr/bin/docker"
readonly BUILDX_PLUGIN="/usr/libexec/docker/cli-plugins/docker-buildx"
readonly BUILDX_TEST_BIN="/usr/bin/test"
readonly BUILDX_STAT_BIN="/usr/bin/stat"

ASSUME_YES=0
OUTPUT_RAW=""
OUTPUT_PATH=""
OUTPUT_PARENT=""
OUTPUT_BASE=""
GCE_DEADLINE_RAW=""
GCE_DEADLINE_EPOCH=0
WORK_DEADLINE_EPOCH=0
GUEST_SHUTDOWN_EPOCH=0
REPOSITORY_ROOT=""
SESSION_DIR=""
PUBLISH_TEMP=""
SESSION_CREATED=0
DOCKER_IDENTITY=""
BUILDX_CONFIG=""

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/phase3_remote_qualification.sh --yes --gce-deadline-epoch EPOCH --output /CHEMIN/ABSOLU.json

Worker invité non interactif de qualification de l'environnement CUDA Phase 3.
Il exige un arrêt invité déjà planifié, ne pilote jamais le cycle de vie GCP et
publie un unique objet JSON atomique hors du worktree après tous les contrôles.
EOF
}

while (($# > 0)); do
    case "$1" in
        --yes)
            ASSUME_YES=1
            shift
            ;;
        --output)
            (($# >= 2)) || die "Valeur manquante après --output."
            [[ -z "${OUTPUT_RAW}" ]] || die "--output ne peut être fourni qu'une fois."
            OUTPUT_RAW="$2"
            shift 2
            ;;
        --gce-deadline-epoch)
            (($# >= 2)) || die "Valeur manquante après --gce-deadline-epoch."
            [[ -z "${GCE_DEADLINE_RAW}" ]] || die "--gce-deadline-epoch ne peut être fourni qu'une fois."
            GCE_DEADLINE_RAW="$2"
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

((ASSUME_YES == 1)) || die "--yes est obligatoire pour ce worker distant explicitement autorisé."
[[ -n "${OUTPUT_RAW}" ]] || die "--output ABSOLU est obligatoire."
[[ "${GCE_DEADLINE_RAW}" =~ ^[0-9]{10}$ ]] || \
    die "--gce-deadline-epoch doit être un epoch UTC positif sur dix chiffres."
GCE_DEADLINE_EPOCH=$((10#${GCE_DEADLINE_RAW}))
WORK_DEADLINE_EPOCH=$((GCE_DEADLINE_EPOCH - WORK_RESERVE_SECONDS))
now_epoch="$(date +%s)" || die "Horloge invitée illisible."
[[ "${now_epoch}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique."
((WORK_DEADLINE_EPOCH > now_epoch)) || \
    die "La deadline de travail GCE-30 min est déjà atteinte; aucune unité ne sera lancée."
case "${OUTPUT_RAW}" in
    /*) ;;
    *) die "--output doit être un chemin absolu." ;;
esac

command -v git >/dev/null 2>&1 || die "git est introuvable."
command -v id >/dev/null 2>&1 || die "id est introuvable."
command -v python3 >/dev/null 2>&1 || die "python3 est introuvable."
command -v mktemp >/dev/null 2>&1 || die "mktemp est introuvable."
command -v tail >/dev/null 2>&1 || die "tail est introuvable."
command -v sleep >/dev/null 2>&1 || die "sleep est introuvable."
command -v timeout >/dev/null 2>&1 || die "GNU timeout est introuvable."
[[ -x "${PREFLIGHT_SCRIPT}" ]] || die "Preflight absent ou non exécutable : ${PREFLIGHT_SCRIPT}."

REPOSITORY_ROOT="$(git -C "${SCRIPT_DIR}/.." rev-parse --show-toplevel 2>/dev/null)" || \
    die "Impossible d'identifier le clone Git."
REPOSITORY_ROOT="$(cd -- "${REPOSITORY_ROOT}" && pwd -P)" || die "Racine Git illisible."
[[ "${SCRIPT_DIR}" == "${REPOSITORY_ROOT}/gcp-migration" ]] || \
    die "Le worker doit être exécuté depuis le clone canonique qui le contient."

OUTPUT_PARENT="$(dirname -- "${OUTPUT_RAW}")"
OUTPUT_BASE="$(basename -- "${OUTPUT_RAW}")"
[[ -n "${OUTPUT_BASE}" && "${OUTPUT_BASE}" != "." && "${OUTPUT_BASE}" != ".." ]] || \
    die "Nom d'artefact invalide."
[[ -d "${OUTPUT_PARENT}" ]] || die "Le répertoire parent de --output doit déjà exister."
OUTPUT_PARENT="$(cd -- "${OUTPUT_PARENT}" && pwd -P)" || die "Parent de sortie illisible."
OUTPUT_PATH="${OUTPUT_PARENT}/${OUTPUT_BASE}"
case "${OUTPUT_PATH}/" in
    "${REPOSITORY_ROOT}/"*) die "--output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
esac
[[ ! -e "${OUTPUT_PATH}" && ! -L "${OUTPUT_PATH}" ]] || \
    die "La sortie doit être inexistante : ${OUTPUT_PATH}."

worktree_status="$(git -C "${REPOSITORY_ROOT}" status --porcelain --untracked-files=normal)" || \
    die "Impossible de vérifier la propreté du clone Git."
[[ -z "${worktree_status}" ]] || die "Le clone Git doit être entièrement propre avant qualification."
HEAD_SHA="$(git -C "${REPOSITORY_ROOT}" rev-parse HEAD)" || die "HEAD Git illisible."
[[ "${HEAD_SHA}" =~ ^[0-9a-f]{40}$ ]] || die "SHA Git non canonique : ${HEAD_SHA}."
verified_sha="$(git -C "${REPOSITORY_ROOT}" rev-parse --verify "${HEAD_SHA}^{commit}")" || \
    die "Le SHA ${HEAD_SHA} n'identifie pas un commit local."
[[ "${verified_sha}" == "${HEAD_SHA}" ]] || die "Le commit qualifié n'est pas canonique."

readonly DOCKERFILE="${REPOSITORY_ROOT}/${DOCKERFILE_RELATIVE}"
readonly DOCKER_CONTEXT="${REPOSITORY_ROOT}/containers"
readonly ASSEMBLER="${REPOSITORY_ROOT}/${ASSEMBLER_RELATIVE}"
[[ -f "${DOCKERFILE}" && ! -L "${DOCKERFILE}" ]] || \
    die "Dockerfile Phase 3 absent ou symbolique : ${DOCKERFILE}."
[[ "$(sed -n '1p' "${DOCKERFILE}")" == "FROM ${BASE_IMAGE_REF}" ]] || \
    die "Le Dockerfile Phase 3 doit partir de l'image CUDA épinglée ${BASE_IMAGE_REF}."
[[ -f "${ASSEMBLER}" && ! -L "${ASSEMBLER}" ]] || \
    die "Assembleur Phase 3 absent ou symbolique : ${ASSEMBLER}."

cleanup() {
    local original_status=$?
    trap - EXIT HUP INT TERM
    if [[ -n "${PUBLISH_TEMP}" && -f "${PUBLISH_TEMP}" ]]; then
        rm -f -- "${PUBLISH_TEMP}" || true
    fi
    if [[ -n "${SESSION_DIR}" && -n "${BUILDX_CONFIG}" && \
        "${BUILDX_CONFIG}" == "${SESSION_DIR}/buildx-config" && \
        -e "${BUILDX_CONFIG}" ]]; then
        if [[ "${DOCKER_IDENTITY}" == "sudo" ]] && command -v sudo >/dev/null 2>&1; then
            sudo -n -- /usr/bin/rm -rf -- "${BUILDX_CONFIG}" || true
        else
            rm -rf -- "${BUILDX_CONFIG}" || true
        fi
    fi
    if ((SESSION_CREATED == 1)) && [[ -n "${SESSION_DIR}" && -d "${SESSION_DIR}" ]]; then
        case "${SESSION_DIR}" in
            "${TMPDIR:-/tmp}"/morsehgp3d-phase3-worker.*)
                rm -rf -- "${SESSION_DIR}" || true
                ;;
            *)
                printf '[ATTENTION] Nettoyage refusé pour un temporaire non canonique : %s\n' \
                    "${SESSION_DIR}" >&2
                ;;
        esac
    fi
    exit "${original_status}"
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

SESSION_DIR="$(mktemp -d "${TMPDIR:-/tmp}/morsehgp3d-phase3-worker.XXXXXXXX")" || \
    die "Impossible de créer le temporaire borné de qualification."
SESSION_CREATED=1
BUILDX_CONFIG="${SESSION_DIR}/buildx-config"
mkdir -p -- "${SESSION_DIR}/build" "${SESSION_DIR}/logs" "${SESSION_DIR}/results" \
    "${BUILDX_CONFIG}"
chmod 700 "${BUILDX_CONFIG}"
export BUILDX_CONFIG
PUBLISH_TEMP="$(mktemp "${OUTPUT_PARENT}/.${OUTPUT_BASE}.XXXXXXXX.partial")" || \
    die "Impossible de réserver l'artefact temporaire atomique."
chmod 600 "${PUBLISH_TEMP}"

readonly BUILD_DIR="${SESSION_DIR}/build"
readonly LOG_DIR="${SESSION_DIR}/logs"
readonly RESULT_DIR="${SESSION_DIR}/results"
readonly CONTAINER_HOME="${CONTAINER_BUILD}/.phase3-home"
readonly GUARD_LOG="${LOG_DIR}/guest-shutdown-guard.log"
readonly PREFLIGHT_LOG="${LOG_DIR}/preflight.log"
readonly DOCKER_LOG="${LOG_DIR}/docker-info.log"
readonly BUILD_LOG="${LOG_DIR}/docker-build.log"
readonly RELEASE_LOG="${LOG_DIR}/cuda-release.log"
readonly AUDIT_LOG="${LOG_DIR}/cuda-audit.log"
readonly RUNTIME_LOG="${LOG_DIR}/runtime.log"
readonly BINDING_LOG="${LOG_DIR}/binding.log"
readonly ELF_LOG="${LOG_DIR}/cuobjdump-elf.log"
readonly PTX_LOG="${LOG_DIR}/cuobjdump-ptx.log"
readonly PTX_STDERR_LOG="${LOG_DIR}/cuobjdump-ptx.stderr.log"
readonly SANITIZER_LOG="${LOG_DIR}/compute-sanitizer.log"
readonly RUNTIME_JSONL="${RESULT_DIR}/runtime.jsonl"
readonly SANITIZER_JSONL="${RESULT_DIR}/sanitizer-runtime.jsonl"
readonly IID_FILE="${SESSION_DIR}/docker-image.iid"

mkdir -p -- "${BUILD_DIR}/.phase3-home"

readonly CONTAINER_UID="$(id -u)"
readonly CONTAINER_GID="$(id -g)"
[[ "${CONTAINER_UID}" =~ ^[0-9]+$ && "${CONTAINER_GID}" =~ ^[0-9]+$ ]] || \
    die "UID/GID invité illisible; exécution conteneur refusée."
read -r -a CONTAINER_GROUP_IDS <<<"$(id -G)"
declare -a DOCKER_IDENTITY_ARGS=(--user "${CONTAINER_UID}:${CONTAINER_GID}")
for group_id in "${CONTAINER_GROUP_IDS[@]}"; do
    [[ "${group_id}" =~ ^[0-9]+$ ]] || die "Groupe invité non numérique : ${group_id}."
    DOCKER_IDENTITY_ARGS+=(--group-add "${group_id}")
done

guard_is_scheduled() {
    local evidence="$1"
    local mode=""
    local scheduled_usec=""
    local scheduled_epoch=0
    local now_epoch=0
    local remaining_seconds=0
    [[ -s "${evidence}" ]] || return 1
    [[ "$(grep -Ec '^MODE=' "${evidence}" || true)" == "1" ]] || return 1
    [[ "$(grep -Ec '^USEC=' "${evidence}" || true)" == "1" ]] || return 1
    mode="$(sed -n 's/^MODE=\(.*\)$/\1/p' "${evidence}")"
    scheduled_usec="$(sed -n 's/^USEC=\([0-9][0-9]*\)$/\1/p' "${evidence}")"
    [[ "${scheduled_usec}" =~ ^[0-9]{1,18}$ ]] || return 1
    [[ "${mode}" == "poweroff" ]] || return 1
    scheduled_epoch=$((10#${scheduled_usec} / 1000000))
    now_epoch="$(date +%s)"
    remaining_seconds=$((scheduled_epoch - now_epoch))
    ((remaining_seconds >= GUEST_GUARD_MIN_REMAINING_SECONDS)) || return 1
    ((remaining_seconds <= GUEST_GUARD_MAX_REMAINING_SECONDS)) || return 1
    ((scheduled_epoch <= GCE_DEADLINE_EPOCH)) || return 1
    GUEST_SHUTDOWN_EPOCH="${scheduled_epoch}"
}

begin_unit() {
    local label="$1"
    local now=0
    local remaining=0
    now="$(date +%s)" || die "Horloge invitée illisible avant l'unité ${label}."
    [[ "${now}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique avant l'unité ${label}."
    ((now < WORK_DEADLINE_EPOCH)) || \
        die "Deadline de travail atteinte; unité ${label} non lancée."
    remaining=$((WORK_DEADLINE_EPOCH - now))
    printf '[DEADLINE] unité=%s, secondes restantes avant GCE-30 min=%s.\n' \
        "${label}" "${remaining}"
}

report_failure_log() {
    local label="$1"
    local path="$2"

    if [[ ! -f "${path}" || -L "${path}" ]]; then
        printf '[DIAGNOSTIC %s] Journal absent, non régulier ou symbolique : %s.\n' \
            "${label}" "${path}" >&2
        return 0
    fi
    printf '[DIAGNOSTIC %s] %s dernières lignes et %s octets au plus; début.\n' \
        "${label}" "${FAILURE_LOG_MAX_LINES}" "${FAILURE_LOG_MAX_BYTES}" >&2
    tail -c "${FAILURE_LOG_MAX_BYTES}" -- "${path}" | \
        tail -n "${FAILURE_LOG_MAX_LINES}" >&2 || \
        printf '[DIAGNOSTIC %s] Lecture bornée du journal impossible.\n' "${label}" >&2
    printf '[DIAGNOSTIC %s] fin.\n' "${label}" >&2
}

collect_docker_host_diagnostics() {
    {
        printf '%s\n' '[DIAGNOSTIC HÔTE DOCKER] début.'
        if command -v systemctl >/dev/null 2>&1; then
            printf '%s\n' '[systemctl is-active docker]'
            timeout --foreground --kill-after=2s \
                "${DOCKER_DIAGNOSTIC_TIMEOUT_SECONDS}s" \
                systemctl is-active docker || true
            printf '%s\n' '[systemctl is-enabled docker]'
            timeout --foreground --kill-after=2s \
                "${DOCKER_DIAGNOSTIC_TIMEOUT_SECONDS}s" \
                systemctl is-enabled docker || true
        else
            printf '%s\n' 'systemctl absent.'
        fi
        if command -v dpkg-query >/dev/null 2>&1; then
            printf '%s\n' '[paquets Docker/containerd/NVIDIA]'
            timeout --foreground --kill-after=2s \
                "${DOCKER_DIAGNOSTIC_TIMEOUT_SECONDS}s" \
                dpkg-query -W \
                '-f=${binary:Package}\t${Status}\t${Version}\n' \
                docker.io docker-ce docker-ce-cli containerd containerd.io \
                nvidia-container-toolkit nvidia-container-runtime || true
        else
            printf '%s\n' 'dpkg-query absent.'
        fi
        if command -v journalctl >/dev/null 2>&1; then
            printf '%s\n' '[journal Docker du boot, 80 lignes au plus]'
            if command -v sudo >/dev/null 2>&1; then
                timeout --foreground --kill-after=2s \
                    "${DOCKER_DIAGNOSTIC_TIMEOUT_SECONDS}s" \
                    sudo -n -- journalctl --unit=docker.service --boot \
                    --no-pager --lines=80 || true
            else
                timeout --foreground --kill-after=2s \
                    "${DOCKER_DIAGNOSTIC_TIMEOUT_SECONDS}s" \
                    journalctl --unit=docker.service --boot \
                    --no-pager --lines=80 || true
            fi
        else
            printf '%s\n' 'journalctl absent.'
        fi
        printf '%s\n' '[DIAGNOSTIC HÔTE DOCKER] fin.'
    } >>"${DOCKER_LOG}" 2>&1
}

certify_sudo_docker_client() {
    local component=""
    local index=0
    local metadata=""
    local mode=""
    local owner_uid=""
    local path=""
    local -a expected_paths=(/ /usr /usr/bin "${SUDO_DOCKER_BIN}")

    command -v sudo >/dev/null 2>&1 || return 1
    timeout --foreground --kill-after=2s \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
        sudo -n -- /usr/bin/test -f "${SUDO_DOCKER_BIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1
    timeout --foreground --kill-after=2s \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
        sudo -n -- /usr/bin/test ! -L "${SUDO_DOCKER_BIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1
    timeout --foreground --kill-after=2s \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
        sudo -n -- /usr/bin/test -x "${SUDO_DOCKER_BIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1
    metadata="$(timeout --foreground --kill-after=2s \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
        sudo -n -- /usr/bin/stat -Lc '%n|%u|%a' -- \
        / /usr /usr/bin "${SUDO_DOCKER_BIN}" 2>>"${DOCKER_LOG}")" || return 1
    printf '%s\n' "${metadata}" >>"${DOCKER_LOG}"
    while IFS='|' read -r path owner_uid mode; do
        ((index < ${#expected_paths[@]})) || return 1
        [[ "${path}" == "${expected_paths[index]}" && "${owner_uid}" == "0" && \
            "${mode}" =~ ^[0-7]{3,4}$ ]] || return 1
        (((8#${mode} & 8#22) == 0)) || return 1
        index=$((index + 1))
    done <<<"${metadata}"
    ((index == ${#expected_paths[@]})) || return 1
    timeout --foreground --kill-after=2s \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
        sudo -n -- "${SUDO_DOCKER_BIN}" --version \
        >>"${DOCKER_LOG}" 2>&1
}

certify_buildx_plugin() {
    local index=0
    local metadata=""
    local mode=""
    local owner_uid=""
    local path=""
    local parent="${BUILDX_PLUGIN}"
    local -a identity=()
    local -a reverse_paths=()
    local -a expected_paths=()

    case "${DOCKER_IDENTITY}" in
        direct)
            ;;
        sudo)
            identity=(sudo -n --)
            ;;
        *)
            return 1
            ;;
    esac

    while true; do
        reverse_paths+=("${parent}")
        [[ "${parent}" == "/" ]] && break
        parent="${parent%/*}"
        [[ -n "${parent}" ]] || parent="/"
    done
    for ((index = ${#reverse_paths[@]} - 1; index >= 0; index--)); do
        expected_paths+=("${reverse_paths[index]}")
    done

    for ((index = 0; index < ${#expected_paths[@]} - 1; index++)); do
        path="${expected_paths[index]}"
        timeout --foreground --kill-after=2s \
            "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
            "${identity[@]}" "${BUILDX_TEST_BIN}" -d "${path}" \
            >>"${DOCKER_LOG}" 2>&1 || return 1
        timeout --foreground --kill-after=2s \
            "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
            "${identity[@]}" "${BUILDX_TEST_BIN}" ! -L "${path}" \
            >>"${DOCKER_LOG}" 2>&1 || return 1
    done
    timeout --foreground --kill-after=2s \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
        "${identity[@]}" "${BUILDX_TEST_BIN}" -f "${BUILDX_PLUGIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1
    timeout --foreground --kill-after=2s \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
        "${identity[@]}" "${BUILDX_TEST_BIN}" ! -L "${BUILDX_PLUGIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1
    timeout --foreground --kill-after=2s \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
        "${identity[@]}" "${BUILDX_TEST_BIN}" -x "${BUILDX_PLUGIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1

    metadata="$(timeout --foreground --kill-after=2s \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
        "${identity[@]}" "${BUILDX_STAT_BIN}" -c '%n|%u|%a' -- \
        "${expected_paths[@]}" 2>>"${DOCKER_LOG}")" || return 1
    printf '%s\n' "${metadata}" >>"${DOCKER_LOG}"
    index=0
    while IFS='|' read -r path owner_uid mode; do
        ((index < ${#expected_paths[@]})) || return 1
        [[ "${path}" == "${expected_paths[index]}" && "${owner_uid}" == "0" && \
            "${mode}" =~ ^[0-7]{3,4}$ ]] || return 1
        (((8#${mode} & 8#22) == 0)) || return 1
        index=$((index + 1))
    done <<<"${metadata}"
    ((index == ${#expected_paths[@]})) || return 1

    if [[ "${DOCKER_IDENTITY}" == "sudo" ]]; then
        BUILDX=(sudo -n --preserve-env=BUILDX_CONFIG -- "${BUILDX_PLUGIN}")
    else
        BUILDX=("${BUILDX_PLUGIN}")
    fi
    timeout --foreground --kill-after=2s \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
        "${BUILDX[@]}" version >>"${DOCKER_LOG}" 2>&1
}

attempt_sudo_docker_certification() {
    if ((sudo_docker_certification_attempted == 1)); then
        ((sudo_docker_client == 1))
        return
    fi
    sudo_docker_certification_attempted=1
    if command -v sudo >/dev/null 2>&1; then
        printf '[CLI SUDO] chemin système fixe à certifier : %s.\n' \
            "${SUDO_DOCKER_BIN}" >>"${DOCKER_LOG}"
        if certify_sudo_docker_client; then
            sudo_docker_client=1
            return 0
        fi
        printf '%s\n' '[CLI SUDO] chemin absent, non sûr ou client inexécutable.' \
            >>"${DOCKER_LOG}"
    else
        printf '%s\n' '[CLI SUDO] sudo absent.' >>"${DOCKER_LOG}"
    fi
    return 1
}

read_guest_shutdown_guard() {
    local candidate="${SESSION_DIR}/guest-shutdown-candidate.log"

    if command -v sudo >/dev/null 2>&1 && \
        sudo -n cat /run/systemd/shutdown/scheduled >"${candidate}" 2>&1 && \
        guard_is_scheduled "${candidate}"; then
        mv -- "${candidate}" "${GUARD_LOG}"
        return 0
    fi
    if [[ -r /run/systemd/shutdown/scheduled ]] && \
        cat /run/systemd/shutdown/scheduled >"${candidate}" 2>&1 && \
        guard_is_scheduled "${candidate}"; then
        mv -- "${candidate}" "${GUARD_LOG}"
        return 0
    fi
    rm -f -- "${candidate}"
    return 1
}

read_guest_shutdown_guard || \
    die "Arrêt invité planifié absent ou illisible; aucun travail GPU ou Docker n'a été lancé."
printf '[GARDE] Arrêt invité=%s, échéance GCE sûre=%s, deadline de travail=%s.\n' \
    "${GUEST_SHUTDOWN_EPOCH}" "${GCE_DEADLINE_EPOCH}" "${WORK_DEADLINE_EPOCH}"

begin_unit "preflight-blackwell"
if ! "${PREFLIGHT_SCRIPT}" --skip-docker >"${PREFLIGHT_LOG}" 2>&1; then
    report_failure_log "preflight-blackwell" "${PREFLIGHT_LOG}"
    die "Le preflight Blackwell non destructif a échoué; voir ${PREFLIGHT_LOG}."
fi

declare -a DOCKER=()
declare -a BUILDX=()
direct_docker_client=0
docker_deadline_reached=0
sudo_docker_client=0
sudo_docker_certification_attempted=0
docker_path="$(command -v docker 2>/dev/null || true)"
if [[ -n "${docker_path}" && "${docker_path}" == /* && \
    "${docker_path}" != *$'\n'* && "${docker_path}" != *$'\r'* && \
    -f "${docker_path}" && -x "${docker_path}" ]]; then
    printf '[CLI DIRECTE] %s\n' "${docker_path}" >>"${DOCKER_LOG}"
    if timeout --foreground --kill-after=2s \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
        "${docker_path}" --version >>"${DOCKER_LOG}" 2>&1; then
        direct_docker_client=1
    else
        printf '%s\n' '[CLI DIRECTE] docker --version a échoué.' >>"${DOCKER_LOG}"
    fi
else
    printf '%s\n' '[CLI DIRECTE] docker absent du PATH ou non exécutable absolu régulier.' \
        >>"${DOCKER_LOG}"
    docker_path=""
fi
if ((direct_docker_client == 0)); then
    attempt_sudo_docker_certification || true
fi
if ((direct_docker_client == 0 && sudo_docker_client == 0)); then
    collect_docker_host_diagnostics
    report_failure_log "docker-info" "${DOCKER_LOG}"
    die "Client Docker absent ou inexécutable dans le PATH utilisateur et indisponible via sudo non interactif."
fi
begin_unit "docker-access"
for ((attempt = 1; attempt <= DOCKER_INFO_MAX_ATTEMPTS; attempt++)); do
    probe_now="$(date +%s)" || die "Horloge invitée illisible pendant les sondes Docker."
    [[ "${probe_now}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique pendant les sondes Docker."
    if ((probe_now >= WORK_DEADLINE_EPOCH)); then
        docker_deadline_reached=1
        printf '[SONDE DOCKER] deadline atteinte avant tentative=%s/%s.\n' \
            "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}" >>"${DOCKER_LOG}"
        break
    fi
    if ((direct_docker_client == 1)); then
        printf '[SONDE DOCKER] tentative=%s/%s, voie=directe.\n' \
            "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}" >>"${DOCKER_LOG}"
        if timeout --foreground --kill-after=2s \
            "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
            "${docker_path}" info >>"${DOCKER_LOG}" 2>&1; then
            DOCKER=("${docker_path}")
            DOCKER_IDENTITY="direct"
            printf '[DOCKER] Daemon accessible directement à la tentative %s/%s.\n' \
                "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}"
            break
        fi
    fi
    probe_now="$(date +%s)" || die "Horloge invitée illisible entre les voies Docker."
    [[ "${probe_now}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique entre les voies Docker."
    if ((probe_now >= WORK_DEADLINE_EPOCH)); then
        docker_deadline_reached=1
        printf '[SONDE DOCKER] deadline atteinte avant voie sudo, tentative=%s/%s.\n' \
            "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}" >>"${DOCKER_LOG}"
        break
    fi
    if ((sudo_docker_certification_attempted == 0)); then
        attempt_sudo_docker_certification || true
        probe_now="$(date +%s)" || die "Horloge invitée illisible après certification Docker sudo."
        [[ "${probe_now}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique après certification Docker sudo."
        if ((probe_now >= WORK_DEADLINE_EPOCH)); then
            docker_deadline_reached=1
            printf '[SONDE DOCKER] deadline atteinte après certification sudo, tentative=%s/%s.\n' \
                "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}" >>"${DOCKER_LOG}"
            break
        fi
    fi
    if ((sudo_docker_client == 1)); then
        printf '[SONDE DOCKER] tentative=%s/%s, voie=sudo-n.\n' \
            "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}" >>"${DOCKER_LOG}"
        if timeout --foreground --kill-after=2s \
            "${DOCKER_PROBE_TIMEOUT_SECONDS}s" \
            sudo -n -- "${SUDO_DOCKER_BIN}" info >>"${DOCKER_LOG}" 2>&1; then
            DOCKER=(sudo -n -- "${SUDO_DOCKER_BIN}")
            DOCKER_IDENTITY="sudo"
            printf '[DOCKER] Voie directe indisponible; sudo non interactif certifié à la tentative %s/%s.\n' \
                "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}"
            break
        fi
    fi
    if ((attempt < DOCKER_INFO_MAX_ATTEMPTS)); then
        probe_now="$(date +%s)" || die "Horloge invitée illisible avant l'attente Docker."
        [[ "${probe_now}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique avant l'attente Docker."
        if ((probe_now + DOCKER_INFO_RETRY_SECONDS >= WORK_DEADLINE_EPOCH)); then
            docker_deadline_reached=1
            printf '[SONDE DOCKER] attente refusée par la deadline après tentative=%s/%s.\n' \
                "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}" >>"${DOCKER_LOG}"
            break
        fi
        sleep "${DOCKER_INFO_RETRY_SECONDS}"
    fi
done
if ((${#DOCKER[@]} == 0)); then
    collect_docker_host_diagnostics
    report_failure_log "docker-info" "${DOCKER_LOG}"
    if ((docker_deadline_reached == 1)); then
        die "Deadline de travail atteinte pendant les sondes Docker; aucune sonde suivante ni construction n'a été lancée."
    fi
    die "Client Docker présent mais daemon inaccessible directement et via sudo non interactif après les sondes bornées."
fi

begin_unit "buildx-certification"
printf '[BUILDX] chemin système fixe à certifier : %s; identité=%s.\n' \
    "${BUILDX_PLUGIN}" "${DOCKER_IDENTITY}" >>"${DOCKER_LOG}"
if ! certify_buildx_plugin; then
    collect_docker_host_diagnostics
    report_failure_log "docker-info" "${DOCKER_LOG}"
    die "Plugin Buildx fixe absent, inexécutable ou non sûr : ${BUILDX_PLUGIN}."
fi

IMAGE_REF="morsehgp3d-phase3:${HEAD_SHA}"
begin_unit "buildx-build"
if ! "${BUILDX[@]}" build --load \
    --file "${DOCKERFILE}" \
    --tag "${IMAGE_REF}" \
    --iidfile "${IID_FILE}" \
    "${DOCKER_CONTEXT}" >"${BUILD_LOG}" 2>&1; then
    report_failure_log "docker-build" "${BUILD_LOG}"
    die "La construction de l'image CUDA Phase 3 a échoué; voir ${BUILD_LOG}."
fi
[[ -s "${IID_FILE}" ]] || die "Docker n'a pas écrit d'identifiant d'image."
IMAGE_ID="$(tr -d '\r\n' <"${IID_FILE}")"
[[ "${IMAGE_ID}" =~ ^sha256:[0-9a-f]{64}$ ]] || die "Identifiant d'image Docker non canonique."
inspected_image_id="$("${DOCKER[@]}" image inspect --format '{{.Id}}' "${IMAGE_REF}")" || \
    die "Impossible de relire l'identifiant de l'image construite."
[[ "${inspected_image_id}" == "${IMAGE_ID}" ]] || \
    die "L'image taguée ne correspond pas à l'iidfile de construction."
printf 'base_image_ref=%s\nimage_ref=%s\nimage_id=%s\n' \
    "${BASE_IMAGE_REF}" "${IMAGE_REF}" "${IMAGE_ID}" >>"${BUILD_LOG}"

run_container() {
    local log_path="$1"
    shift
    "${DOCKER[@]}" run --rm --gpus all \
        "${DOCKER_IDENTITY_ARGS[@]}" \
        --volume "${REPOSITORY_ROOT}:${CONTAINER_REPOSITORY}:ro" \
        --volume "${BUILD_DIR}:${CONTAINER_BUILD}:rw" \
        --volume "${RESULT_DIR}:${CONTAINER_RESULTS}:rw" \
        --workdir "${CONTAINER_SOURCE}" \
        --env "HOME=${CONTAINER_HOME}" \
        --env "MORSEHGP3D_CUDA_IMAGE_REF=${IMAGE_REF}" \
        --env "MORSEHGP3D_CUDA_IMAGE_ID=${IMAGE_ID}" \
        --env "MORSEHGP3D_GIT_SHA=${HEAD_SHA}" \
        "${IMAGE_REF}" "$@" >"${log_path}" 2>&1
}

run_container_split_output() {
    local stdout_path="$1"
    local stderr_path="$2"
    shift 2
    "${DOCKER[@]}" run --rm --gpus all \
        "${DOCKER_IDENTITY_ARGS[@]}" \
        --volume "${REPOSITORY_ROOT}:${CONTAINER_REPOSITORY}:ro" \
        --volume "${BUILD_DIR}:${CONTAINER_BUILD}:rw" \
        --volume "${RESULT_DIR}:${CONTAINER_RESULTS}:rw" \
        --workdir "${CONTAINER_SOURCE}" \
        --env "HOME=${CONTAINER_HOME}" \
        --env "MORSEHGP3D_CUDA_IMAGE_REF=${IMAGE_REF}" \
        --env "MORSEHGP3D_CUDA_IMAGE_ID=${IMAGE_ID}" \
        --env "MORSEHGP3D_GIT_SHA=${HEAD_SHA}" \
        "${IMAGE_REF}" "$@" >"${stdout_path}" 2>"${stderr_path}"
}

begin_unit "cuda-release"
if ! run_container "${RELEASE_LOG}" cmake --workflow --preset cuda-release; then
    report_failure_log "cuda-release" "${RELEASE_LOG}"
    die "Le workflow cuda-release a échoué; voir ${RELEASE_LOG}."
fi
begin_unit "cuda-audit"
if ! run_container "${AUDIT_LOG}" cmake --workflow --preset cuda-audit; then
    report_failure_log "cuda-audit" "${AUDIT_LOG}"
    die "Le workflow cuda-audit a échoué; voir ${AUDIT_LOG}."
fi
begin_unit "runtime"
if ! run_container "${RUNTIME_LOG}" "${RUNTIME_PATH}" \
    --allocation-bytes 67108864 \
    --exercise-structured-error \
    --output "${CONTAINER_RESULTS}/runtime.jsonl"; then
    report_failure_log "runtime" "${RUNTIME_LOG}"
    die "Le runtime Phase 3 a échoué; voir ${RUNTIME_LOG}."
fi
begin_unit "binding-dlpack"
if ! run_container "${BINDING_LOG}" python3 \
    tests/cuda/check_phase3_binding.py "${MODULE_DIR}"; then
    report_failure_log "binding-dlpack" "${BINDING_LOG}"
    die "Le contrôle de liaison Python/DLPack a échoué; voir ${BINDING_LOG}."
fi
begin_unit "cuobjdump-elf"
if ! run_container "${ELF_LOG}" cuobjdump -lelf "${RUNTIME_PATH}"; then
    report_failure_log "cuobjdump-elf" "${ELF_LOG}"
    die "cuobjdump n'a pas pu lister les objets ELF AOT; voir ${ELF_LOG}."
fi
architectures="$(grep -Eo 'sm_[0-9]+' "${ELF_LOG}" | sort -u || true)"
if [[ "${architectures}" != "sm_120" ]]; then
    report_failure_log "cuobjdump-elf" "${ELF_LOG}"
    die "Le binaire AOT doit contenir au moins un ELF et uniquement sm_120; observé : ${architectures:-aucun}."
fi
begin_unit "cuobjdump-ptx"
if ! run_container_split_output "${PTX_LOG}" "${PTX_STDERR_LOG}" \
    cuobjdump -lptx "${RUNTIME_PATH}"; then
    report_failure_log "cuobjdump-ptx-stderr" "${PTX_STDERR_LOG}"
    die "cuobjdump n'a pas pu auditer les entrées PTX; voir ${PTX_STDERR_LOG}."
fi
if grep -q '[^[:space:]]' "${PTX_LOG}"; then
    report_failure_log "cuobjdump-ptx" "${PTX_LOG}"
    die "Une entrée PTX a été détectée; le runtime mesuré doit être AOT sm_120 uniquement."
fi
begin_unit "compute-sanitizer"
if ! run_container "${SANITIZER_LOG}" compute-sanitizer \
    --tool memcheck \
    --leak-check full \
    --error-exitcode=86 \
    "${RUNTIME_PATH}" \
    --allocation-bytes 4194304 \
    --exercise-structured-error \
    --output "${CONTAINER_RESULTS}/sanitizer-runtime.jsonl"; then
    report_failure_log "compute-sanitizer" "${SANITIZER_LOG}"
    die "compute-sanitizer a détecté une erreur ou a échoué; voir ${SANITIZER_LOG}."
fi

[[ -s "${RUNTIME_JSONL}" ]] || die "Le runtime n'a pas produit son JSONL."
[[ -s "${SANITIZER_JSONL}" ]] || die "Le passage memcheck n'a pas produit son JSONL."

python3 -B "${ASSEMBLER}" \
    --git-sha "${HEAD_SHA}" \
    --base-image-ref "${BASE_IMAGE_REF}" \
    --image-ref "${IMAGE_REF}" \
    --image-id "${IMAGE_ID}" \
    --runtime-jsonl "${RUNTIME_JSONL}" \
    --sanitizer-jsonl "${SANITIZER_JSONL}" \
    --guest-guard-log "${GUARD_LOG}" \
    --preflight-log "${PREFLIGHT_LOG}" \
    --build-log "${BUILD_LOG}" \
    --release-log "${RELEASE_LOG}" \
    --audit-log "${AUDIT_LOG}" \
    --binding-log "${BINDING_LOG}" \
    --elf-log "${ELF_LOG}" \
    --ptx-log "${PTX_LOG}" \
    --ptx-stderr-log "${PTX_STDERR_LOG}" \
    --sanitizer-log "${SANITIZER_LOG}" \
    --output "${PUBLISH_TEMP}"

python3 - "${PUBLISH_TEMP}" "${OUTPUT_PATH}" <<'PY'
import json
import os
from pathlib import Path
import sys

temporary = Path(sys.argv[1])
target = Path(sys.argv[2])
with temporary.open(encoding="utf-8") as stream:
    value = json.load(stream)
if not isinstance(value, dict) or value.get("status") != "worker_passed_pending_shutdown":
    raise SystemExit("the Phase 3 worker artifact is not pending targeted shutdown")
try:
    os.link(temporary, target, follow_symlinks=False)
except FileExistsError:
    raise SystemExit(f"refusing to replace an existing artifact: {target}")
os.unlink(temporary)
directory_fd = os.open(target.parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
try:
    os.fsync(directory_fd)
finally:
    os.close(directory_fd)
PY
PUBLISH_TEMP=""

printf '[SUCCÈS WORKER] Artefact provisoire publié atomiquement : %s\n' "${OUTPUT_PATH}"
printf '[CYCLE DE VIE] Le worker invité ne ferme pas la VM; l’orchestrateur externe doit certifier TERMINATED.\n'
