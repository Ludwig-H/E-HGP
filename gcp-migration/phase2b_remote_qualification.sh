#!/usr/bin/env bash
set -Eeuo pipefail

# Non-mutating guest worker. VM lifecycle remains exclusively owned by the
# guarded start_and_verify.sh / stop_and_verify.sh orchestrator.
readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase2b_qualification.py"
readonly CONTAINER_REPOSITORY="/workspace/repository"
readonly CONTAINER_BUILD="${CONTAINER_REPOSITORY}/build"
readonly CONTAINER_SOURCE="${CONTAINER_REPOSITORY}/morsehgp3d"
readonly CONTAINER_RESULTS="/results"
readonly DOCKER_BIN="/usr/bin/docker"
readonly TIMEOUT_BIN="/usr/bin/timeout"
readonly DATE_BIN="/usr/bin/date"
readonly WORK_DEADLINE_RESERVE_SECONDS=1800
readonly POST_TIMEOUT_RESERVE_SECONDS=60
readonly WORK_UNIT_KILL_AFTER_SECONDS=5
readonly CLEANUP_TIMEOUT_SECONDS=10
readonly CLEANUP_KILL_AFTER_SECONDS=2
readonly CLEANUP_INITIAL_ABSENCE_READS=5
readonly CLEANUP_POST_REMOVE_ABSENCE_READS=2
readonly MAX_SESSION_SECONDS=28800
readonly CONTAINER_SESSION_LABEL="morsehgp3d.phase2b.session"

ASSUME_YES=0
EXPECTED_SHA=""
IMAGE_ID=""
GCE_DEADLINE_RAW=""
GCE_DEADLINE_EPOCH=0
GUEST_SHUTDOWN_EPOCH=0
WORK_DEADLINE_EPOCH=0
OUTPUT_RAW=""
OUTPUT_PATH=""
REPOSITORY_ROOT=""
SESSION_DIR=""
BUILD_DIR=""
RESULT_DIR=""
SESSION_TOKEN=""
ACTIVE_CONTAINER_NAME=""
ACTIVE_CONTAINER_CIDFILE=""
UNIT_COUNTER=0
declare -a DOCKER=()
declare -a DOCKER_IDENTITY_ARGS=()

