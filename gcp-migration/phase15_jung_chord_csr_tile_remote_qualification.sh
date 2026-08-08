#!/usr/bin/env bash
set -Eeuo pipefail

# Guest-only worker.  GCP lifecycle remains owned by the guarded orchestrator.
readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly PREFLIGHT_SCRIPT="${SCRIPT_DIR}/blackwell_preflight.sh"
readonly ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase15_jung_chord_csr_tile_qualification.py"
readonly STATUS_RELATIVE="docs/implementation_status.toml"
readonly STATUS_CHECKER_RELATIVE="tools/check_implementation_status.py"
readonly DOCKERFILE_RELATIVE="containers/cuda12.9-sm120.Dockerfile"
readonly BASE_IMAGE_REF="nvidia/cuda:12.9.2-devel-ubuntu24.04@sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
readonly CONTAINER_REPOSITORY="/workspace/repository"
readonly CONTAINER_SOURCE="${CONTAINER_REPOSITORY}/morsehgp3d"
readonly CONTAINER_BUILD="${CONTAINER_REPOSITORY}/build"
readonly CONTAINER_RESULTS="/results"
readonly CONTAINER_BINARY="${CONTAINER_BUILD}/morsehgp3d-cuda-release/morsehgp3d_gpu_jung_chord_csr_tile_qualification"
readonly TARGET_NAME="morsehgp3d_gpu_jung_chord_csr_tile_qualification"
readonly DEFAULT_CHORD_CAPACITY=67108864
readonly ORACLE_CHORD_CAPACITY=32768
readonly MAX_CHORD_CAPACITY=4294967296
readonly MAX_SESSION_SECONDS=28800
readonly WORK_RESERVE_SECONDS=120
readonly WORK_UNIT_KILL_AFTER_SECONDS=10
readonly ORACLE_TIMEOUT_SECONDS=120
readonly SCALE_50K_TIMEOUT_SECONDS=300
readonly CLEANUP_TIMEOUT_SECONDS=10
readonly CLEANUP_KILL_AFTER_SECONDS=2
readonly DOCKER_BIN="/usr/bin/docker"
readonly TIMEOUT_BIN="/usr/bin/timeout"
readonly DATE_BIN="/usr/bin/date"
readonly TAIL_BIN="/usr/bin/tail"
readonly CONTAINER_SESSION_LABEL="morsehgp3d.phase15.jung_chord_csr.session"

ASSUME_YES=0
EXPECTED_SHA=""
GCE_DEADLINE_RAW=""
GCE_DEADLINE_EPOCH=0
GUEST_SHUTDOWN_EPOCH=0
WORK_DEADLINE_EPOCH=0
OUTPUT_RAW=""
OUTPUT_PATH=""
OUTPUT_LOG_DIR=""
CHORD_CAPACITY="${MORSEHGP3D_PHASE15_JUNG_CHORD_CAPACITY:-${DEFAULT_CHORD_CAPACITY}}"
REPOSITORY_ROOT=""
REPOSITORY_BUILD_MOUNTPOINT=""
REPOSITORY_BUILD_MOUNTPOINT_CREATED=0
SESSION_DIR=""
BUILD_ROOT=""
SESSION_TOKEN=""
IMAGE_REF=""
IMAGE_ID=""
ACTIVE_CONTAINER_NAME=""
ACTIVE_CONTAINER_CIDFILE=""
UNIT_COUNTER=0
declare -a DOCKER=()
declare -a DOCKER_IDENTITY_ARGS=()

die() {
    printf '[ERREUR P15-HOCUDA-P0] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/phase15_jung_chord_csr_tile_remote_qualification.sh \
  --yes --expected-sha SHA --gce-deadline-epoch EPOCH \
  --output /CHEMIN/ABSOLU.json [--chord-capacity N]

Worker invité non mutatif pour la route G4 unique P15-HOCUDA-P0. Il exige un
checkout propre et les deux coupe-circuits déjà certifiés. Il configure puis
construit uniquement la cible Jung-chord, exécute exactement trois unités dans
cet ordre : n=32 all-pairs/oracle sous memcheck, n=50000 uniform_latin, puis
n=50000 eight_clusters, support 4 et rang fermé 11. Il ne tente aucun palier,
aucun tuning et aucun redimensionnement après capacity_exhausted.

La capacité 50k vaut 67 108 864 PointIds par défaut (environ 576 Mio pour les
PointIds u64 et leurs flags, hors CSR et espace de travail). L'environnement
MORSEHGP3D_PHASE15_JUNG_CHORD_CAPACITY ou --chord-capacity peut la remplacer;
la valeur effectivement utilisée est inscrite dans l'artefact.
EOF
}

