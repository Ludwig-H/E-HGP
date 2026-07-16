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

ASSUME_YES=0
OUTPUT_RAW=""
OUTPUT_PATH=""
OUTPUT_PARENT=""
OUTPUT_BASE=""
REPOSITORY_ROOT=""
SESSION_DIR=""
PUBLISH_TEMP=""
SESSION_CREATED=0

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/phase3_remote_qualification.sh --yes --output /CHEMIN/ABSOLU.json

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
case "${OUTPUT_RAW}" in
    /*) ;;
    *) die "--output doit être un chemin absolu." ;;
esac

command -v git >/dev/null 2>&1 || die "git est introuvable."
command -v id >/dev/null 2>&1 || die "id est introuvable."
command -v python3 >/dev/null 2>&1 || die "python3 est introuvable."
command -v mktemp >/dev/null 2>&1 || die "mktemp est introuvable."
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
mkdir -p -- "${SESSION_DIR}/build" "${SESSION_DIR}/logs" "${SESSION_DIR}/results"
PUBLISH_TEMP="$(mktemp "${OUTPUT_PARENT}/.${OUTPUT_BASE}.XXXXXXXX.partial")" || \
    die "Impossible de réserver l'artefact temporaire atomique."
chmod 600 "${PUBLISH_TEMP}"

readonly BUILD_DIR="${SESSION_DIR}/build"
readonly LOG_DIR="${SESSION_DIR}/logs"
readonly RESULT_DIR="${SESSION_DIR}/results"
readonly CONTAINER_HOME="${CONTAINER_BUILD}/.phase3-home"
readonly GUARD_LOG="${LOG_DIR}/guest-shutdown-guard.log"
readonly PREFLIGHT_LOG="${LOG_DIR}/preflight.log"
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
printf '[GARDE] Arrêt invité planifié relu avant tout travail lourd.\n'

if ! "${PREFLIGHT_SCRIPT}" --skip-docker >"${PREFLIGHT_LOG}" 2>&1; then
    die "Le preflight Blackwell non destructif a échoué; voir ${PREFLIGHT_LOG}."
fi

declare -a DOCKER=(docker)
if command -v docker >/dev/null 2>&1 && docker info >/dev/null 2>&1; then
    DOCKER=(docker)
elif command -v sudo >/dev/null 2>&1 && sudo -n docker info >/dev/null 2>&1; then
    DOCKER=(sudo -n docker)
else
    die "Docker est absent ou inaccessible sans interaction."
fi

IMAGE_REF="morsehgp3d-phase3:${HEAD_SHA}"
if ! "${DOCKER[@]}" build \
    --file "${DOCKERFILE}" \
    --tag "${IMAGE_REF}" \
    --iidfile "${IID_FILE}" \
    "${DOCKER_CONTEXT}" >"${BUILD_LOG}" 2>&1; then
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

if ! run_container "${RELEASE_LOG}" cmake --workflow --preset cuda-release; then
    die "Le workflow cuda-release a échoué; voir ${RELEASE_LOG}."
fi
if ! run_container "${AUDIT_LOG}" cmake --workflow --preset cuda-audit; then
    die "Le workflow cuda-audit a échoué; voir ${AUDIT_LOG}."
fi
if ! run_container "${RUNTIME_LOG}" "${RUNTIME_PATH}" \
    --allocation-bytes 67108864 \
    --exercise-structured-error \
    --output "${CONTAINER_RESULTS}/runtime.jsonl"; then
    die "Le runtime Phase 3 a échoué; voir ${RUNTIME_LOG}."
fi
if ! run_container "${BINDING_LOG}" python3 \
    tests/cuda/check_phase3_binding.py "${MODULE_DIR}"; then
    die "Le contrôle de liaison Python/DLPack a échoué; voir ${BINDING_LOG}."
fi
if ! run_container "${ELF_LOG}" cuobjdump -lelf "${RUNTIME_PATH}"; then
    die "cuobjdump n'a pas pu lister les objets ELF AOT; voir ${ELF_LOG}."
fi
architectures="$(grep -Eo 'sm_[0-9]+' "${ELF_LOG}" | sort -u || true)"
[[ "${architectures}" == "sm_120" ]] || \
    die "Le binaire AOT doit contenir au moins un ELF et uniquement sm_120; observé : ${architectures:-aucun}."
if ! run_container_split_output "${PTX_LOG}" "${PTX_STDERR_LOG}" \
    cuobjdump -lptx "${RUNTIME_PATH}"; then
    die "cuobjdump n'a pas pu auditer les entrées PTX; voir ${PTX_STDERR_LOG}."
fi
if grep -q '[^[:space:]]' "${PTX_LOG}"; then
    die "Une entrée PTX a été détectée; le runtime mesuré doit être AOT sm_120 uniquement."
fi
if ! run_container "${SANITIZER_LOG}" compute-sanitizer \
    --tool memcheck \
    --leak-check full \
    --error-exitcode=86 \
    "${RUNTIME_PATH}" \
    --allocation-bytes 4194304 \
    --exercise-structured-error \
    --output "${CONTAINER_RESULTS}/sanitizer-runtime.jsonl"; then
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