die() {
    printf '[ERREUR PHASE 2B] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/phase2b_remote_qualification.sh --yes \
  --expected-sha SHA --image-id sha256:... --gce-deadline-epoch EPOCH \
  --output /CHEMIN/ABSOLU.json

Worker invité borné de qualification des prédicats Phase 2B. Il exige un
checkout propre au SHA annoncé et un arrêt invité déjà programmé. Il ne crée,
ne démarre, n'arrête et ne supprime aucune ressource GCP. L'artefact publié
reste provisoire jusqu'à la certification externe de l'arrêt ciblé.
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
        --image-id)
            (($# >= 2)) || die "Valeur manquante après --image-id."
            [[ -z "${IMAGE_ID}" ]] || die "--image-id ne peut être fourni qu'une fois."
            IMAGE_ID="$2"
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
[[ "${IMAGE_ID}" =~ ^sha256:[0-9a-f]{64}$ ]] || die "--image-id doit être un identifiant sha256 canonique."
[[ "${GCE_DEADLINE_RAW}" =~ ^[0-9]{10}$ ]] || die "--gce-deadline-epoch doit être un epoch UTC sur dix chiffres."
[[ -n "${OUTPUT_RAW}" ]] || die "--output est obligatoire."
case "${OUTPUT_RAW}" in
    /*) ;;
    *) die "--output doit être un chemin absolu." ;;
esac
[[ -x "${TIMEOUT_BIN}" && -x "${DATE_BIN}" ]] || die "Les exécutables bornés timeout/date sont absents."
[[ -x "${DOCKER_BIN}" ]] || die "${DOCKER_BIN} est absent ou non exécutable."

REPOSITORY_ROOT="$(git -C "${SCRIPT_DIR}/.." rev-parse --show-toplevel 2>/dev/null)" || \
    die "Impossible d'identifier le clone Git."
REPOSITORY_ROOT="$(cd -- "${REPOSITORY_ROOT}" && pwd -P)" || die "Racine Git illisible."
[[ "${SCRIPT_DIR}" == "${REPOSITORY_ROOT}/gcp-migration" ]] || \
    die "Le worker doit être exécuté depuis le clone canonique qui le contient."
readonly ASSEMBLER="${REPOSITORY_ROOT}/${ASSEMBLER_RELATIVE}"
[[ -f "${ASSEMBLER}" && ! -L "${ASSEMBLER}" ]] || die "Assembleur Phase 2B absent ou symbolique."

observed_sha="$(git -C "${REPOSITORY_ROOT}" rev-parse HEAD)" || die "HEAD Git illisible."
[[ "${observed_sha}" == "${EXPECTED_SHA}" ]] || \
    die "Le checkout ${observed_sha} diffère du SHA attendu ${EXPECTED_SHA}."
[[ "$(git -C "${REPOSITORY_ROOT}" rev-parse --verify "${EXPECTED_SHA}^{commit}")" == "${EXPECTED_SHA}" ]] || \
    die "Le SHA attendu n'est pas un commit local canonique."
[[ -z "$(git -C "${REPOSITORY_ROOT}" status --porcelain --untracked-files=normal)" ]] || \
    die "Le checkout qualifié doit être entièrement propre."

output_parent="$(dirname -- "${OUTPUT_RAW}")"
output_base="$(basename -- "${OUTPUT_RAW}")"
[[ -n "${output_base}" && "${output_base}" != "." && "${output_base}" != ".." ]] || \
    die "Nom d'artefact invalide."
[[ -d "${output_parent}" ]] || die "Le parent de --output doit déjà exister."
output_parent="$(cd -- "${output_parent}" && pwd -P)" || die "Parent de sortie illisible."
OUTPUT_PATH="${output_parent}/${output_base}"
case "${OUTPUT_PATH}/" in
    "${REPOSITORY_ROOT}/"*) die "--output doit rester hors du worktree." ;;
esac
[[ ! -e "${OUTPUT_PATH}" && ! -L "${OUTPUT_PATH}" ]] || die "La sortie doit être inexistante."

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
((GCE_DEADLINE_EPOCH > now_epoch)) || die "L'échéance GCE est déjà atteinte."
((GCE_DEADLINE_EPOCH - now_epoch <= MAX_SESSION_SECONDS)) || \
    die "L'échéance GCE dépasse la borne de huit heures."
GUEST_SHUTDOWN_EPOCH="$(read_guest_shutdown_epoch)" || \
    die "L'arrêt invité poweroff programmé est absent ou illisible."
[[ "${GUEST_SHUTDOWN_EPOCH}" =~ ^[0-9]+$ ]] || die "Échéance invitée non numérique."
((GUEST_SHUTDOWN_EPOCH > now_epoch)) || die "L'arrêt invité est déjà échu."
((GUEST_SHUTDOWN_EPOCH <= GCE_DEADLINE_EPOCH)) || \
    die "L'arrêt invité doit précéder ou égaler l'échéance GCE."
WORK_DEADLINE_EPOCH=$((GUEST_SHUTDOWN_EPOCH - WORK_DEADLINE_RESERVE_SECONDS))
((WORK_DEADLINE_EPOCH - now_epoch >= POST_TIMEOUT_RESERVE_SECONDS)) || \
    die "La réserve avant arrêt invité est insuffisante."
printf '[GARDE PHASE 2B] arrêt invité=%s, échéance GCE=%s, travail avant=%s.\n' \
    "${GUEST_SHUTDOWN_EPOCH}" "${GCE_DEADLINE_EPOCH}" "${WORK_DEADLINE_EPOCH}"

run_bounded() {
    local label="$1"
    local current=0
    local remaining=0
    local soft_timeout=0
    shift
    current="$(${DATE_BIN} +%s)" || return 125
    [[ "${current}" =~ ^[0-9]+$ ]] || return 125
    remaining=$((WORK_DEADLINE_EPOCH - current))
    soft_timeout=$((remaining - WORK_UNIT_KILL_AFTER_SECONDS - POST_TIMEOUT_RESERVE_SECONDS))
    if ((soft_timeout <= 0)); then
        printf '[DEADLINE PHASE 2B] unité %s refusée, réserve insuffisante.\n' "${label}" >&2
        return 124
    fi
    printf '[UNITÉ PHASE 2B] %s, timeout=%ss.\n' "${label}" "${soft_timeout}" >&2
    "${TIMEOUT_BIN}" --kill-after="${WORK_UNIT_KILL_AFTER_SECONDS}s" \
        "${soft_timeout}s" "$@"
}

docker_cleanup_call() {
    "${TIMEOUT_BIN}" --kill-after="${CLEANUP_KILL_AFTER_SECONDS}s" \
        "${CLEANUP_TIMEOUT_SECONDS}s" "${DOCKER[@]}" "$@"
}

container_ids_by_name() {
    local name="$1"
    docker_cleanup_call ps -aq --no-trunc --filter "name=^/${name}$"
}

cleanup_active_container() {
    local cid=""
    local candidate=""
    local evidence=""
    local observed_id=""
    local observed_name=""
    local observed_image=""
    local observed_label=""
    local extra=""
    local absence=0
    local required_absences="${CLEANUP_INITIAL_ABSENCE_READS}"

    [[ -n "${ACTIVE_CONTAINER_NAME}" ]] || return 0
    if [[ -f "${ACTIVE_CONTAINER_CIDFILE}" && ! -L "${ACTIVE_CONTAINER_CIDFILE}" ]]; then
        cid="$(tr -d '\r\n' <"${ACTIVE_CONTAINER_CIDFILE}")" || cid=""
        [[ -z "${cid}" || "${cid}" =~ ^[0-9a-f]{64}$ ]] || return 1
    fi
    candidate="$(container_ids_by_name "${ACTIVE_CONTAINER_NAME}")" || return 1
    if [[ -n "${candidate}" ]]; then
        [[ "${candidate}" =~ ^[0-9a-f]{64}$ ]] || return 1
        if [[ -n "${cid}" && "${candidate}" != "${cid}" ]]; then
            return 1
        fi
        cid="${candidate}"
    fi
    if [[ -n "${cid}" ]]; then
        evidence="$(docker_cleanup_call inspect \
            --format '{{.Id}}|{{.Name}}|{{.Config.Image}}|{{index .Config.Labels "morsehgp3d.phase2b.session"}}' \
            "${cid}")" || {
                [[ -z "$(container_ids_by_name "${ACTIVE_CONTAINER_NAME}")" ]] || return 1
                evidence=""
            }
        if [[ -n "${evidence}" ]]; then
            IFS='|' read -r observed_id observed_name observed_image observed_label extra <<<"${evidence}"
            [[ "${observed_id}" == "${cid}" && \
                "${observed_name}" == "/${ACTIVE_CONTAINER_NAME}" && \
                "${observed_image}" == "${IMAGE_ID}" && \
                "${observed_label}" == "${SESSION_TOKEN}" && -z "${extra}" ]] || return 1
            docker_cleanup_call rm -f "${cid}" >/dev/null || return 1
        fi
        required_absences="${CLEANUP_POST_REMOVE_ABSENCE_READS}"
    fi
    for ((absence = 0; absence < required_absences; absence++)); do
        [[ -z "$(container_ids_by_name "${ACTIVE_CONTAINER_NAME}")" ]] || return 1
        ((absence + 1 == required_absences)) || sleep 1
    done
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
    if [[ -n "${SESSION_DIR}" && -d "${SESSION_DIR}" ]]; then
        case "${SESSION_DIR}" in
            "${TMPDIR:-/tmp}"/morsehgp3d-phase2b-worker.*)
                rm -rf -- "${SESSION_DIR}" || cleanup_status=125
                ;;
            *)
                printf '[ERREUR PHASE 2B] nettoyage refusé pour %s.\n' "${SESSION_DIR}" >&2
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

inspected_image_id="$(run_bounded image-inspect "${DOCKER[@]}" image inspect \
    --format '{{.Id}}' "${IMAGE_ID}")" || die "L'image CUDA qualifiée est absente."
[[ "${inspected_image_id}" == "${IMAGE_ID}" ]] || die "L'identité de l'image CUDA a changé."

SESSION_DIR="$(mktemp -d "${TMPDIR:-/tmp}/morsehgp3d-phase2b-worker.XXXXXXXX")" || \
    die "Impossible de créer le temporaire de session."
chmod 700 "${SESSION_DIR}"
BUILD_DIR="${SESSION_DIR}/build"
RESULT_DIR="${SESSION_DIR}/results"
mkdir -p -- "${BUILD_DIR}/home" "${RESULT_DIR}"
SESSION_TOKEN="$(printf '%s' "${SESSION_DIR}:${EXPECTED_SHA}" | sha256sum | cut -c1-16)"
[[ "${SESSION_TOKEN}" =~ ^[0-9a-f]{16}$ ]] || die "Jeton de session invalide."

uid="$(id -u)"
gid="$(id -g)"
DOCKER_IDENTITY_ARGS=(
    --user "${uid}:${gid}"
    --volume "${REPOSITORY_ROOT}:${CONTAINER_REPOSITORY}:ro"
    --volume "${BUILD_DIR}:${CONTAINER_BUILD}:rw"
    --volume "${RESULT_DIR}:${CONTAINER_RESULTS}:rw"
    --workdir "${CONTAINER_SOURCE}"
    --env "HOME=${CONTAINER_BUILD}/home"
)
while read -r group_id; do
    [[ "${group_id}" =~ ^[0-9]+$ ]] || die "Identifiant de groupe Docker invalide."
    DOCKER_IDENTITY_ARGS+=(--group-add "${group_id}")
done < <(id -G | tr ' ' '\n')

run_container() {
    local label="$1"
    local stdout_path="$2"
    local stderr_path="$3"
    local entrypoint="$4"
    local name=""
    local cidfile=""
    local collision=""
    local run_status=0
    local cleanup_status=0
    shift 4

    UNIT_COUNTER=$((UNIT_COUNTER + 1))
    name="morsehgp3d-phase2b-${SESSION_TOKEN}-${UNIT_COUNTER}"
    cidfile="${SESSION_DIR}/${name}.cid"
    collision="$(container_ids_by_name "${name}")" || return 125
    [[ -z "${collision}" ]] || {
        printf '[ERREUR PHASE 2B] collision de nom Docker : %s.\n' "${name}" >&2
        return 125
    }
    ACTIVE_CONTAINER_NAME="${name}"
    ACTIVE_CONTAINER_CIDFILE="${cidfile}"
    if run_bounded "${label}" "${DOCKER[@]}" run \
        --name "${name}" \
        --label "${CONTAINER_SESSION_LABEL}=${SESSION_TOKEN}" \
        --cidfile "${cidfile}" --gpus all --interactive \
        --entrypoint "${entrypoint}" \
        "${DOCKER_IDENTITY_ARGS[@]}" \
        "${IMAGE_ID}" "$@" >"${stdout_path}" 2>"${stderr_path}"; then
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

readonly RELEASE_LOG="${RESULT_DIR}/cuda-release.log"
readonly AUDIT_LOG="${RESULT_DIR}/cuda-audit.log"
readonly CPU_BUILD_LOG="${RESULT_DIR}/cpu-replay-build.log"
readonly DISTANCE_SUMMARY="${RESULT_DIR}/distance-summary.json"
readonly ORIENTATION_SUMMARY="${RESULT_DIR}/orientation-summary.json"
readonly POWER_SUMMARY="${RESULT_DIR}/power-summary.json"
readonly BENCHMARK_SUMMARY="${RESULT_DIR}/benchmark-summary.json"
readonly ELF_LOG="${RESULT_DIR}/runner.elf.txt"
readonly PTX_LOG="${RESULT_DIR}/runner.ptx.txt"
readonly RUNNER="${CONTAINER_BUILD}/morsehgp3d-cuda-release/morsehgp3d_gpu_predicate_replay"
readonly CPU_REPLAY="${CONTAINER_BUILD}/morsehgp3d-cuda-release/morsehgp3d_replay_predicate"

run_container cuda-release /dev/null "${RELEASE_LOG}" /usr/bin/cmake \
    --workflow --preset cuda-release || die "Le workflow CUDA release a échoué."
run_container cuda-audit /dev/null "${AUDIT_LOG}" /usr/bin/cmake \
    --workflow --preset cuda-audit || die "Le workflow CUDA audit a échoué."
run_container cpu-replay-build /dev/null "${CPU_BUILD_LOG}" /usr/bin/cmake \
    --build --preset cuda-release --target morsehgp3d_replay_predicate --parallel 8 || \
    die "La construction du replay CPU a échoué."

run_container distance-differential "${DISTANCE_SUMMARY}" \
    "${RESULT_DIR}/distance-differential.stderr.log" /usr/bin/python3 \
    tests/cuda/check_phase2b_distance_filter.py "${RUNNER}" "${CPU_REPLAY}" || \
    die "Le différentiel distance a échoué."
run_container orientation-differential "${ORIENTATION_SUMMARY}" \
    "${RESULT_DIR}/orientation-differential.stderr.log" /usr/bin/python3 \
    tests/cuda/check_phase2b_orientation_filter.py "${RUNNER}" "${CPU_REPLAY}" || \
    die "Le différentiel orientation a échoué."
run_container power-differential "${POWER_SUMMARY}" \
    "${RESULT_DIR}/power-differential.stderr.log" /usr/bin/python3 \
    tests/cuda/check_phase2b_power_bisector_filter.py "${RUNNER}" "${CPU_REPLAY}" || \
    die "Le différentiel bisecteur de puissance a échoué."
run_container benchmark "${BENCHMARK_SUMMARY}" \
    "${RESULT_DIR}/benchmark.stderr.log" /usr/bin/python3 \
    tests/cuda/benchmark_phase2b_predicates.py "${RUNNER}" \
    --cases 262144 --repeats 3 --predicate all --timeout-seconds 240 || \
    die "Le benchmark froid Phase 2B a échoué."

run_container cuobjdump-elf "${ELF_LOG}" "${RESULT_DIR}/runner.elf.stderr.log" \
    /usr/local/cuda/bin/cuobjdump -lelf "${RUNNER}" || \
    die "L'inspection ELF AOT a échoué."
run_container cuobjdump-ptx "${PTX_LOG}" "${RESULT_DIR}/runner.ptx.stderr.log" \
    /usr/local/cuda/bin/cuobjdump -lptx "${RUNNER}" || \
    die "L'inspection PTX a échoué."

printf '%s\n' \
    '1 compare_squared_distances 0000000000000000 0000000000000000 0000000000000000 3ff0000000000000 0000000000000000 0000000000000000 4000000000000000 0000000000000000 0000000000000000' \
    '2 compare_squared_distances 0000000000000000 0000000000000000 0000000000000000 bff0000000000000 0000000000000000 0000000000000000 3ff0000000000000 0000000000000000 0000000000000000' \
    >"${RESULT_DIR}/sanitizer-distance-input.txt"
printf '%s\n' \
    '3 orientation_3d 0000000000000000 0000000000000000 0000000000000000 3ff0000000000000 0000000000000000 0000000000000000 0000000000000000 3ff0000000000000 0000000000000000 0000000000000000 0000000000000000 3ff0000000000000' \
    '4 orientation_3d 0000000000000000 0000000000000000 0000000000000000 3ff0000000000000 0000000000000000 0000000000000000 0000000000000000 3ff0000000000000 0000000000000000 3ff0000000000000 3ff0000000000000 0000000000000000' \
    >"${RESULT_DIR}/sanitizer-orientation-input.txt"
printf '%s\n' \
    '5 power_bisector_side 0 0 0 1 2 3ff0000000000000 0000000000000000 0000000000000000 4000000000000000 0000000000000000 0000000000000000 1 0 1 1' \
    '6 power_bisector_side 0 0 0 1 2 bff0000000000000 0000000000000000 0000000000000000 3ff0000000000000 0000000000000000 0000000000000000 1 0 1 1' \
    >"${RESULT_DIR}/sanitizer-power-input.txt"

run_container sanitizer-distance "${RESULT_DIR}/sanitizer-distance-output.jsonl" \
    "${RESULT_DIR}/sanitizer-distance.stderr.log" /usr/local/cuda/bin/compute-sanitizer \
    --log-file "${CONTAINER_RESULTS}/sanitizer-distance.log" \
    --tool memcheck --leak-check full --error-exitcode=86 \
    "${RUNNER}" --audit-known <"${RESULT_DIR}/sanitizer-distance-input.txt" || \
    die "compute-sanitizer distance a échoué."
run_container sanitizer-orientation "${RESULT_DIR}/sanitizer-orientation-output.jsonl" \
    "${RESULT_DIR}/sanitizer-orientation.stderr.log" /usr/local/cuda/bin/compute-sanitizer \
    --log-file "${CONTAINER_RESULTS}/sanitizer-orientation.log" \
    --tool memcheck --leak-check full --error-exitcode=86 \
    "${RUNNER}" --audit-known <"${RESULT_DIR}/sanitizer-orientation-input.txt" || \
    die "compute-sanitizer orientation a échoué."
run_container sanitizer-power "${RESULT_DIR}/sanitizer-power-output.jsonl" \
    "${RESULT_DIR}/sanitizer-power.stderr.log" /usr/local/cuda/bin/compute-sanitizer \
    --log-file "${CONTAINER_RESULTS}/sanitizer-power.log" \
    --tool memcheck --leak-check full --error-exitcode=86 \
    "${RUNNER}" --audit-known <"${RESULT_DIR}/sanitizer-power-input.txt" || \
    die "compute-sanitizer bisecteur de puissance a échoué."

run_bounded assemble-artifact python3 -B "${ASSEMBLER}" \
    --git-sha "${EXPECTED_SHA}" \
    --image-id "${IMAGE_ID}" \
    --distance-summary "${DISTANCE_SUMMARY}" \
    --orientation-summary "${ORIENTATION_SUMMARY}" \
    --power-summary "${POWER_SUMMARY}" \
    --benchmark-summary "${BENCHMARK_SUMMARY}" \
    --elf-log "${ELF_LOG}" \
    --ptx-log "${PTX_LOG}" \
    --sanitizer-distance-log "${RESULT_DIR}/sanitizer-distance.log" \
    --sanitizer-orientation-log "${RESULT_DIR}/sanitizer-orientation.log" \
    --sanitizer-power-log "${RESULT_DIR}/sanitizer-power.log" \
    --output "${OUTPUT_PATH}" || die "L'assemblage de l'artefact Phase 2B a échoué."

printf '[SUCCÈS WORKER PHASE 2B] Artefact provisoire : %s\n' "${OUTPUT_PATH}"
printf '[CYCLE DE VIE] Aucune mutation GCP effectuée; l’orchestrateur doit certifier TERMINATED.\n'