while (($# > 0)); do
    case "$1" in
        --yes)
            ASSUME_YES=1
            shift
            ;;
        --expected-sha)
            (($# >= 2)) || die "Valeur manquante après --expected-sha."
            [[ -z "${EXPECTED_SHA}" ]] || die "--expected-sha ne peut être fourni qu'une fois."
            EXPECTED_SHA="$2"
            shift 2
            ;;
        --gce-deadline-epoch)
            (($# >= 2)) || die "Valeur manquante après --gce-deadline-epoch."
            [[ -z "${GCE_DEADLINE_RAW}" ]] || die "--gce-deadline-epoch ne peut être fourni qu'une fois."
            GCE_DEADLINE_RAW="$2"
            shift 2
            ;;
        --output)
            (($# >= 2)) || die "Valeur manquante après --output."
            [[ -z "${OUTPUT_RAW}" ]] || die "--output ne peut être fourni qu'une fois."
            OUTPUT_RAW="$2"
            shift 2
            ;;
        --chord-capacity)
            (($# >= 2)) || die "Valeur manquante après --chord-capacity."
            CHORD_CAPACITY="$2"
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

((ASSUME_YES == 1)) || die "--yes est obligatoire pour ce worker explicitement autorisé."
[[ "${EXPECTED_SHA}" =~ ^[0-9a-f]{40}$ ]] || die "--expected-sha doit être un SHA canonique."
[[ "${GCE_DEADLINE_RAW}" =~ ^[0-9]{10}$ ]] || die "--gce-deadline-epoch doit être un epoch UTC sur dix chiffres."
[[ "${CHORD_CAPACITY}" =~ ^[1-9][0-9]*$ ]] || die "La capacité de cordes doit être strictement positive."
((10#${CHORD_CAPACITY} <= MAX_CHORD_CAPACITY)) || \
    die "La capacité de cordes dépasse la borne opérationnelle ${MAX_CHORD_CAPACITY}; aucun lancement."
CHORD_CAPACITY="$((10#${CHORD_CAPACITY}))"
[[ -n "${OUTPUT_RAW}" ]] || die "--output est obligatoire."
case "${OUTPUT_RAW}" in
    /*.json) ;;
    *) die "--output doit être un chemin JSON absolu." ;;
esac
[[ -x "${TIMEOUT_BIN}" && -x "${DATE_BIN}" && -x "${TAIL_BIN}" ]] || \
    die "Les exécutables bornés timeout/date/tail sont absents."
[[ -x "${DOCKER_BIN}" ]] || die "${DOCKER_BIN} est absent ou non exécutable."
[[ -x "${PREFLIGHT_SCRIPT}" && ! -L "${PREFLIGHT_SCRIPT}" ]] || \
    die "Le preflight Blackwell est absent, symbolique ou non exécutable."

REPOSITORY_ROOT="$(git -C "${SCRIPT_DIR}/.." rev-parse --show-toplevel 2>/dev/null)" || \
    die "Impossible d'identifier le clone Git."
REPOSITORY_ROOT="$(cd -- "${REPOSITORY_ROOT}" && pwd -P)" || die "Racine Git illisible."
[[ "${SCRIPT_DIR}" == "${REPOSITORY_ROOT}/gcp-migration" ]] || \
    die "Le worker doit être exécuté depuis le clone canonique qui le contient."
readonly ASSEMBLER="${REPOSITORY_ROOT}/${ASSEMBLER_RELATIVE}"
readonly DOCKERFILE="${REPOSITORY_ROOT}/${DOCKERFILE_RELATIVE}"
readonly STATUS_FILE="${REPOSITORY_ROOT}/${STATUS_RELATIVE}"
readonly STATUS_CHECKER="${REPOSITORY_ROOT}/${STATUS_CHECKER_RELATIVE}"
[[ -f "${ASSEMBLER}" && ! -L "${ASSEMBLER}" ]] || \
    die "L'assembleur P15-HOCUDA-P0 est absent ou symbolique."
[[ -f "${DOCKERFILE}" && ! -L "${DOCKERFILE}" ]] || \
    die "Le Dockerfile CUDA épinglé est absent ou symbolique."
[[ -f "${STATUS_FILE}" && ! -L "${STATUS_FILE}" && \
    -f "${STATUS_CHECKER}" && ! -L "${STATUS_CHECKER}" ]] || \
    die "Le registre de phase ou son checker est absent ou symbolique."
grep -Fqx "FROM ${BASE_IMAGE_REF}" "${DOCKERFILE}" || \
    die "Le Dockerfile ne part pas de l'image CUDA épinglée attendue."

observed_sha="$(git -C "${REPOSITORY_ROOT}" rev-parse HEAD)" || die "HEAD Git illisible."
[[ "${observed_sha}" == "${EXPECTED_SHA}" ]] || \
    die "Le checkout ${observed_sha} diffère du SHA attendu ${EXPECTED_SHA}."
[[ -z "$(git -C "${REPOSITORY_ROOT}" status --porcelain --untracked-files=normal)" ]] || \
    die "Le checkout qualifié doit être entièrement propre."
python3 -B "${STATUS_CHECKER}" || die "Le registre des portes d'implémentation est invalide."
python3 -B - "${STATUS_FILE}" <<'PY' || \
    die "La porte d'entrée P15-HOCUDA-P0 n'est pas ouverte dans le registre."
from pathlib import Path
import sys
import tomllib
value = tomllib.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
phase = next((item for item in value.get("phases", []) if item.get("id") == "15"), None)
if not isinstance(phase, dict):
    raise SystemExit("phase 15 missing")
expected = {
    "phase15_shallow_higher_cuda_prototype_entry_gate_satisfied": True,
    "phase15_shallow_higher_cuda_prototype_exit_gate_satisfied": False,
    "phase15_shallow_higher_cuda_prototype_backend": "cuda_g4_plus_reference_cpu_oracle",
    "phase15_shallow_higher_cuda_prototype_profile": "hgp_reduced",
    "phase15_shallow_higher_cuda_prototype_mode": "proposal_only_shallow_higher_support_work_falsifier_v1",
    "phase15_shallow_higher_cuda_prototype_deployment_status": "prototype_only",
    "phase15_shallow_higher_cuda_prototype_public_status": "not_claimed",
}
if any(phase.get(key) != expected_value for key, expected_value in expected.items()):
    raise SystemExit("P15-HOCUDA-P0 gate mismatch")
PY

output_parent="$(dirname -- "${OUTPUT_RAW}")"
output_base="$(basename -- "${OUTPUT_RAW}")"
[[ -d "${output_parent}" ]] || die "Le parent de --output doit déjà exister."
output_parent="$(cd -- "${output_parent}" && pwd -P)" || die "Parent de sortie illisible."
OUTPUT_PATH="${output_parent}/${output_base}"
OUTPUT_LOG_DIR="${output_parent}/${output_base%.json}.logs"
case "${OUTPUT_PATH}/" in
    "${REPOSITORY_ROOT}/"*) die "Les artefacts doivent rester hors du worktree." ;;
esac
[[ ! -e "${OUTPUT_PATH}" && ! -L "${OUTPUT_PATH}" ]] || die "La sortie doit être inexistante."
[[ ! -e "${OUTPUT_LOG_DIR}" && ! -L "${OUTPUT_LOG_DIR}" ]] || die "Le répertoire de logs doit être inexistant."
mkdir -- "${OUTPUT_LOG_DIR}" || die "Impossible de créer le répertoire de logs."
chmod 700 -- "${OUTPUT_LOG_DIR}"

read_guest_shutdown_epoch() {
    local evidence=""
    local scheduled_usec=""
    if [[ -r /run/systemd/shutdown/scheduled ]]; then
        evidence="$(< /run/systemd/shutdown/scheduled)" || return 1
    elif command -v sudo >/dev/null 2>&1; then
        evidence="$(sudo -n -- /usr/bin/cat /run/systemd/shutdown/scheduled)" || return 1
    else
        return 1
    fi
    grep -qx 'MODE=poweroff' <<<"${evidence}" || return 1
    scheduled_usec="$(sed -n 's/^USEC=\([0-9][0-9]*\)$/\1/p' <<<"${evidence}")"
    [[ "${scheduled_usec}" =~ ^[0-9]{10,18}$ ]] || return 1
    printf '%s\n' "$((10#${scheduled_usec} / 1000000))"
}

now_epoch="$(${DATE_BIN} +%s)" || die "Horloge invitée illisible."
[[ "${now_epoch}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique."
GCE_DEADLINE_EPOCH=$((10#${GCE_DEADLINE_RAW}))
((GCE_DEADLINE_EPOCH > now_epoch)) || die "L'échéance GCE sûre est déjà atteinte."
((GCE_DEADLINE_EPOCH - now_epoch <= MAX_SESSION_SECONDS)) || \
    die "L'échéance GCE dépasse huit heures."
GUEST_SHUTDOWN_EPOCH="$(read_guest_shutdown_epoch)" || \
    die "L'arrêt invité poweroff programmé est absent ou illisible."
((GUEST_SHUTDOWN_EPOCH > now_epoch)) || die "L'arrêt invité est déjà échu."
((GUEST_SHUTDOWN_EPOCH <= GCE_DEADLINE_EPOCH + 300)) || \
    die "L'arrêt invité n'est pas compatible avec l'échéance GCE certifiée."
if ((GUEST_SHUTDOWN_EPOCH < GCE_DEADLINE_EPOCH)); then
    WORK_DEADLINE_EPOCH=$((GUEST_SHUTDOWN_EPOCH - WORK_RESERVE_SECONDS))
else
    WORK_DEADLINE_EPOCH=$((GCE_DEADLINE_EPOCH - WORK_RESERVE_SECONDS))
fi
((WORK_DEADLINE_EPOCH > now_epoch)) || die "La réserve de travail est déjà épuisée."
printf '[GARDE P15-HOCUDA-P0] arrêt invité=%s, échéance sûre GCE=%s, travail avant=%s.\n' \
    "${GUEST_SHUTDOWN_EPOCH}" "${GCE_DEADLINE_EPOCH}" "${WORK_DEADLINE_EPOCH}"
printf '[PHASE] P15-HOCUDA-P0; backend=cuda_g4_plus_reference_cpu_oracle; profile=hgp_reduced; mode=proposal_only_shallow_higher_support_work_falsifier_v1.\n'

run_bounded() {
    local label="$1"
    local timeout_cap_seconds="$2"
    local current=0
    local remaining=0
    local soft_timeout=0
    shift 2
    [[ "${timeout_cap_seconds}" =~ ^[0-9]+$ ]] || return 125
    current="$(${DATE_BIN} +%s)" || return 125
    [[ "${current}" =~ ^[0-9]+$ ]] || return 125
    remaining=$((WORK_DEADLINE_EPOCH - current))
    soft_timeout=$((remaining - WORK_UNIT_KILL_AFTER_SECONDS))
    if ((timeout_cap_seconds > 0 && soft_timeout > timeout_cap_seconds)); then
        soft_timeout=${timeout_cap_seconds}
    fi
    ((soft_timeout > 0)) || {
        printf '[DEADLINE P15-HOCUDA-P0] unité %s refusée.\n' "${label}" >&2
        return 124
    }
    printf '[UNITÉ P15-HOCUDA-P0] %s, timeout=%ss.\n' "${label}" "${soft_timeout}" >&2
    "${TIMEOUT_BIN}" --kill-after="${WORK_UNIT_KILL_AFTER_SECONDS}s" \
        "${soft_timeout}s" "$@"
}

docker_cleanup_call() {
    "${TIMEOUT_BIN}" --kill-after="${CLEANUP_KILL_AFTER_SECONDS}s" \
        "${CLEANUP_TIMEOUT_SECONDS}s" "${DOCKER[@]}" "$@"
}

cleanup_active_container() {
    local cid=""
    local observed=""
    [[ -n "${ACTIVE_CONTAINER_NAME}" ]] || return 0
    if [[ -f "${ACTIVE_CONTAINER_CIDFILE}" && ! -L "${ACTIVE_CONTAINER_CIDFILE}" ]]; then
        cid="$(tr -d '\r\n' <"${ACTIVE_CONTAINER_CIDFILE}")" || return 1
        [[ "${cid}" =~ ^[0-9a-f]{64}$ ]] || return 1
        observed="$(docker_cleanup_call inspect --format \
            '{{.Id}}|{{.Name}}|{{index .Config.Labels "morsehgp3d.phase15.jung_chord_csr.session"}}' \
            "${cid}" 2>/dev/null || true)"
        if [[ -n "${observed}" ]]; then
            [[ "${observed}" == "${cid}|/${ACTIVE_CONTAINER_NAME}|${SESSION_TOKEN}" ]] || return 1
            docker_cleanup_call rm -f "${cid}" >/dev/null || return 1
        fi
    fi
    [[ -z "$(docker_cleanup_call ps -aq --no-trunc --filter "name=^/${ACTIVE_CONTAINER_NAME}$")" ]] || return 1
    rm -f -- "${ACTIVE_CONTAINER_CIDFILE}" || return 1
    ACTIVE_CONTAINER_NAME=""
    ACTIVE_CONTAINER_CIDFILE=""
}

cleanup() {
    local original_status=$?
    local cleanup_status=0
    trap - EXIT HUP INT TERM
    if [[ -n "${ACTIVE_CONTAINER_NAME}" ]]; then
        cleanup_active_container || cleanup_status=125
    fi
    if ((REPOSITORY_BUILD_MOUNTPOINT_CREATED == 1)); then
        if [[ -d "${REPOSITORY_BUILD_MOUNTPOINT}" && ! -L "${REPOSITORY_BUILD_MOUNTPOINT}" ]]; then
            rmdir -- "${REPOSITORY_BUILD_MOUNTPOINT}" || cleanup_status=125
        else
            cleanup_status=125
        fi
    fi
    if [[ -n "${SESSION_DIR}" && -d "${SESSION_DIR}" ]]; then
        case "${SESSION_DIR}" in
            "${TMPDIR:-/tmp}"/morsehgp3d-phase15-jung-worker.*)
                rm -rf -- "${SESSION_DIR}" || cleanup_status=125
                ;;
            *)
                printf '[ERREUR P15-HOCUDA-P0] Nettoyage refusé pour %s.\n' "${SESSION_DIR}" >&2
                cleanup_status=125
                ;;
        esac
    fi
    ((cleanup_status == 0)) || exit "${cleanup_status}"
    exit "${original_status}"
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

if "${TIMEOUT_BIN}" --kill-after=2s 10s "${DOCKER_BIN}" info >/dev/null 2>&1; then
    DOCKER=("${DOCKER_BIN}")
elif command -v sudo >/dev/null 2>&1 && \
    "${TIMEOUT_BIN}" --kill-after=2s 10s sudo -n -- "${DOCKER_BIN}" info >/dev/null 2>&1; then
    DOCKER=(sudo -n -- "${DOCKER_BIN}")
else
    die "Docker n'est accessible ni directement ni par sudo non interactif."
fi

if ! run_bounded preflight 0 "${PREFLIGHT_SCRIPT}" --skip-docker \
    >"${OUTPUT_LOG_DIR}/preflight.stdout.log" \
    2>"${OUTPUT_LOG_DIR}/preflight.stderr.log"; then
    "${TAIL_BIN}" -n 160 -- "${OUTPUT_LOG_DIR}/preflight.stderr.log" >&2 || true
    die "Le preflight Blackwell a échoué."
fi

IMAGE_REF="morsehgp3d-phase15-jung:${EXPECTED_SHA}"
if ! run_bounded image-build 0 "${DOCKER[@]}" build \
    --file "${DOCKERFILE}" --tag "${IMAGE_REF}" "${REPOSITORY_ROOT}" \
    >"${OUTPUT_LOG_DIR}/image-build.stdout.log" \
    2>"${OUTPUT_LOG_DIR}/image-build.stderr.log"; then
    "${TAIL_BIN}" -n 160 -- "${OUTPUT_LOG_DIR}/image-build.stderr.log" >&2 || true
    die "La construction de l'image CUDA épinglée a échoué."
fi
IMAGE_ID="$(run_bounded image-id 0 "${DOCKER[@]}" image inspect \
    --format '{{.Id}}' "${IMAGE_REF}")" || die "L'identité de l'image construite est illisible."
[[ "${IMAGE_ID}" =~ ^sha256:[0-9a-f]{64}$ ]] || die "Identifiant d'image non canonique."

REPOSITORY_BUILD_MOUNTPOINT="${REPOSITORY_ROOT}/build"
if [[ -e "${REPOSITORY_BUILD_MOUNTPOINT}" || -L "${REPOSITORY_BUILD_MOUNTPOINT}" ]]; then
    [[ -d "${REPOSITORY_BUILD_MOUNTPOINT}" && ! -L "${REPOSITORY_BUILD_MOUNTPOINT}" ]] || \
        die "Le point de montage build du clone n'est pas un répertoire ordinaire."
else
    mkdir -- "${REPOSITORY_BUILD_MOUNTPOINT}" || die "Impossible de créer le point de montage build."
    REPOSITORY_BUILD_MOUNTPOINT_CREATED=1
fi
SESSION_DIR="$(mktemp -d "${TMPDIR:-/tmp}/morsehgp3d-phase15-jung-worker.XXXXXXXX")" || \
    die "Impossible de créer le temporaire de session."
chmod 700 -- "${SESSION_DIR}"
BUILD_ROOT="${SESSION_DIR}/build"
mkdir -p -- "${BUILD_ROOT}/home"
SESSION_TOKEN="$(printf '%s' "${SESSION_DIR}:${EXPECTED_SHA}" | sha256sum | cut -c1-16)"
[[ "${SESSION_TOKEN}" =~ ^[0-9a-f]{16}$ ]] || die "Jeton de session invalide."

uid="$(id -u)"
gid="$(id -g)"
DOCKER_IDENTITY_ARGS=(
    --user "${uid}:${gid}"
    --volume "${REPOSITORY_ROOT}:${CONTAINER_REPOSITORY}:ro"
    --volume "${BUILD_ROOT}:${CONTAINER_BUILD}:rw"
    --volume "${OUTPUT_LOG_DIR}:${CONTAINER_RESULTS}:rw"
    --workdir "${CONTAINER_SOURCE}"
    --env "HOME=${CONTAINER_BUILD}/home"
    --env "MORSEHGP3D_CUDA_IMAGE_REF=${IMAGE_REF}"
    --env "MORSEHGP3D_CUDA_IMAGE_ID=${IMAGE_ID}"
    --env "MORSEHGP3D_GIT_SHA=${EXPECTED_SHA}"
)
while read -r group_id; do
    [[ "${group_id}" =~ ^[0-9]+$ ]] || die "Identifiant de groupe Docker invalide."
    DOCKER_IDENTITY_ARGS+=(--group-add "${group_id}")
done < <(id -G | tr ' ' '\n')

run_container() {
    local timeout_cap_seconds="$1"
    local label="$2"
    local stdout_path="$3"
    local stderr_path="$4"
    local entrypoint="$5"
    local name=""
    local cidfile=""
    local collision=""
    local run_status=0
    local cleanup_status=0
    shift 5
    UNIT_COUNTER=$((UNIT_COUNTER + 1))
    name="morsehgp3d-p15-jung-${SESSION_TOKEN}-${UNIT_COUNTER}"
    cidfile="${SESSION_DIR}/${name}.cid"
    collision="$(docker_cleanup_call ps -aq --no-trunc --filter "name=^/${name}$")" || return 125
    [[ -z "${collision}" ]] || return 125
    ACTIVE_CONTAINER_NAME="${name}"
    ACTIVE_CONTAINER_CIDFILE="${cidfile}"
    if run_bounded "${label}" "${timeout_cap_seconds}" "${DOCKER[@]}" run \
        --name "${name}" \
        --label "${CONTAINER_SESSION_LABEL}=${SESSION_TOKEN}" \
        --cidfile "${cidfile}" --gpus all \
        --entrypoint "${entrypoint}" \
        "${DOCKER_IDENTITY_ARGS[@]}" "${IMAGE_ID}" "$@" \
        >"${stdout_path}" 2>"${stderr_path}"; then
        run_status=0
    else
        run_status=$?
    fi
    if cleanup_active_container; then
        cleanup_status=0
    else
        cleanup_status=125
    fi
    ((cleanup_status == 0)) || return "${cleanup_status}"
    return "${run_status}"
}

run_container 0 configure \
    "${OUTPUT_LOG_DIR}/configure.stdout.log" \
    "${OUTPUT_LOG_DIR}/configure.stderr.log" \
    /usr/bin/cmake --preset cuda-release || {
        "${TAIL_BIN}" -n 160 -- "${OUTPUT_LOG_DIR}/configure.stderr.log" >&2 || true
        die "La configuration CUDA release ciblée a échoué."
    }
run_container 0 target-build \
    "${OUTPUT_LOG_DIR}/target-build.stdout.log" \
    "${OUTPUT_LOG_DIR}/target-build.stderr.log" \
    /usr/bin/cmake --build "${CONTAINER_BUILD}/morsehgp3d-cuda-release" \
    --target "${TARGET_NAME}" --parallel 8 || {
        "${TAIL_BIN}" -n 160 -- "${OUTPUT_LOG_DIR}/target-build.stderr.log" >&2 || true
        die "La construction de l'unique cible P15-HOCUDA-P0 a échoué."
    }

host_binary="${BUILD_ROOT}/morsehgp3d-cuda-release/${TARGET_NAME}"
[[ -f "${host_binary}" && ! -L "${host_binary}" && -x "${host_binary}" ]] || \
    die "Le binaire ciblé n'a pas été produit sûrement."

oracle_status=0
if run_container "${ORACLE_TIMEOUT_SECONDS}" oracle32-memcheck \
    "${OUTPUT_LOG_DIR}/oracle32.stdout.json" \
    "${OUTPUT_LOG_DIR}/oracle32.stderr.log" \
    /usr/local/cuda/bin/compute-sanitizer \
    --target-processes all --tool memcheck --leak-check full \
    --report-api-errors no --error-exitcode=86 \
    --log-file "${CONTAINER_RESULTS}/oracle32.memcheck.log" \
    "${CONTAINER_BINARY}" \
    --point-count 32 --family uniform_latin --morton-window 31 \
    --support-size 4 --maximum-closed-rank 11 \
    --chord-capacity "${ORACLE_CHORD_CAPACITY}" --oracle; then
    oracle_status=0
else
    oracle_status=$?
fi
((oracle_status == 0)) || die "L'unique oracle n=32 sous compute-sanitizer a échoué (code=${oracle_status})."

uniform_status=0
if run_container "${SCALE_50K_TIMEOUT_SECONDS}" uniform-50000 \
    "${OUTPUT_LOG_DIR}/uniform-50000.stdout.json" \
    "${OUTPUT_LOG_DIR}/uniform-50000.stderr.log" \
    "${CONTAINER_BINARY}" \
    --point-count 50000 --family uniform_latin --morton-window 8 \
    --support-size 4 --maximum-closed-rank 11 \
    --chord-capacity "${CHORD_CAPACITY}"; then
    uniform_status=0
else
    uniform_status=$?
fi
((uniform_status == 0 || uniform_status == 1)) || \
    die "La qualification directe uniform_latin 50k a échoué sans reçu de capacité (code=${uniform_status})."

clusters_status=0
if run_container "${SCALE_50K_TIMEOUT_SECONDS}" eight-clusters-50000 \
    "${OUTPUT_LOG_DIR}/eight-clusters-50000.stdout.json" \
    "${OUTPUT_LOG_DIR}/eight-clusters-50000.stderr.log" \
    "${CONTAINER_BINARY}" \
    --point-count 50000 --family eight_clusters --morton-window 8 \
    --support-size 4 --maximum-closed-rank 11 \
    --chord-capacity "${CHORD_CAPACITY}"; then
    clusters_status=0
else
    clusters_status=$?
fi
((clusters_status == 0 || clusters_status == 1)) || \
    die "La qualification directe eight_clusters 50k a échoué sans reçu de capacité (code=${clusters_status})."

run_bounded assemble 0 python3 -B "${ASSEMBLER}" \
    --git-sha "${EXPECTED_SHA}" \
    --image-id "${IMAGE_ID}" \
    --binary "${host_binary}" \
    --container-binary "${CONTAINER_BINARY}" \
    --chord-capacity "${CHORD_CAPACITY}" \
    --log-dir "${OUTPUT_LOG_DIR}" \
    --oracle-exit-status "${oracle_status}" \
    --uniform-exit-status "${uniform_status}" \
    --clusters-exit-status "${clusters_status}" \
    --output "${OUTPUT_PATH}" || die "L'assemblage strict de l'artefact P15-HOCUDA-P0 a échoué."

printf '[SUCCÈS WORKER P15-HOCUDA-P0] Artefact provisoire : %s\n' "${OUTPUT_PATH}"
printf '[ARTEFACTS BRUTS] JSON/stdout/stderr : %s\n' "${OUTPUT_LOG_DIR}"
printf '[CYCLE DE VIE] GCP non muté par le worker; arrêt ciblé dû par l’orchestrateur.\n'
