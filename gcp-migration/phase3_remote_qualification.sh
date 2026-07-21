#!/usr/bin/env bash
set -Eeuo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly PREFLIGHT_SCRIPT="${SCRIPT_DIR}/blackwell_preflight.sh"
readonly DOCKERFILE_RELATIVE="containers/cuda12.9-sm120.Dockerfile"
readonly ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase3_qualification.py"
readonly PHASE4_ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase4_spatial_qualification.py"
readonly PHASE4_DIFFERENTIAL_RELATIVE="morsehgp3d/tests/differential/check_spatial_gpu_reference.py"
readonly PHASE4_LBVH_DIFFERENTIAL_RELATIVE="morsehgp3d/tests/differential/check_spatial_gpu_lbvh.py"
readonly PHASE5_K1_BORUVKA_ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase5_k1_boruvka_qualification.py"
readonly PHASE5_K1_BORUVKA_WORK_PROFILE_ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase5_k1_boruvka_work_profile.py"
readonly PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase5_k1_boruvka_exact_search_work_profile.py"
readonly PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_CHECKER_RELATIVE="morsehgp3d/tests/configuration/check_phase5_k1_boruvka_exact_search_work_profile.py"
readonly PHASE7_H_POLYTOPE_ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase7_h_polytope_qualification.py"
readonly BASE_IMAGE_REF="nvidia/cuda:12.9.2-devel-ubuntu24.04@sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
readonly CONTAINER_REPOSITORY="/workspace/repository"
readonly CONTAINER_BUILD="${CONTAINER_REPOSITORY}/build"
readonly CONTAINER_SOURCE="${CONTAINER_REPOSITORY}/morsehgp3d"
readonly CONTAINER_RESULTS="/results"
readonly RUNTIME_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_phase3_runtime"
readonly RUNTIME_PATH="${CONTAINER_REPOSITORY}/${RUNTIME_RELATIVE}"
readonly PHASE4_REPLAY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_spatial_reference_replay"
readonly PHASE4_REPLAY_PATH="${CONTAINER_REPOSITORY}/${PHASE4_REPLAY_RELATIVE}"
readonly PHASE4_DIFFERENTIAL_PATH="${CONTAINER_REPOSITORY}/${PHASE4_DIFFERENTIAL_RELATIVE}"
readonly PHASE4_LBVH_REPLAY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_spatial_lbvh_replay"
readonly PHASE4_LBVH_REPLAY_PATH="${CONTAINER_REPOSITORY}/${PHASE4_LBVH_REPLAY_RELATIVE}"
readonly PHASE4_LBVH_DIFFERENTIAL_PATH="${CONTAINER_REPOSITORY}/${PHASE4_LBVH_DIFFERENTIAL_RELATIVE}"
readonly PHASE5_K1_BORUVKA_FULL_REPLAY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_k1_boruvka_full_replay"
readonly PHASE5_K1_BORUVKA_FULL_REPLAY_PATH="${CONTAINER_REPOSITORY}/${PHASE5_K1_BORUVKA_FULL_REPLAY_RELATIVE}"
readonly PHASE5_K1_BORUVKA_WORK_PROFILE_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_k1_boruvka_morton_work_profile"
readonly PHASE5_K1_BORUVKA_WORK_PROFILE_PATH="${CONTAINER_REPOSITORY}/${PHASE5_K1_BORUVKA_WORK_PROFILE_RELATIVE}"
readonly PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_k1_boruvka_exact_search_work_profile"
readonly PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_PATH="${CONTAINER_REPOSITORY}/${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_RELATIVE}"
readonly PHASE7_H_POLYTOPE_BINARY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_h_polytope_proposal_qualification"
readonly PHASE7_H_POLYTOPE_BINARY_PATH="${CONTAINER_REPOSITORY}/${PHASE7_H_POLYTOPE_BINARY_RELATIVE}"
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
readonly PROBE_KILL_AFTER_SECONDS=2
readonly WORK_UNIT_KILL_AFTER_SECONDS=5
readonly CONTAINER_CLEANUP_TIMEOUT_SECONDS=2
readonly CONTAINER_CLEANUP_KILL_AFTER_SECONDS=1
readonly CONTAINER_CLEANUP_MAX_ATTEMPTS=7
readonly CONTAINER_CLEANUP_INITIAL_ABSENCES=5
readonly CONTAINER_CLEANUP_POST_RM_ABSENCES=2
readonly CONTAINER_CLEANUP_RETRY_SECONDS=1
readonly WORK_UNIT_POST_TIMEOUT_RESERVE_SECONDS=60
readonly CONTAINER_SESSION_LABEL="morsehgp3d.phase3.session"
readonly DOCKER_BIN="/usr/bin/docker"
readonly DOCKER_TEST_BIN="/usr/bin/test"
readonly DOCKER_STAT_BIN="/usr/bin/stat"
readonly TIMEOUT_BIN="/usr/bin/timeout"
readonly TIMEOUT_TEST_BIN="/usr/bin/test"
readonly TIMEOUT_STAT_BIN="/usr/bin/stat"
readonly SLEEP_BIN="/usr/bin/sleep"
readonly SLEEP_TEST_BIN="/usr/bin/test"
readonly SLEEP_STAT_BIN="/usr/bin/stat"
readonly DATE_BIN="/usr/bin/date"
readonly DATE_TEST_BIN="/usr/bin/test"
readonly DATE_STAT_BIN="/usr/bin/stat"
readonly BUILDX_PLUGIN="/usr/libexec/docker/cli-plugins/docker-buildx"
readonly BUILDX_TEST_BIN="/usr/bin/test"
readonly BUILDX_STAT_BIN="/usr/bin/stat"

ASSUME_YES=0
OUTPUT_RAW=""
OUTPUT_PATH=""
OUTPUT_PARENT=""
OUTPUT_BASE=""
PHASE4_OUTPUT_RAW=""
PHASE4_OUTPUT_PATH=""
PHASE4_OUTPUT_PARENT=""
PHASE4_OUTPUT_BASE=""
PHASE5_OUTPUT_RAW=""
PHASE5_OUTPUT_PATH=""
PHASE5_OUTPUT_PARENT=""
PHASE5_OUTPUT_BASE=""
PHASE5_WORK_PROFILE_OUTPUT_RAW=""
PHASE5_WORK_PROFILE_OUTPUT_PATH=""
PHASE5_WORK_PROFILE_OUTPUT_PARENT=""
PHASE5_WORK_PROFILE_OUTPUT_BASE=""
PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW=""
PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH=""
PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PARENT=""
PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_BASE=""
PHASE7_H_POLYTOPE_OUTPUT_RAW=""
PHASE7_H_POLYTOPE_OUTPUT_PATH=""
PHASE7_H_POLYTOPE_OUTPUT_PARENT=""
PHASE7_H_POLYTOPE_OUTPUT_BASE=""
GCE_DEADLINE_RAW=""
GCE_DEADLINE_EPOCH=0
WORK_DEADLINE_EPOCH=0
GUEST_SHUTDOWN_EPOCH=0
REPOSITORY_ROOT=""
SESSION_DIR=""
PUBLISH_TEMP=""
PHASE4_PUBLISH_TEMP=""
PHASE5_PUBLISH_TEMP=""
PHASE5_WORK_PROFILE_PUBLISH_TEMP=""
PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP=""
PHASE7_H_POLYTOPE_PUBLISH_TEMP=""
SESSION_CREATED=0
DOCKER_IDENTITY=""
BUILDX_CONFIG=""
SESSION_TOKEN=""
ACTIVE_CONTAINER_CIDFILE=""
ACTIVE_CONTAINER_LOG=""
ACTIVE_CONTAINER_NAME=""
REPOSITORY_BUILD_MOUNTPOINT=""
REPOSITORY_BUILD_MOUNTPOINT_CREATED=0
declare -a DOCKER=()
declare -a BUILDX=()

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

certify_fixed_executable_chain() {
    local fixed_bin="$1"
    local test_bin="$2"
    local stat_bin="$3"
    local index=0
    local metadata=""
    local mode=""
    local owner_uid=""
    local parent="${fixed_bin}"
    local path=""
    local -a reverse_paths=()
    local -a expected_paths=()

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
        "${test_bin}" -d "${path}" || return 1
        "${test_bin}" ! -L "${path}" || return 1
    done
    "${test_bin}" -f "${fixed_bin}" || return 1
    "${test_bin}" ! -L "${fixed_bin}" || return 1
    "${test_bin}" -x "${fixed_bin}" || return 1
    metadata="$("${stat_bin}" -Lc '%n|%u|%a' -- \
        "${expected_paths[@]}")" || return 1
    index=0
    while IFS='|' read -r path owner_uid mode; do
        ((index < ${#expected_paths[@]})) || return 1
        [[ "${path}" == "${expected_paths[index]}" && "${owner_uid}" == "0" && \
            "${mode}" =~ ^[0-7]{3,4}$ ]] || return 1
        (((8#${mode} & 8#22) == 0)) || return 1
        index=$((index + 1))
    done <<<"${metadata}"
    ((index == ${#expected_paths[@]}))
}

certify_fixed_timeout() {
    local timeout_help=""
    local timeout_version=""

    certify_fixed_executable_chain \
        "${TIMEOUT_BIN}" "${TIMEOUT_TEST_BIN}" "${TIMEOUT_STAT_BIN}" || return 1
    timeout_version="$("${TIMEOUT_BIN}" --version 2>/dev/null || true)"
    [[ "${timeout_version}" == timeout\ \(GNU\ coreutils\)* ]] || return 1
    timeout_help="$("${TIMEOUT_BIN}" --help 2>/dev/null || true)"
    [[ "${timeout_help}" == *"--kill-after=DURATION"* && \
        "${timeout_help}" == *"--foreground"* ]]
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/phase3_remote_qualification.sh --yes --gce-deadline-epoch EPOCH --output /CHEMIN/ABSOLU.json [--phase4-spatial-output /CHEMIN/ABSOLU.json] [--phase5-k1-boruvka-output /CHEMIN/ABSOLU.json] [--phase5-k1-boruvka-work-profile-output /CHEMIN/ABSOLU.json] [--phase5-k1-boruvka-exact-search-work-profile-output /CHEMIN/ABSOLU.json] [--phase7-h-polytope-output /CHEMIN/ABSOLU.json]

Worker invité non interactif de qualification de l'environnement CUDA Phase 3.
Il exige un arrêt invité déjà planifié, ne pilote jamais le cycle de vie GCP et
publie l'objet Phase 3 sans remplacement hors du worktree après les contrôles.
L'option Phase 4 exécute en plus le replay spatial exhaustif de référence et
le replay LBVH résident, puis publie leur compagnon provisoire commun, sans
rollback d'un nom final. Aucun des deux artefacts ne certifie l'arrêt de la VM.
L'option Phase 5 exécute le replay réel de la boucle K1 Boruvka complète avec
émission chunkée par sources contiguës non scindées sous budget de candidats,
audite son ELF sm_120 et l'absence de PTX, puis le passe sous memcheck et
racecheck. Son compagnon provisoire partage la même responsabilité d'arrêt
externe.
L'option de profil Morton exécute le benchmark canonique de travail K1
Boruvka sur neuf couples taille/famille. Son artefact reste empirique,
benchmark-only et soumis au même arrêt externe ciblé.
L'option de profil de recherche exacte exécute séparément neuf mesures de la
chaîne external-1NN exacte amorcée par Morton, avec W=16. Chaque journal est
validé avant assemblage; le compagnon reste benchmark-only et ne porte aucune
revendication de scalabilité ou de statut scientifique public.
L'option H-polytope Phase 7 exécute la qualification analytique CUDA du
transcript de propositions, audite son ELF sm_120 et l'absence de PTX, puis
le passe sous memcheck et racecheck. Son compagnon reste provisoire,
benchmark-only et soumis à la fermeture externe ciblée de la VM.
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
        --phase4-spatial-output)
            (($# >= 2)) || die "Valeur manquante après --phase4-spatial-output."
            [[ -z "${PHASE4_OUTPUT_RAW}" ]] || \
                die "--phase4-spatial-output ne peut être fourni qu'une fois."
            PHASE4_OUTPUT_RAW="$2"
            shift 2
            ;;
        --phase5-k1-boruvka-output)
            (($# >= 2)) || die "Valeur manquante après --phase5-k1-boruvka-output."
            [[ -z "${PHASE5_OUTPUT_RAW}" ]] || \
                die "--phase5-k1-boruvka-output ne peut être fourni qu'une fois."
            PHASE5_OUTPUT_RAW="$2"
            shift 2
            ;;
        --phase5-k1-boruvka-work-profile-output)
            (($# >= 2)) || \
                die "Valeur manquante après --phase5-k1-boruvka-work-profile-output."
            [[ -z "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" ]] || \
                die "--phase5-k1-boruvka-work-profile-output ne peut être fourni qu'une fois."
            PHASE5_WORK_PROFILE_OUTPUT_RAW="$2"
            shift 2
            ;;
        --phase5-k1-boruvka-exact-search-work-profile-output)
            (($# >= 2)) || \
                die "Valeur manquante après --phase5-k1-boruvka-exact-search-work-profile-output."
            [[ -z "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" ]] || \
                die "--phase5-k1-boruvka-exact-search-work-profile-output ne peut être fourni qu'une fois."
            PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW="$2"
            shift 2
            ;;
        --phase7-h-polytope-output)
            (($# >= 2)) || \
                die "Valeur manquante après --phase7-h-polytope-output."
            [[ -z "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" ]] || \
                die "--phase7-h-polytope-output ne peut être fourni qu'une fois."
            PHASE7_H_POLYTOPE_OUTPUT_RAW="$2"
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
case "${OUTPUT_RAW}" in
    /*) ;;
    *) die "--output doit être un chemin absolu." ;;
esac
if [[ -n "${PHASE4_OUTPUT_RAW}" ]]; then
    case "${PHASE4_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase4-spatial-output doit être un chemin absolu." ;;
    esac
    [[ "${PHASE4_OUTPUT_RAW}" != "${OUTPUT_RAW}" ]] || \
        die "Les sorties Phase 3 et Phase 4 doivent être distinctes."
fi
if [[ -n "${PHASE5_OUTPUT_RAW}" ]]; then
    case "${PHASE5_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase5-k1-boruvka-output doit être un chemin absolu." ;;
    esac
    [[ "${PHASE5_OUTPUT_RAW}" != "${OUTPUT_RAW}" && \
        "${PHASE5_OUTPUT_RAW}" != "${PHASE4_OUTPUT_RAW}" ]] || \
        die "Les sorties Phase 3, Phase 4 et Phase 5 doivent être distinctes."
fi
if [[ -n "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" ]]; then
    case "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase5-k1-boruvka-work-profile-output doit être un chemin absolu." ;;
    esac
    [[ "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" != "${OUTPUT_RAW}" && \
        "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" != "${PHASE4_OUTPUT_RAW}" && \
        "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" != "${PHASE5_OUTPUT_RAW}" ]] || \
        die "Toutes les sorties Phase 3, Phase 4 et Phase 5 doivent être distinctes."
fi
if [[ -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" ]]; then
    case "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase5-k1-boruvka-exact-search-work-profile-output doit être un chemin absolu." ;;
    esac
    [[ "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" != "${OUTPUT_RAW}" && \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" != "${PHASE4_OUTPUT_RAW}" && \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" != "${PHASE5_OUTPUT_RAW}" && \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" != \
            "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" ]] || \
        die "Toutes les sorties Phase 3, Phase 4 et Phase 5 doivent être distinctes."
fi
if [[ -n "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" ]]; then
    case "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase7-h-polytope-output doit être un chemin absolu." ;;
    esac
    [[ "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" != "${OUTPUT_RAW}" && \
        "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" != "${PHASE4_OUTPUT_RAW}" && \
        "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" != "${PHASE5_OUTPUT_RAW}" && \
        "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" != \
            "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" && \
        "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" != \
            "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" ]] || \
        die "La sortie H-polytope Phase 7 doit être distincte de toutes les autres sorties."
fi
certify_fixed_timeout || \
    die "La chaîne fixe /usr/bin/timeout n'est pas sûre, GNU ou compatible avec les groupes/--kill-after."
certify_fixed_executable_chain \
    "${SLEEP_BIN}" "${SLEEP_TEST_BIN}" "${SLEEP_STAT_BIN}" || \
    die "La chaîne fixe /usr/bin/sleep n'est pas sûre."
certify_fixed_executable_chain \
    "${DATE_BIN}" "${DATE_TEST_BIN}" "${DATE_STAT_BIN}" || \
    die "La chaîne fixe /usr/bin/date n'est pas sûre."
now_epoch="$("${DATE_BIN}" +%s)" || die "Horloge invitée illisible."
[[ "${now_epoch}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique."
((WORK_DEADLINE_EPOCH > now_epoch)) || \
    die "La deadline de travail GCE-30 min est déjà atteinte; aucune unité ne sera lancée."

command -v git >/dev/null 2>&1 || die "git est introuvable."
command -v id >/dev/null 2>&1 || die "id est introuvable."
command -v python3 >/dev/null 2>&1 || die "python3 est introuvable."
command -v mktemp >/dev/null 2>&1 || die "mktemp est introuvable."
command -v tail >/dev/null 2>&1 || die "tail est introuvable."
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
if [[ -n "${PHASE4_OUTPUT_RAW}" ]]; then
    PHASE4_OUTPUT_PARENT="$(dirname -- "${PHASE4_OUTPUT_RAW}")"
    PHASE4_OUTPUT_BASE="$(basename -- "${PHASE4_OUTPUT_RAW}")"
    [[ -n "${PHASE4_OUTPUT_BASE}" && "${PHASE4_OUTPUT_BASE}" != "." && \
        "${PHASE4_OUTPUT_BASE}" != ".." ]] || die "Nom d'artefact Phase 4 invalide."
    [[ -d "${PHASE4_OUTPUT_PARENT}" ]] || \
        die "Le répertoire parent de --phase4-spatial-output doit déjà exister."
    PHASE4_OUTPUT_PARENT="$(cd -- "${PHASE4_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie Phase 4 illisible."
    PHASE4_OUTPUT_PATH="${PHASE4_OUTPUT_PARENT}/${PHASE4_OUTPUT_BASE}"
    [[ "${PHASE4_OUTPUT_PARENT}" == "${OUTPUT_PARENT}" ]] || \
        die "Les artefacts Phase 3 et Phase 4 doivent partager le même parent sûr."
    [[ "${PHASE4_OUTPUT_PATH}" != "${OUTPUT_PATH}" ]] || \
        die "Les artefacts Phase 3 et Phase 4 doivent utiliser deux noms distincts."
    case "${PHASE4_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*) \
            die "--phase4-spatial-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE4_OUTPUT_PATH}" && ! -L "${PHASE4_OUTPUT_PATH}" ]] || \
        die "La sortie Phase 4 doit être inexistante : ${PHASE4_OUTPUT_PATH}."
fi
if [[ -n "${PHASE5_OUTPUT_RAW}" ]]; then
    PHASE5_OUTPUT_PARENT="$(dirname -- "${PHASE5_OUTPUT_RAW}")"
    PHASE5_OUTPUT_BASE="$(basename -- "${PHASE5_OUTPUT_RAW}")"
    [[ -n "${PHASE5_OUTPUT_BASE}" && "${PHASE5_OUTPUT_BASE}" != "." && \
        "${PHASE5_OUTPUT_BASE}" != ".." ]] || die "Nom d'artefact Phase 5 invalide."
    [[ -d "${PHASE5_OUTPUT_PARENT}" ]] || \
        die "Le répertoire parent de --phase5-k1-boruvka-output doit déjà exister."
    PHASE5_OUTPUT_PARENT="$(cd -- "${PHASE5_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie Phase 5 illisible."
    PHASE5_OUTPUT_PATH="${PHASE5_OUTPUT_PARENT}/${PHASE5_OUTPUT_BASE}"
    [[ "${PHASE5_OUTPUT_PARENT}" == "${OUTPUT_PARENT}" ]] || \
        die "Les sorties Phase 3 et Phase 5 doivent partager le même répertoire physique."
    [[ "${PHASE5_OUTPUT_PATH}" != "${OUTPUT_PATH}" && \
        "${PHASE5_OUTPUT_PATH}" != "${PHASE4_OUTPUT_PATH}" ]] || \
        die "Les sorties Phase 3, Phase 4 et Phase 5 doivent être distinctes."
    case "${PHASE5_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*)
            die "--phase5-k1-boruvka-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE5_OUTPUT_PATH}" && ! -L "${PHASE5_OUTPUT_PATH}" ]] || \
        die "La sortie Phase 5 doit être inexistante : ${PHASE5_OUTPUT_PATH}."
fi
if [[ -n "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" ]]; then
    PHASE5_WORK_PROFILE_OUTPUT_PARENT="$(dirname -- \
        "${PHASE5_WORK_PROFILE_OUTPUT_RAW}")"
    PHASE5_WORK_PROFILE_OUTPUT_BASE="$(basename -- \
        "${PHASE5_WORK_PROFILE_OUTPUT_RAW}")"
    [[ -n "${PHASE5_WORK_PROFILE_OUTPUT_BASE}" && \
        "${PHASE5_WORK_PROFILE_OUTPUT_BASE}" != "." && \
        "${PHASE5_WORK_PROFILE_OUTPUT_BASE}" != ".." ]] || \
        die "Nom d'artefact du profil de travail Phase 5 invalide."
    [[ -d "${PHASE5_WORK_PROFILE_OUTPUT_PARENT}" ]] || \
        die "Le répertoire parent de --phase5-k1-boruvka-work-profile-output doit déjà exister."
    PHASE5_WORK_PROFILE_OUTPUT_PARENT="$(cd -- \
        "${PHASE5_WORK_PROFILE_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie du profil de travail Phase 5 illisible."
    PHASE5_WORK_PROFILE_OUTPUT_PATH="${PHASE5_WORK_PROFILE_OUTPUT_PARENT}/${PHASE5_WORK_PROFILE_OUTPUT_BASE}"
    [[ "${PHASE5_WORK_PROFILE_OUTPUT_PARENT}" == "${OUTPUT_PARENT}" ]] || \
        die "Toutes les sorties doivent partager le même répertoire physique sûr."
    [[ "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" != "${OUTPUT_PATH}" && \
        "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" != "${PHASE4_OUTPUT_PATH}" && \
        "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" != "${PHASE5_OUTPUT_PATH}" ]] || \
        die "Toutes les sorties Phase 3, Phase 4 et Phase 5 doivent être distinctes."
    case "${PHASE5_WORK_PROFILE_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*)
            die "--phase5-k1-boruvka-work-profile-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" && \
        ! -L "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" ]] || \
        die "La sortie du profil de travail Phase 5 doit être inexistante : ${PHASE5_WORK_PROFILE_OUTPUT_PATH}."
fi
if [[ -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" ]]; then
    PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PARENT="$(dirname -- \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}")"
    PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_BASE="$(basename -- \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}")"
    [[ -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_BASE}" && \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_BASE}" != "." && \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_BASE}" != ".." ]] || \
        die "Nom d'artefact du profil external-1NN exact Phase 5 invalide."
    [[ -d "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PARENT}" ]] || \
        die "Le répertoire parent de --phase5-k1-boruvka-exact-search-work-profile-output doit déjà exister."
    PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PARENT="$(cd -- \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie du profil external-1NN exact Phase 5 illisible."
    PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH="${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PARENT}/${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_BASE}"
    [[ "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PARENT}" == \
        "${OUTPUT_PARENT}" ]] || \
        die "Toutes les sorties doivent partager le même répertoire physique sûr."
    [[ "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" != \
            "${OUTPUT_PATH}" && \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" != \
            "${PHASE4_OUTPUT_PATH}" && \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" != \
            "${PHASE5_OUTPUT_PATH}" && \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" != \
            "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" ]] || \
        die "Toutes les sorties Phase 3, Phase 4 et Phase 5 doivent être distinctes."
    case "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*)
            die "--phase5-k1-boruvka-exact-search-work-profile-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" && \
        ! -L "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" ]] || \
        die "La sortie du profil external-1NN exact Phase 5 doit être inexistante : ${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}."
fi
if [[ -n "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" ]]; then
    PHASE7_H_POLYTOPE_OUTPUT_PARENT="$(dirname -- \
        "${PHASE7_H_POLYTOPE_OUTPUT_RAW}")"
    PHASE7_H_POLYTOPE_OUTPUT_BASE="$(basename -- \
        "${PHASE7_H_POLYTOPE_OUTPUT_RAW}")"
    [[ -n "${PHASE7_H_POLYTOPE_OUTPUT_BASE}" && \
        "${PHASE7_H_POLYTOPE_OUTPUT_BASE}" != "." && \
        "${PHASE7_H_POLYTOPE_OUTPUT_BASE}" != ".." ]] || \
        die "Nom d'artefact H-polytope Phase 7 invalide."
    [[ -d "${PHASE7_H_POLYTOPE_OUTPUT_PARENT}" ]] || \
        die "Le répertoire parent de --phase7-h-polytope-output doit déjà exister."
    PHASE7_H_POLYTOPE_OUTPUT_PARENT="$(cd -- \
        "${PHASE7_H_POLYTOPE_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie H-polytope Phase 7 illisible."
    PHASE7_H_POLYTOPE_OUTPUT_PATH="${PHASE7_H_POLYTOPE_OUTPUT_PARENT}/${PHASE7_H_POLYTOPE_OUTPUT_BASE}"
    [[ "${PHASE7_H_POLYTOPE_OUTPUT_PARENT}" == "${OUTPUT_PARENT}" ]] || \
        die "Toutes les sorties doivent partager le même répertoire physique sûr."
    [[ "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" != "${OUTPUT_PATH}" && \
        "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" != "${PHASE4_OUTPUT_PATH}" && \
        "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" != "${PHASE5_OUTPUT_PATH}" && \
        "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" != \
            "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" && \
        "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" != \
            "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" ]] || \
        die "La sortie H-polytope Phase 7 doit être distincte de toutes les autres sorties."
    case "${PHASE7_H_POLYTOPE_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*)
            die "--phase7-h-polytope-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" && \
        ! -L "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" ]] || \
        die "La sortie H-polytope Phase 7 doit être inexistante : ${PHASE7_H_POLYTOPE_OUTPUT_PATH}."
fi

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
readonly PHASE4_ASSEMBLER="${REPOSITORY_ROOT}/${PHASE4_ASSEMBLER_RELATIVE}"
readonly PHASE4_DIFFERENTIAL="${REPOSITORY_ROOT}/${PHASE4_DIFFERENTIAL_RELATIVE}"
readonly PHASE4_LBVH_DIFFERENTIAL="${REPOSITORY_ROOT}/${PHASE4_LBVH_DIFFERENTIAL_RELATIVE}"
readonly PHASE5_K1_BORUVKA_ASSEMBLER="${REPOSITORY_ROOT}/${PHASE5_K1_BORUVKA_ASSEMBLER_RELATIVE}"
readonly PHASE5_K1_BORUVKA_WORK_PROFILE_ASSEMBLER="${REPOSITORY_ROOT}/${PHASE5_K1_BORUVKA_WORK_PROFILE_ASSEMBLER_RELATIVE}"
readonly PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_ASSEMBLER="${REPOSITORY_ROOT}/${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_ASSEMBLER_RELATIVE}"
readonly PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_CHECKER="${REPOSITORY_ROOT}/${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_CHECKER_RELATIVE}"
readonly PHASE7_H_POLYTOPE_ASSEMBLER="${REPOSITORY_ROOT}/${PHASE7_H_POLYTOPE_ASSEMBLER_RELATIVE}"
[[ -f "${DOCKERFILE}" && ! -L "${DOCKERFILE}" ]] || \
    die "Dockerfile Phase 3 absent ou symbolique : ${DOCKERFILE}."
[[ "$(sed -n '1p' "${DOCKERFILE}")" == "FROM ${BASE_IMAGE_REF}" ]] || \
    die "Le Dockerfile Phase 3 doit partir de l'image CUDA épinglée ${BASE_IMAGE_REF}."
[[ -f "${ASSEMBLER}" && ! -L "${ASSEMBLER}" ]] || \
    die "Assembleur Phase 3 absent ou symbolique : ${ASSEMBLER}."
if [[ -n "${PHASE4_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE4_ASSEMBLER}" && ! -L "${PHASE4_ASSEMBLER}" ]] || \
        die "Assembleur Phase 4 absent ou symbolique : ${PHASE4_ASSEMBLER}."
    [[ -f "${PHASE4_DIFFERENTIAL}" && ! -L "${PHASE4_DIFFERENTIAL}" ]] || \
        die "Différentiel spatial Phase 4 absent ou symbolique : ${PHASE4_DIFFERENTIAL}."
    [[ -f "${PHASE4_LBVH_DIFFERENTIAL}" && ! -L "${PHASE4_LBVH_DIFFERENTIAL}" ]] || \
        die "Différentiel LBVH résident Phase 4 absent ou symbolique : ${PHASE4_LBVH_DIFFERENTIAL}."
fi
if [[ -n "${PHASE5_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE5_K1_BORUVKA_ASSEMBLER}" && \
        ! -L "${PHASE5_K1_BORUVKA_ASSEMBLER}" ]] || \
        die "Assembleur Phase 5 absent ou symbolique : ${PHASE5_K1_BORUVKA_ASSEMBLER}."
fi
if [[ -n "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE5_K1_BORUVKA_WORK_PROFILE_ASSEMBLER}" && \
        ! -L "${PHASE5_K1_BORUVKA_WORK_PROFILE_ASSEMBLER}" ]] || \
        die "Assembleur du profil de travail Phase 5 absent ou symbolique : ${PHASE5_K1_BORUVKA_WORK_PROFILE_ASSEMBLER}."
fi
if [[ -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_ASSEMBLER}" && \
        ! -L "${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_ASSEMBLER}" ]] || \
        die "Assembleur du profil external-1NN exact Phase 5 absent ou symbolique : ${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_ASSEMBLER}."
    [[ -f "${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_CHECKER}" && \
        ! -L "${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_CHECKER}" ]] || \
        die "Checker du profil external-1NN exact Phase 5 absent ou symbolique : ${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_CHECKER}."
fi
if [[ -n "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE7_H_POLYTOPE_ASSEMBLER}" && \
        ! -L "${PHASE7_H_POLYTOPE_ASSEMBLER}" ]] || \
        die "Assembleur H-polytope Phase 7 absent ou symbolique : ${PHASE7_H_POLYTOPE_ASSEMBLER}."
fi

remove_container_from_cidfile() {
    local cidfile="$1"
    local container_name="$2"
    local log_path="$3"
    local absence_count=0
    local attempt=0
    local container_id=""
    local cid_evidence_status=2
    local cid_presence=""
    local cleanup_target=""
    local collision=""
    local evidence=""
    local observed_id=""
    local observed_image=""
    local observed_label=""
    local observed_name=""
    local extra=""
    local required_absences="${CONTAINER_CLEANUP_INITIAL_ABSENCES}"

    [[ "${container_name}" =~ ^morsehgp3d-phase3-[a-z0-9]{8}-[a-z0-9][a-z0-9-]*$ ]] || {
        printf '[ERREUR NETTOYAGE] nom de conteneur non canonique : %s.\n' \
            "${container_name}" >&2
        return 1
    }
    if [[ -e "${cidfile}" || -L "${cidfile}" ]]; then
        if [[ ! -f "${cidfile}" || -L "${cidfile}" ]]; then
            printf '[ERREUR NETTOYAGE] cidfile non régulier ou symbolique : %s.\n' \
                "${cidfile}" >&2
        else
            container_id="$(tr -d '\r\n' <"${cidfile}")" || container_id=""
        fi
        if [[ "${container_id}" =~ ^[0-9a-f]{64}$ ]]; then
            cid_evidence_status=0
        else
            printf '[ERREUR NETTOYAGE] identifiant de conteneur non canonique dans %s.\n' \
                "${cidfile}" >&2
        fi
    fi
    if ((${#DOCKER[@]} == 0)); then
        printf '[ERREUR NETTOYAGE] identité Docker indisponible pour supprimer %s.\n' \
            "${container_name}" >&2
        return 1
    fi
    for ((attempt = 1; attempt <= CONTAINER_CLEANUP_MAX_ATTEMPTS; attempt++)); do
        cid_presence=""
        if ((cid_evidence_status == 0)) && [[ -n "${container_id}" ]]; then
            if ! cid_presence="$("${TIMEOUT_BIN}" \
                --kill-after="${CONTAINER_CLEANUP_KILL_AFTER_SECONDS}s" \
                "${CONTAINER_CLEANUP_TIMEOUT_SECONDS}s" \
                "${DOCKER[@]}" ps -a --no-trunc \
                --filter "id=${container_id}" --format '{{.ID}}' \
                2>>"${log_path}")"; then
                printf '[ERREUR NETTOYAGE] inspection bornée du CID Docker impossible.\n' >&2
                return 1
            fi
            [[ -z "${cid_presence}" || "${cid_presence}" == "${container_id}" ]] || {
                printf '[ERREUR NETTOYAGE] filtre CID Docker ambigu : %s.\n' \
                    "${cid_presence}" >&2
                return 1
            }
        fi
        if ! collision="$("${TIMEOUT_BIN}" \
            --kill-after="${CONTAINER_CLEANUP_KILL_AFTER_SECONDS}s" \
            "${CONTAINER_CLEANUP_TIMEOUT_SECONDS}s" \
            "${DOCKER[@]}" ps -a --no-trunc \
            --filter "name=^/${container_name}$" --format '{{.Names}}' \
            2>>"${log_path}")"; then
            printf '[ERREUR NETTOYAGE] inspection bornée des noms Docker impossible.\n' >&2
            return 1
        fi
        if [[ -z "${cid_presence}" && -z "${collision}" ]]; then
            absence_count=$((absence_count + 1))
            if ((absence_count >= required_absences)); then
                rm -f -- "${cidfile}" || return 1
                return "${cid_evidence_status}"
            fi
        else
            absence_count=0
            [[ -z "${collision}" || "${collision}" == "${container_name}" ]] || {
                printf '[ERREUR NETTOYAGE] collision Docker ambiguë : %s.\n' \
                    "${collision}" >&2
                return 1
            }
            if [[ -n "${cid_presence}" ]]; then
                cleanup_target="${container_id}"
            else
                cleanup_target="${container_name}"
            fi
            if ! evidence="$("${TIMEOUT_BIN}" \
                --kill-after="${CONTAINER_CLEANUP_KILL_AFTER_SECONDS}s" \
                "${CONTAINER_CLEANUP_TIMEOUT_SECONDS}s" \
                "${DOCKER[@]}" inspect \
                --format '{{.Id}}|{{.Name}}|{{.Config.Image}}|{{index .Config.Labels "morsehgp3d.phase3.session"}}' \
                "${cleanup_target}" 2>>"${log_path}")"; then
                printf '[ERREUR NETTOYAGE] attestation bornée du conteneur impossible.\n' >&2
                return 1
            fi
            IFS='|' read -r observed_id observed_name observed_image observed_label extra \
                <<<"${evidence}"
            [[ "${observed_id}" =~ ^[0-9a-f]{64}$ && \
                "${observed_name}" == "/${container_name}" && \
                "${observed_image}" == "${IMAGE_REF}" && \
                "${observed_label}" == "${SESSION_TOKEN}" && -z "${extra}" ]] || {
                printf '[ERREUR NETTOYAGE] nom Docker occupé par une cible non attestée; suppression refusée.\n' >&2
                return 1
            }
            if ((cid_evidence_status == 0)) && \
                [[ -n "${container_id}" && "${observed_id}" != "${container_id}" ]]; then
                printf '[ERREUR NETTOYAGE] le CID attesté diffère du cidfile; suppression refusée.\n' >&2
                return 1
            fi
            cleanup_target="${observed_id}"
            required_absences="${CONTAINER_CLEANUP_POST_RM_ABSENCES}"
            if ! "${TIMEOUT_BIN}" \
                --kill-after="${CONTAINER_CLEANUP_KILL_AFTER_SECONDS}s" \
                "${CONTAINER_CLEANUP_TIMEOUT_SECONDS}s" \
                "${DOCKER[@]}" rm -f "${cleanup_target}" \
                >>"${log_path}" 2>&1; then
                printf '[ERREUR NETTOYAGE] docker rm -f borné a échoué pour %s.\n' \
                    "${cleanup_target}" >&2
                return 1
            fi
        fi
        if ((attempt < CONTAINER_CLEANUP_MAX_ATTEMPTS)); then
            "${SLEEP_BIN}" "${CONTAINER_CLEANUP_RETRY_SECONDS}" || return 1
        fi
    done
    printf '[ERREUR NETTOYAGE] absence stable non certifiée après la grâce bornée.\n' >&2
    return 1
}

cleanup() {
    local original_status=$?
    local cleanup_status=0
    trap - EXIT HUP INT TERM
    if [[ -n "${ACTIVE_CONTAINER_CIDFILE}" ]]; then
        remove_container_from_cidfile \
            "${ACTIVE_CONTAINER_CIDFILE}" \
            "${ACTIVE_CONTAINER_NAME}" \
            "${ACTIVE_CONTAINER_LOG:-/dev/null}" || cleanup_status=$?
        ACTIVE_CONTAINER_CIDFILE=""
        ACTIVE_CONTAINER_LOG=""
        ACTIVE_CONTAINER_NAME=""
    fi
    if [[ -n "${PUBLISH_TEMP}" && -f "${PUBLISH_TEMP}" ]]; then
        rm -f -- "${PUBLISH_TEMP}" || true
    fi
    if [[ -n "${PHASE4_PUBLISH_TEMP}" && -f "${PHASE4_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE4_PUBLISH_TEMP}" || true
    fi
    if [[ -n "${PHASE5_PUBLISH_TEMP}" && -f "${PHASE5_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE5_PUBLISH_TEMP}" || true
    fi
    if [[ -n "${PHASE5_WORK_PROFILE_PUBLISH_TEMP}" && \
        -f "${PHASE5_WORK_PROFILE_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE5_WORK_PROFILE_PUBLISH_TEMP}" || true
    fi
    if [[ -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP}" && \
        -f "${PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP}" || true
    fi
    if [[ -n "${PHASE7_H_POLYTOPE_PUBLISH_TEMP}" && \
        -f "${PHASE7_H_POLYTOPE_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE7_H_POLYTOPE_PUBLISH_TEMP}" || true
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
    if ((REPOSITORY_BUILD_MOUNTPOINT_CREATED == 1)); then
        if [[ "${REPOSITORY_BUILD_MOUNTPOINT}" == "${REPOSITORY_ROOT}/build" && \
            -d "${REPOSITORY_BUILD_MOUNTPOINT}" && \
            ! -L "${REPOSITORY_BUILD_MOUNTPOINT}" ]]; then
            rmdir -- "${REPOSITORY_BUILD_MOUNTPOINT}" || \
                printf '[ATTENTION] Point de montage build non vide; conservation : %s\n' \
                    "${REPOSITORY_BUILD_MOUNTPOINT}" >&2
        else
            printf '[ATTENTION] Nettoyage refusé pour le point de montage build non canonique : %s\n' \
                "${REPOSITORY_BUILD_MOUNTPOINT}" >&2
        fi
    fi
    if ((cleanup_status != 0 && original_status == 0)); then
        original_status="${cleanup_status}"
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
SESSION_TOKEN="${SESSION_DIR##*.}"
SESSION_TOKEN="${SESSION_TOKEN,,}"
[[ "${SESSION_TOKEN}" =~ ^[a-z0-9]{8}$ ]] || \
    die "Jeton de session privée non canonique."
BUILDX_CONFIG="${SESSION_DIR}/buildx-config"
mkdir -p -- "${SESSION_DIR}/build" "${SESSION_DIR}/logs" "${SESSION_DIR}/results" \
    "${SESSION_DIR}/container-cids" "${BUILDX_CONFIG}"
chmod 700 "${SESSION_DIR}/container-cids" "${BUILDX_CONFIG}"
export BUILDX_CONFIG
PUBLISH_TEMP="$(mktemp "${OUTPUT_PARENT}/.${OUTPUT_BASE}.XXXXXXXX.partial")" || \
    die "Impossible de réserver l'artefact temporaire."
chmod 600 "${PUBLISH_TEMP}"
if [[ -n "${PHASE4_OUTPUT_PATH}" ]]; then
    PHASE4_PUBLISH_TEMP="$(mktemp \
        "${PHASE4_OUTPUT_PARENT}/.${PHASE4_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact Phase 4 temporaire."
    chmod 600 "${PHASE4_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE5_OUTPUT_PATH}" ]]; then
    PHASE5_PUBLISH_TEMP="$(mktemp \
        "${PHASE5_OUTPUT_PARENT}/.${PHASE5_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact Phase 5 temporaire."
    chmod 600 "${PHASE5_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" ]]; then
    PHASE5_WORK_PROFILE_PUBLISH_TEMP="$(mktemp \
        "${PHASE5_WORK_PROFILE_OUTPUT_PARENT}/.${PHASE5_WORK_PROFILE_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact temporaire du profil de travail Phase 5."
    chmod 600 "${PHASE5_WORK_PROFILE_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" ]]; then
    PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP="$(mktemp \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PARENT}/.${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact temporaire du profil external-1NN exact Phase 5."
    chmod 600 "${PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" ]]; then
    PHASE7_H_POLYTOPE_PUBLISH_TEMP="$(mktemp \
        "${PHASE7_H_POLYTOPE_OUTPUT_PARENT}/.${PHASE7_H_POLYTOPE_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact temporaire H-polytope Phase 7."
    rm -f -- "${PHASE7_H_POLYTOPE_PUBLISH_TEMP}" || \
        die "Impossible de libérer le nom temporaire H-polytope Phase 7."
    [[ ! -e "${PHASE7_H_POLYTOPE_PUBLISH_TEMP}" && \
        ! -L "${PHASE7_H_POLYTOPE_PUBLISH_TEMP}" ]] || \
        die "Le nom temporaire H-polytope Phase 7 reste occupé."
fi

readonly BUILD_DIR="${SESSION_DIR}/build"
readonly LOG_DIR="${SESSION_DIR}/logs"
readonly RESULT_DIR="${SESSION_DIR}/results"
readonly CONTAINER_CID_DIR="${SESSION_DIR}/container-cids"
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
readonly PHASE4_DIFFERENTIAL_LOG="${LOG_DIR}/phase4-spatial-differential.log"
readonly PHASE4_ELF_LOG="${LOG_DIR}/phase4-spatial-cuobjdump-elf.log"
readonly PHASE4_PTX_LOG="${LOG_DIR}/phase4-spatial-cuobjdump-ptx.log"
readonly PHASE4_PTX_STDERR_LOG="${LOG_DIR}/phase4-spatial-cuobjdump-ptx.stderr.log"
readonly PHASE4_SANITIZER_LOG="${LOG_DIR}/phase4-spatial-compute-sanitizer.log"
readonly PHASE4_DIFFERENTIAL_SUMMARY="${RESULT_DIR}/phase4-spatial-differential.json"
readonly PHASE4_QUICK_SUMMARY="${RESULT_DIR}/phase4-spatial-quick.json"
readonly PHASE4_LBVH_DIFFERENTIAL_LOG="${LOG_DIR}/phase4-spatial-lbvh-differential.log"
readonly PHASE4_LBVH_ELF_LOG="${LOG_DIR}/phase4-spatial-lbvh-cuobjdump-elf.log"
readonly PHASE4_LBVH_PTX_LOG="${LOG_DIR}/phase4-spatial-lbvh-cuobjdump-ptx.log"
readonly PHASE4_LBVH_PTX_STDERR_LOG="${LOG_DIR}/phase4-spatial-lbvh-cuobjdump-ptx.stderr.log"
readonly PHASE4_LBVH_SANITIZER_LOG="${LOG_DIR}/phase4-spatial-lbvh-compute-sanitizer.log"
readonly PHASE4_LBVH_RACECHECK_LOG="${LOG_DIR}/phase4-spatial-lbvh-racecheck.log"
readonly PHASE4_LBVH_DIFFERENTIAL_SUMMARY="${RESULT_DIR}/phase4-spatial-lbvh-differential.json"
readonly PHASE4_LBVH_MEMCHECK_SUMMARY="${RESULT_DIR}/phase4-spatial-lbvh-memcheck.json"
readonly PHASE5_K1_BORUVKA_FULL_REPLAY_LOG="${LOG_DIR}/phase5-k1-boruvka-full-replay.log"
readonly PHASE5_K1_BORUVKA_FULL_REPLAY_STDERR_LOG="${LOG_DIR}/phase5-k1-boruvka-full-replay.stderr.log"
readonly PHASE5_K1_BORUVKA_ELF_LOG="${LOG_DIR}/phase5-k1-boruvka-cuobjdump-elf.log"
readonly PHASE5_K1_BORUVKA_PTX_LOG="${LOG_DIR}/phase5-k1-boruvka-cuobjdump-ptx.log"
readonly PHASE5_K1_BORUVKA_PTX_STDERR_LOG="${LOG_DIR}/phase5-k1-boruvka-cuobjdump-ptx.stderr.log"
readonly PHASE5_K1_BORUVKA_MEMCHECK_LOG="${LOG_DIR}/phase5-k1-boruvka-memcheck.log"
readonly PHASE5_K1_BORUVKA_RACECHECK_LOG="${LOG_DIR}/phase5-k1-boruvka-racecheck.log"
readonly PHASE7_H_POLYTOPE_QUALIFICATION_LOG="${RESULT_DIR}/phase7-h-polytope-qualification.json"
readonly PHASE7_H_POLYTOPE_QUALIFICATION_STDERR_LOG="${LOG_DIR}/phase7-h-polytope-qualification.stderr.log"
readonly PHASE7_H_POLYTOPE_ELF_LOG="${LOG_DIR}/phase7-h-polytope-cuobjdump-elf.log"
readonly PHASE7_H_POLYTOPE_PTX_LOG="${LOG_DIR}/phase7-h-polytope-cuobjdump-ptx.log"
readonly PHASE7_H_POLYTOPE_PTX_STDERR_LOG="${LOG_DIR}/phase7-h-polytope-cuobjdump-ptx.stderr.log"
readonly PHASE7_H_POLYTOPE_MEMCHECK_LOG="${LOG_DIR}/phase7-h-polytope-memcheck.log"
readonly PHASE7_H_POLYTOPE_RACECHECK_LOG="${LOG_DIR}/phase7-h-polytope-racecheck.log"
declare -a PHASE5_K1_BORUVKA_WORK_PROFILE_LOGS=()
declare -a PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_LOGS=()

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
    now_epoch="$("${DATE_BIN}" +%s)"
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
    now="$("${DATE_BIN}" +%s)" || die "Horloge invitée illisible avant l'unité ${label}."
    [[ "${now}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique avant l'unité ${label}."
    ((now < WORK_DEADLINE_EPOCH)) || \
        die "Deadline de travail atteinte; unité ${label} non lancée."
    remaining=$((WORK_DEADLINE_EPOCH - now))
    printf '[DEADLINE] unité=%s, secondes restantes avant GCE-30 min=%s.\n' \
        "${label}" "${remaining}"
}

run_until_work_deadline() {
    local label="$1"
    local now=0
    local remaining=0
    local soft_timeout=0
    shift

    now="$("${DATE_BIN}" +%s)" || {
        printf '[ERREUR] Horloge invitée illisible avant exécution bornée de %s.\n' \
            "${label}" >&2
        return 125
    }
    [[ "${now}" =~ ^[0-9]+$ ]] || {
        printf '[ERREUR] Horloge invitée non numérique avant exécution bornée de %s.\n' \
            "${label}" >&2
        return 125
    }
    remaining=$((WORK_DEADLINE_EPOCH - now))
    soft_timeout=$((remaining - WORK_UNIT_KILL_AFTER_SECONDS - WORK_UNIT_POST_TIMEOUT_RESERVE_SECONDS))
    if ((soft_timeout <= 0)); then
        printf '[ERREUR] Réserve deadline insuffisante; unité %s non lancée.\n' \
            "${label}" >&2
        return 124
    fi
    printf '[TIMEOUT] unité=%s, borne douce=%ss, kill-after=%ss, réserve post-timeout=%ss.\n' \
        "${label}" "${soft_timeout}" "${WORK_UNIT_KILL_AFTER_SECONDS}" \
        "${WORK_UNIT_POST_TIMEOUT_RESERVE_SECONDS}" >&2
    "${TIMEOUT_BIN}" --kill-after="${WORK_UNIT_KILL_AFTER_SECONDS}s" \
        "${soft_timeout}s" "$@"
}

probe_until_work_deadline() {
    local label="$1"
    local maximum_seconds="$2"
    local now=0
    local remaining=0
    local soft_timeout=0
    shift 2

    [[ "${maximum_seconds}" =~ ^[1-9][0-9]*$ ]] || return 125
    now="$("${DATE_BIN}" +%s)" || return 125
    [[ "${now}" =~ ^[0-9]+$ ]] || return 125
    remaining=$((WORK_DEADLINE_EPOCH - now))
    soft_timeout=$((remaining - PROBE_KILL_AFTER_SECONDS - WORK_UNIT_POST_TIMEOUT_RESERVE_SECONDS))
    ((soft_timeout > 0)) || {
        printf '[SONDE REFUSÉE] deadline/réserve atteinte avant %s.\n' "${label}" >&2
        return 124
    }
    if ((soft_timeout > maximum_seconds)); then
        soft_timeout="${maximum_seconds}"
    fi
    "${TIMEOUT_BIN}" --kill-after="${PROBE_KILL_AFTER_SECONDS}s" \
        "${soft_timeout}s" "$@"
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
            "${TIMEOUT_BIN}" --kill-after=2s \
                "${DOCKER_DIAGNOSTIC_TIMEOUT_SECONDS}s" \
                systemctl is-active docker || true
            printf '%s\n' '[systemctl is-enabled docker]'
            "${TIMEOUT_BIN}" --kill-after=2s \
                "${DOCKER_DIAGNOSTIC_TIMEOUT_SECONDS}s" \
                systemctl is-enabled docker || true
        else
            printf '%s\n' 'systemctl absent.'
        fi
        if command -v dpkg-query >/dev/null 2>&1; then
            printf '%s\n' '[paquets Docker/containerd/NVIDIA]'
            "${TIMEOUT_BIN}" --kill-after=2s \
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
                "${TIMEOUT_BIN}" --kill-after=2s \
                    "${DOCKER_DIAGNOSTIC_TIMEOUT_SECONDS}s" \
                    sudo -n -- journalctl --unit=docker.service --boot \
                    --no-pager --lines=80 || true
            else
                "${TIMEOUT_BIN}" --kill-after=2s \
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

certify_docker_client() {
    local access="$1"
    local index=0
    local metadata=""
    local mode=""
    local owner_uid=""
    local path=""
    local parent="${DOCKER_BIN}"
    local -a identity=()
    local -a reverse_paths=()
    local -a expected_paths=()

    case "${access}" in
        direct)
            ;;
        sudo)
            command -v sudo >/dev/null 2>&1 || return 1
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
        probe_until_work_deadline "docker-${access}-dir-${index}" \
            "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
            "${identity[@]}" "${DOCKER_TEST_BIN}" -d "${path}" \
            >>"${DOCKER_LOG}" 2>&1 || return 1
        probe_until_work_deadline "docker-${access}-link-${index}" \
            "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
            "${identity[@]}" "${DOCKER_TEST_BIN}" ! -L "${path}" \
            >>"${DOCKER_LOG}" 2>&1 || return 1
    done
    probe_until_work_deadline "docker-${access}-file" \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
        "${identity[@]}" "${DOCKER_TEST_BIN}" -f "${DOCKER_BIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1
    probe_until_work_deadline "docker-${access}-binary-link" \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
        "${identity[@]}" "${DOCKER_TEST_BIN}" ! -L "${DOCKER_BIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1
    probe_until_work_deadline "docker-${access}-executable" \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
        "${identity[@]}" "${DOCKER_TEST_BIN}" -x "${DOCKER_BIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1
    metadata="$(probe_until_work_deadline "docker-${access}-metadata" \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
        "${identity[@]}" "${DOCKER_STAT_BIN}" -Lc '%n|%u|%a' -- \
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
    probe_until_work_deadline "docker-${access}-version" \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
        "${identity[@]}" "${DOCKER_BIN}" --version \
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
        probe_until_work_deadline "buildx-dir-${index}" \
            "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
            "${identity[@]}" "${BUILDX_TEST_BIN}" -d "${path}" \
            >>"${DOCKER_LOG}" 2>&1 || return 1
        probe_until_work_deadline "buildx-link-${index}" \
            "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
            "${identity[@]}" "${BUILDX_TEST_BIN}" ! -L "${path}" \
            >>"${DOCKER_LOG}" 2>&1 || return 1
    done
    probe_until_work_deadline "buildx-file" \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
        "${identity[@]}" "${BUILDX_TEST_BIN}" -f "${BUILDX_PLUGIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1
    probe_until_work_deadline "buildx-binary-link" \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
        "${identity[@]}" "${BUILDX_TEST_BIN}" ! -L "${BUILDX_PLUGIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1
    probe_until_work_deadline "buildx-executable" \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
        "${identity[@]}" "${BUILDX_TEST_BIN}" -x "${BUILDX_PLUGIN}" \
        >>"${DOCKER_LOG}" 2>&1 || return 1

    metadata="$(probe_until_work_deadline "buildx-metadata" \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
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
    probe_until_work_deadline "buildx-version" \
        "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
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
            "${DOCKER_BIN}" >>"${DOCKER_LOG}"
        if certify_docker_client sudo; then
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
if ! run_until_work_deadline "preflight-blackwell" \
    "${PREFLIGHT_SCRIPT}" --skip-docker >"${PREFLIGHT_LOG}" 2>&1; then
    report_failure_log "preflight-blackwell" "${PREFLIGHT_LOG}"
    die "Le preflight Blackwell non destructif a échoué; voir ${PREFLIGHT_LOG}."
fi

direct_docker_client=0
docker_deadline_reached=0
sudo_docker_client=0
sudo_docker_certification_attempted=0
printf '[CLI DIRECTE] chemin système fixe à certifier : %s; PATH ignoré.\n' \
    "${DOCKER_BIN}" >>"${DOCKER_LOG}"
if certify_docker_client direct; then
    direct_docker_client=1
else
    printf '%s\n' '[CLI DIRECTE] chemin fixe absent, non sûr ou client inexécutable.' \
        >>"${DOCKER_LOG}"
fi
if ((direct_docker_client == 0)); then
    attempt_sudo_docker_certification || true
fi
if ((direct_docker_client == 0 && sudo_docker_client == 0)); then
    collect_docker_host_diagnostics
    report_failure_log "docker-info" "${DOCKER_LOG}"
    die "Client Docker fixe /usr/bin/docker absent, non sûr ou inexécutable directement et via sudo non interactif."
fi
begin_unit "docker-access"
for ((attempt = 1; attempt <= DOCKER_INFO_MAX_ATTEMPTS; attempt++)); do
    probe_now="$("${DATE_BIN}" +%s)" || die "Horloge invitée illisible pendant les sondes Docker."
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
        if probe_until_work_deadline "docker-info-direct-${attempt}" \
            "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
            "${DOCKER_BIN}" info >>"${DOCKER_LOG}" 2>&1; then
            DOCKER=("${DOCKER_BIN}")
            DOCKER_IDENTITY="direct"
            printf '[DOCKER] Daemon accessible directement à la tentative %s/%s.\n' \
                "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}"
            break
        fi
    fi
    probe_now="$("${DATE_BIN}" +%s)" || die "Horloge invitée illisible entre les voies Docker."
    [[ "${probe_now}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique entre les voies Docker."
    if ((probe_now >= WORK_DEADLINE_EPOCH)); then
        docker_deadline_reached=1
        printf '[SONDE DOCKER] deadline atteinte avant voie sudo, tentative=%s/%s.\n' \
            "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}" >>"${DOCKER_LOG}"
        break
    fi
    if ((sudo_docker_certification_attempted == 0)); then
        attempt_sudo_docker_certification || true
        probe_now="$("${DATE_BIN}" +%s)" || die "Horloge invitée illisible après certification Docker sudo."
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
        if probe_until_work_deadline "docker-info-sudo-${attempt}" \
            "${DOCKER_PROBE_TIMEOUT_SECONDS}" \
            sudo -n -- "${DOCKER_BIN}" info >>"${DOCKER_LOG}" 2>&1; then
            DOCKER=(sudo -n -- "${DOCKER_BIN}")
            DOCKER_IDENTITY="sudo"
            printf '[DOCKER] Voie directe indisponible; sudo non interactif certifié à la tentative %s/%s.\n' \
                "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}"
            break
        fi
    fi
    if ((attempt < DOCKER_INFO_MAX_ATTEMPTS)); then
        probe_now="$("${DATE_BIN}" +%s)" || die "Horloge invitée illisible avant l'attente Docker."
        [[ "${probe_now}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique avant l'attente Docker."
        if ((probe_now + DOCKER_INFO_RETRY_SECONDS >= WORK_DEADLINE_EPOCH)); then
            docker_deadline_reached=1
            printf '[SONDE DOCKER] attente refusée par la deadline après tentative=%s/%s.\n' \
                "${attempt}" "${DOCKER_INFO_MAX_ATTEMPTS}" >>"${DOCKER_LOG}"
            break
        fi
        "${SLEEP_BIN}" "${DOCKER_INFO_RETRY_SECONDS}" || \
            die "Attente Docker fixe interrompue."
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

# Docker applique d'abord le bind du dépôt en lecture seule. Le point de
# montage imbriqué doit donc exister dans la source hôte; un répertoire créé
# uniquement dans l'image serait masqué par ce premier bind.
REPOSITORY_BUILD_MOUNTPOINT="${REPOSITORY_ROOT}/build"
if [[ -L "${REPOSITORY_BUILD_MOUNTPOINT}" || \
    (-e "${REPOSITORY_BUILD_MOUNTPOINT}" && \
        ! -d "${REPOSITORY_BUILD_MOUNTPOINT}") ]]; then
    die "Le point de montage build du dépôt doit être un répertoire non symbolique."
fi
if [[ ! -e "${REPOSITORY_BUILD_MOUNTPOINT}" ]]; then
    mkdir -- "${REPOSITORY_BUILD_MOUNTPOINT}" || \
        die "Impossible de réserver le point de montage build dans le dépôt."
    REPOSITORY_BUILD_MOUNTPOINT_CREATED=1
fi
[[ -d "${REPOSITORY_BUILD_MOUNTPOINT}" && \
    ! -L "${REPOSITORY_BUILD_MOUNTPOINT}" ]] || \
    die "Le point de montage build du dépôt n'est pas un répertoire sûr."

IMAGE_REF="morsehgp3d-phase3:${HEAD_SHA}"
begin_unit "buildx-build"
if ! run_until_work_deadline "buildx-build" "${BUILDX[@]}" build --load \
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
inspected_image_id="$(run_until_work_deadline "docker-image-inspect" \
    "${DOCKER[@]}" image inspect --format '{{.Id}}' "${IMAGE_REF}")" || \
    die "Impossible de relire l'identifiant de l'image construite."
[[ "${inspected_image_id}" == "${IMAGE_ID}" ]] || \
    die "L'image taguée ne correspond pas à l'iidfile de construction."
printf 'base_image_ref=%s\nimage_ref=%s\nimage_id=%s\n' \
    "${BASE_IMAGE_REF}" "${IMAGE_REF}" "${IMAGE_ID}" >>"${BUILD_LOG}"

run_container() {
    local label="$1"
    local log_path="$2"
    local cidfile="${CONTAINER_CID_DIR}/${label}.cid"
    local container_name=""
    local collision=""
    local run_status=0
    local cleanup_status=0
    shift 2

    [[ "${label}" =~ ^[a-z0-9][a-z0-9-]*$ ]] || \
        die "Label de conteneur non canonique : ${label}."
    container_name="morsehgp3d-phase3-${SESSION_TOKEN}-${label}"
    [[ -d "${CONTAINER_CID_DIR}" && ! -L "${CONTAINER_CID_DIR}" ]] || \
        die "Répertoire de cidfiles absent ou symbolique : ${CONTAINER_CID_DIR}."
    [[ ! -e "${cidfile}" && ! -L "${cidfile}" ]] || \
        die "Le cidfile doit être inexistant avant docker run : ${cidfile}."
    if ! collision="$(run_until_work_deadline "${label}-name-check" \
        "${DOCKER[@]}" ps -a --no-trunc \
        --filter "name=^/${container_name}$" --format '{{.Names}}' \
        2>>"${log_path}")"; then
        return 125
    fi
    [[ -z "${collision}" ]] || \
        die "Collision de nom Docker refusée avant run : ${container_name}."
    ACTIVE_CONTAINER_CIDFILE="${cidfile}"
    ACTIVE_CONTAINER_LOG="${log_path}"
    ACTIVE_CONTAINER_NAME="${container_name}"
    if run_until_work_deadline "${label}" "${DOCKER[@]}" run \
        --name "${container_name}" \
        --label "${CONTAINER_SESSION_LABEL}=${SESSION_TOKEN}" \
        --cidfile "${cidfile}" --gpus all \
        "${DOCKER_IDENTITY_ARGS[@]}" \
        --volume "${REPOSITORY_ROOT}:${CONTAINER_REPOSITORY}:ro" \
        --volume "${BUILD_DIR}:${CONTAINER_BUILD}:rw" \
        --volume "${RESULT_DIR}:${CONTAINER_RESULTS}:rw" \
        --workdir "${CONTAINER_SOURCE}" \
        --env "HOME=${CONTAINER_HOME}" \
        --env "MORSEHGP3D_CUDA_IMAGE_REF=${IMAGE_REF}" \
        --env "MORSEHGP3D_CUDA_IMAGE_ID=${IMAGE_ID}" \
        --env "MORSEHGP3D_GIT_SHA=${HEAD_SHA}" \
        "${IMAGE_REF}" "$@" >"${log_path}" 2>&1; then
        run_status=0
    else
        run_status=$?
    fi
    if remove_container_from_cidfile \
        "${cidfile}" "${container_name}" "${log_path}"; then
        cleanup_status=0
    else
        cleanup_status=$?
    fi
    if ((cleanup_status == 0 || cleanup_status == 2)); then
        ACTIVE_CONTAINER_CIDFILE=""
        ACTIVE_CONTAINER_LOG=""
        ACTIVE_CONTAINER_NAME=""
    fi
    ((cleanup_status == 0)) || return 125
    return "${run_status}"
}

run_container_split_output() {
    local label="$1"
    local stdout_path="$2"
    local stderr_path="$3"
    local container_entrypoint="$4"
    local cidfile="${CONTAINER_CID_DIR}/${label}.cid"
    local container_name=""
    local collision=""
    local run_status=0
    local cleanup_status=0
    shift 4

    [[ "${label}" =~ ^[a-z0-9][a-z0-9-]*$ ]] || \
        die "Label de conteneur non canonique : ${label}."
    [[ "${container_entrypoint}" == /* ]] || \
        die "Entrypoint de conteneur non absolu : ${container_entrypoint}."
    container_name="morsehgp3d-phase3-${SESSION_TOKEN}-${label}"
    [[ -d "${CONTAINER_CID_DIR}" && ! -L "${CONTAINER_CID_DIR}" ]] || \
        die "Répertoire de cidfiles absent ou symbolique : ${CONTAINER_CID_DIR}."
    [[ ! -e "${cidfile}" && ! -L "${cidfile}" ]] || \
        die "Le cidfile doit être inexistant avant docker run : ${cidfile}."
    if ! collision="$(run_until_work_deadline "${label}-name-check" \
        "${DOCKER[@]}" ps -a --no-trunc \
        --filter "name=^/${container_name}$" --format '{{.Names}}' \
        2>>"${stderr_path}")"; then
        return 125
    fi
    [[ -z "${collision}" ]] || \
        die "Collision de nom Docker refusée avant run : ${container_name}."
    ACTIVE_CONTAINER_CIDFILE="${cidfile}"
    ACTIVE_CONTAINER_LOG="${stderr_path}"
    ACTIVE_CONTAINER_NAME="${container_name}"
    if run_until_work_deadline "${label}" "${DOCKER[@]}" run \
        --name "${container_name}" \
        --label "${CONTAINER_SESSION_LABEL}=${SESSION_TOKEN}" \
        --cidfile "${cidfile}" --gpus all \
        --entrypoint "${container_entrypoint}" \
        "${DOCKER_IDENTITY_ARGS[@]}" \
        --volume "${REPOSITORY_ROOT}:${CONTAINER_REPOSITORY}:ro" \
        --volume "${BUILD_DIR}:${CONTAINER_BUILD}:rw" \
        --volume "${RESULT_DIR}:${CONTAINER_RESULTS}:rw" \
        --workdir "${CONTAINER_SOURCE}" \
        --env "HOME=${CONTAINER_HOME}" \
        --env "MORSEHGP3D_CUDA_IMAGE_REF=${IMAGE_REF}" \
        --env "MORSEHGP3D_CUDA_IMAGE_ID=${IMAGE_ID}" \
        --env "MORSEHGP3D_GIT_SHA=${HEAD_SHA}" \
        "${IMAGE_REF}" "$@" >"${stdout_path}" 2>"${stderr_path}"; then
        run_status=0
    else
        run_status=$?
    fi
    if remove_container_from_cidfile \
        "${cidfile}" "${container_name}" "${stderr_path}"; then
        cleanup_status=0
    else
        cleanup_status=$?
    fi
    if ((cleanup_status == 0 || cleanup_status == 2)); then
        ACTIVE_CONTAINER_CIDFILE=""
        ACTIVE_CONTAINER_LOG=""
        ACTIVE_CONTAINER_NAME=""
    fi
    ((cleanup_status == 0)) || return 125
    return "${run_status}"
}

begin_unit "cuda-release"
if ! run_container "cuda-release" "${RELEASE_LOG}" cmake --workflow --preset cuda-release; then
    report_failure_log "cuda-release" "${RELEASE_LOG}"
    die "Le workflow cuda-release a échoué; voir ${RELEASE_LOG}."
fi
begin_unit "cuda-audit"
if ! run_container "cuda-audit" "${AUDIT_LOG}" cmake --workflow --preset cuda-audit; then
    report_failure_log "cuda-audit" "${AUDIT_LOG}"
    die "Le workflow cuda-audit a échoué; voir ${AUDIT_LOG}."
fi
begin_unit "runtime"
if ! run_container "runtime" "${RUNTIME_LOG}" "${RUNTIME_PATH}" \
    --allocation-bytes 67108864 \
    --exercise-structured-error \
    --output "${CONTAINER_RESULTS}/runtime.jsonl"; then
    report_failure_log "runtime" "${RUNTIME_LOG}"
    die "Le runtime Phase 3 a échoué; voir ${RUNTIME_LOG}."
fi
begin_unit "binding-dlpack"
if ! run_container "binding-dlpack" "${BINDING_LOG}" python3 \
    tests/cuda/check_phase3_binding.py "${MODULE_DIR}"; then
    report_failure_log "binding-dlpack" "${BINDING_LOG}"
    die "Le contrôle de liaison Python/DLPack a échoué; voir ${BINDING_LOG}."
fi
begin_unit "cuobjdump-elf"
if ! run_container "cuobjdump-elf" "${ELF_LOG}" cuobjdump -lelf "${RUNTIME_PATH}"; then
    report_failure_log "cuobjdump-elf" "${ELF_LOG}"
    die "cuobjdump n'a pas pu lister les objets ELF AOT; voir ${ELF_LOG}."
fi
architectures="$(grep -Eo 'sm_[0-9]+' "${ELF_LOG}" | sort -u || true)"
if [[ "${architectures}" != "sm_120" ]]; then
    report_failure_log "cuobjdump-elf" "${ELF_LOG}"
    die "Le binaire AOT doit contenir au moins un ELF et uniquement sm_120; observé : ${architectures:-aucun}."
fi
begin_unit "cuobjdump-ptx"
if ! run_container_split_output "cuobjdump-ptx" "${PTX_LOG}" "${PTX_STDERR_LOG}" \
    /usr/local/cuda/bin/cuobjdump -lptx "${RUNTIME_PATH}"; then
    report_failure_log "cuobjdump-ptx-stderr" "${PTX_STDERR_LOG}"
    die "cuobjdump n'a pas pu auditer les entrées PTX; voir ${PTX_STDERR_LOG}."
fi
if grep -q '[^[:space:]]' "${PTX_LOG}"; then
    report_failure_log "cuobjdump-ptx" "${PTX_LOG}"
    die "Une entrée PTX a été détectée; le runtime mesuré doit être AOT sm_120 uniquement."
fi
begin_unit "compute-sanitizer"
if ! run_container "compute-sanitizer" "${SANITIZER_LOG}" compute-sanitizer \
    --tool memcheck \
    --leak-check full \
    --report-api-errors no \
    --error-exitcode=86 \
    "${RUNTIME_PATH}" \
    --allocation-bytes 4194304 \
    --exercise-structured-error \
    --output "${CONTAINER_RESULTS}/sanitizer-runtime.jsonl"; then
    report_failure_log "compute-sanitizer" "${SANITIZER_LOG}"
    die "compute-sanitizer a détecté une erreur ou a échoué; voir ${SANITIZER_LOG}."
fi

if [[ -n "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" ]]; then
    [[ -f "${BUILD_DIR}/${PHASE7_H_POLYTOPE_BINARY_RELATIVE#build/}" && \
        ! -L "${BUILD_DIR}/${PHASE7_H_POLYTOPE_BINARY_RELATIVE#build/}" && \
        -x "${BUILD_DIR}/${PHASE7_H_POLYTOPE_BINARY_RELATIVE#build/}" ]] || \
        die "Le binaire CUDA H-polytope Phase 7 n'a pas été construit sûrement."

    begin_unit "phase7-h-polytope-qualification"
    if ! run_container_split_output "phase7-h-polytope-qualification" \
        "${PHASE7_H_POLYTOPE_QUALIFICATION_LOG}" \
        "${PHASE7_H_POLYTOPE_QUALIFICATION_STDERR_LOG}" \
        "${PHASE7_H_POLYTOPE_BINARY_PATH}"; then
        report_failure_log "phase7-h-polytope-qualification" \
            "${PHASE7_H_POLYTOPE_QUALIFICATION_LOG}"
        report_failure_log "phase7-h-polytope-qualification-stderr" \
            "${PHASE7_H_POLYTOPE_QUALIFICATION_STDERR_LOG}"
        die "La qualification analytique CUDA H-polytope Phase 7 a échoué."
    fi
    if ! python3 -B - "${PHASE7_H_POLYTOPE_QUALIFICATION_LOG}" <<'PY'
import json
from pathlib import Path
import sys

lines = Path(sys.argv[1]).read_text(encoding="utf-8").splitlines()
if len(lines) != 1 or not lines[0].strip():
    raise SystemExit("qualification stdout must contain exactly one JSON line")
value = json.loads(lines[0])
if not isinstance(value, dict):
    raise SystemExit("qualification stdout JSON must be an object")
PY
    then
        report_failure_log "phase7-h-polytope-qualification" \
            "${PHASE7_H_POLYTOPE_QUALIFICATION_LOG}"
        die "La sortie de qualification H-polytope Phase 7 n'est pas exactement un objet JSON."
    fi

    begin_unit "phase7-h-polytope-cuobjdump-elf"
    if ! run_container "phase7-h-polytope-cuobjdump-elf" \
        "${PHASE7_H_POLYTOPE_ELF_LOG}" /usr/local/cuda/bin/cuobjdump \
        -lelf "${PHASE7_H_POLYTOPE_BINARY_PATH}"; then
        report_failure_log "phase7-h-polytope-cuobjdump-elf" \
            "${PHASE7_H_POLYTOPE_ELF_LOG}"
        die "cuobjdump n'a pas pu lister les ELF du binaire H-polytope Phase 7."
    fi
    phase7_h_polytope_architectures="$(grep -Eo 'sm_[0-9]+' \
        "${PHASE7_H_POLYTOPE_ELF_LOG}" | sort -u || true)"
    if [[ "${phase7_h_polytope_architectures}" != "sm_120" ]]; then
        report_failure_log "phase7-h-polytope-cuobjdump-elf" \
            "${PHASE7_H_POLYTOPE_ELF_LOG}"
        die "Le binaire H-polytope Phase 7 doit contenir uniquement un ELF sm_120; observé : ${phase7_h_polytope_architectures:-aucun}."
    fi

    begin_unit "phase7-h-polytope-cuobjdump-ptx"
    if ! run_container_split_output "phase7-h-polytope-cuobjdump-ptx" \
        "${PHASE7_H_POLYTOPE_PTX_LOG}" \
        "${PHASE7_H_POLYTOPE_PTX_STDERR_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lptx \
        "${PHASE7_H_POLYTOPE_BINARY_PATH}"; then
        report_failure_log "phase7-h-polytope-cuobjdump-ptx-stderr" \
            "${PHASE7_H_POLYTOPE_PTX_STDERR_LOG}"
        die "cuobjdump n'a pas pu auditer le PTX du binaire H-polytope Phase 7."
    fi
    if grep -q '[^[:space:]]' "${PHASE7_H_POLYTOPE_PTX_LOG}"; then
        report_failure_log "phase7-h-polytope-cuobjdump-ptx" \
            "${PHASE7_H_POLYTOPE_PTX_LOG}"
        die "Une entrée PTX a été détectée dans le binaire H-polytope Phase 7."
    fi

    begin_unit "phase7-h-polytope-memcheck"
    if ! run_container "phase7-h-polytope-memcheck" \
        "${PHASE7_H_POLYTOPE_MEMCHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool memcheck \
        --leak-check full \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE7_H_POLYTOPE_BINARY_PATH}"; then
        report_failure_log "phase7-h-polytope-memcheck" \
            "${PHASE7_H_POLYTOPE_MEMCHECK_LOG}"
        die "Le memcheck du binaire H-polytope Phase 7 a échoué."
    fi

    begin_unit "phase7-h-polytope-racecheck"
    if ! run_container "phase7-h-polytope-racecheck" \
        "${PHASE7_H_POLYTOPE_RACECHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool racecheck \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE7_H_POLYTOPE_BINARY_PATH}"; then
        report_failure_log "phase7-h-polytope-racecheck" \
            "${PHASE7_H_POLYTOPE_RACECHECK_LOG}"
        die "Le racecheck du binaire H-polytope Phase 7 a échoué."
    fi
fi

if [[ -n "${PHASE4_OUTPUT_PATH}" ]]; then
    begin_unit "phase4-spatial-differential"
    if ! run_container "phase4-spatial-differential" \
        "${PHASE4_DIFFERENTIAL_LOG}" /usr/bin/python3 -B \
        "${PHASE4_DIFFERENTIAL_PATH}" "${PHASE4_REPLAY_PATH}" \
        --timeout 300 \
        --summary-json "${CONTAINER_RESULTS}/phase4-spatial-differential.json"; then
        report_failure_log "phase4-spatial-differential" "${PHASE4_DIFFERENTIAL_LOG}"
        die "Le différentiel spatial Phase 4 a échoué; voir ${PHASE4_DIFFERENTIAL_LOG}."
    fi

    begin_unit "phase4-spatial-cuobjdump-elf"
    if ! run_container "phase4-spatial-cuobjdump-elf" "${PHASE4_ELF_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lelf "${PHASE4_REPLAY_PATH}"; then
        report_failure_log "phase4-spatial-cuobjdump-elf" "${PHASE4_ELF_LOG}"
        die "cuobjdump n'a pas pu lister les ELF du replay spatial Phase 4."
    fi
    phase4_architectures="$(grep -Eo 'sm_[0-9]+' "${PHASE4_ELF_LOG}" | sort -u || true)"
    if [[ "${phase4_architectures}" != "sm_120" ]]; then
        report_failure_log "phase4-spatial-cuobjdump-elf" "${PHASE4_ELF_LOG}"
        die "Le replay spatial doit contenir au moins un ELF et uniquement sm_120; observé : ${phase4_architectures:-aucun}."
    fi

    begin_unit "phase4-spatial-cuobjdump-ptx"
    if ! run_container_split_output "phase4-spatial-cuobjdump-ptx" \
        "${PHASE4_PTX_LOG}" "${PHASE4_PTX_STDERR_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lptx "${PHASE4_REPLAY_PATH}"; then
        report_failure_log "phase4-spatial-cuobjdump-ptx-stderr" \
            "${PHASE4_PTX_STDERR_LOG}"
        die "cuobjdump n'a pas pu auditer le PTX du replay spatial Phase 4."
    fi
    if grep -q '[^[:space:]]' "${PHASE4_PTX_LOG}"; then
        report_failure_log "phase4-spatial-cuobjdump-ptx" "${PHASE4_PTX_LOG}"
        die "Une entrée PTX a été détectée dans le replay spatial Phase 4."
    fi

    begin_unit "phase4-spatial-compute-sanitizer"
    if ! run_container "phase4-spatial-compute-sanitizer" \
        "${PHASE4_SANITIZER_LOG}" /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool memcheck \
        --leak-check full \
        --error-exitcode=86 \
        /usr/bin/python3 -B \
        "${PHASE4_DIFFERENTIAL_PATH}" "${PHASE4_REPLAY_PATH}" \
        --quick \
        --timeout 300 \
        --summary-json "${CONTAINER_RESULTS}/phase4-spatial-quick.json"; then
        report_failure_log "phase4-spatial-compute-sanitizer" \
            "${PHASE4_SANITIZER_LOG}"
        die "Le memcheck borné du replay spatial Phase 4 a échoué."
    fi
    [[ -s "${PHASE4_DIFFERENTIAL_SUMMARY}" ]] || \
        die "Le différentiel spatial complet n'a pas produit son résumé JSON."
    [[ -s "${PHASE4_QUICK_SUMMARY}" ]] || \
        die "Le memcheck spatial borné n'a pas produit son résumé JSON."

    begin_unit "phase4-spatial-lbvh-differential"
    if ! run_container "phase4-spatial-lbvh-differential" \
        "${PHASE4_LBVH_DIFFERENTIAL_LOG}" /usr/bin/python3 -B \
        "${PHASE4_LBVH_DIFFERENTIAL_PATH}" "${PHASE4_LBVH_REPLAY_PATH}" \
        --timeout 300 \
        --summary-json "${CONTAINER_RESULTS}/phase4-spatial-lbvh-differential.json"; then
        report_failure_log "phase4-spatial-lbvh-differential" \
            "${PHASE4_LBVH_DIFFERENTIAL_LOG}"
        die "Le différentiel LBVH résident Phase 4 a échoué."
    fi

    begin_unit "phase4-spatial-lbvh-cuobjdump-elf"
    if ! run_container "phase4-spatial-lbvh-cuobjdump-elf" \
        "${PHASE4_LBVH_ELF_LOG}" /usr/local/cuda/bin/cuobjdump -lelf \
        "${PHASE4_LBVH_REPLAY_PATH}"; then
        report_failure_log "phase4-spatial-lbvh-cuobjdump-elf" \
            "${PHASE4_LBVH_ELF_LOG}"
        die "cuobjdump n'a pas pu lister les ELF du replay LBVH résident."
    fi
    phase4_lbvh_architectures="$(grep -Eo 'sm_[0-9]+' \
        "${PHASE4_LBVH_ELF_LOG}" | sort -u || true)"
    if [[ "${phase4_lbvh_architectures}" != "sm_120" ]]; then
        report_failure_log "phase4-spatial-lbvh-cuobjdump-elf" \
            "${PHASE4_LBVH_ELF_LOG}"
        die "Le replay LBVH résident doit contenir uniquement un ELF sm_120; observé : ${phase4_lbvh_architectures:-aucun}."
    fi

    begin_unit "phase4-spatial-lbvh-cuobjdump-ptx"
    if ! run_container_split_output "phase4-spatial-lbvh-cuobjdump-ptx" \
        "${PHASE4_LBVH_PTX_LOG}" "${PHASE4_LBVH_PTX_STDERR_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lptx "${PHASE4_LBVH_REPLAY_PATH}"; then
        report_failure_log "phase4-spatial-lbvh-cuobjdump-ptx-stderr" \
            "${PHASE4_LBVH_PTX_STDERR_LOG}"
        die "cuobjdump n'a pas pu auditer le PTX du replay LBVH résident."
    fi
    if grep -q '[^[:space:]]' "${PHASE4_LBVH_PTX_LOG}"; then
        report_failure_log "phase4-spatial-lbvh-cuobjdump-ptx" \
            "${PHASE4_LBVH_PTX_LOG}"
        die "Une entrée PTX a été détectée dans le replay LBVH résident."
    fi

    begin_unit "phase4-spatial-lbvh-compute-sanitizer"
    if ! run_container "phase4-spatial-lbvh-compute-sanitizer" \
        "${PHASE4_LBVH_SANITIZER_LOG}" /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool memcheck \
        --leak-check full \
        --error-exitcode=86 \
        /usr/bin/python3 -B \
        "${PHASE4_LBVH_DIFFERENTIAL_PATH}" "${PHASE4_LBVH_REPLAY_PATH}" \
        --quick \
        --timeout 300 \
        --summary-json "${CONTAINER_RESULTS}/phase4-spatial-lbvh-memcheck.json"; then
        report_failure_log "phase4-spatial-lbvh-compute-sanitizer" \
            "${PHASE4_LBVH_SANITIZER_LOG}"
        die "Le memcheck borné du replay LBVH résident Phase 4 a échoué."
    fi

    begin_unit "phase4-spatial-lbvh-racecheck"
    if ! run_container "phase4-spatial-lbvh-racecheck" \
        "${PHASE4_LBVH_RACECHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool racecheck \
        --error-exitcode=86 \
        /usr/bin/python3 -B \
        "${PHASE4_LBVH_DIFFERENTIAL_PATH}" "${PHASE4_LBVH_REPLAY_PATH}" \
        --quick \
        --timeout 300; then
        report_failure_log "phase4-spatial-lbvh-racecheck" \
            "${PHASE4_LBVH_RACECHECK_LOG}"
        die "Le racecheck borné du replay LBVH résident Phase 4 a échoué."
    fi
    [[ -s "${PHASE4_LBVH_DIFFERENTIAL_SUMMARY}" ]] || \
        die "Le différentiel LBVH résident n'a pas produit son résumé JSON."
    [[ -s "${PHASE4_LBVH_MEMCHECK_SUMMARY}" ]] || \
        die "Le memcheck LBVH résident n'a pas produit son résumé JSON."
fi

if [[ -n "${PHASE5_OUTPUT_PATH}" ]]; then
    begin_unit "phase5-k1-boruvka-full-replay"
    if ! run_container_split_output "phase5-k1-boruvka-full-replay" \
        "${PHASE5_K1_BORUVKA_FULL_REPLAY_LOG}" \
        "${PHASE5_K1_BORUVKA_FULL_REPLAY_STDERR_LOG}" \
        "${PHASE5_K1_BORUVKA_FULL_REPLAY_PATH}"; then
        report_failure_log "phase5-k1-boruvka-full-replay" \
            "${PHASE5_K1_BORUVKA_FULL_REPLAY_LOG}"
        report_failure_log "phase5-k1-boruvka-full-replay-stderr" \
            "${PHASE5_K1_BORUVKA_FULL_REPLAY_STDERR_LOG}"
        die "Le replay réel de la boucle K1 Boruvka complète Phase 5 a échoué."
    fi

    begin_unit "phase5-k1-boruvka-cuobjdump-elf"
    if ! run_container "phase5-k1-boruvka-cuobjdump-elf" \
        "${PHASE5_K1_BORUVKA_ELF_LOG}" /usr/local/cuda/bin/cuobjdump \
        -lelf "${PHASE5_K1_BORUVKA_FULL_REPLAY_PATH}"; then
        report_failure_log "phase5-k1-boruvka-cuobjdump-elf" \
            "${PHASE5_K1_BORUVKA_ELF_LOG}"
        die "cuobjdump n'a pas pu lister les ELF du replay K1 Boruvka complet Phase 5."
    fi
    phase5_k1_boruvka_architectures="$(grep -Eo 'sm_[0-9]+' \
        "${PHASE5_K1_BORUVKA_ELF_LOG}" | sort -u || true)"
    if [[ "${phase5_k1_boruvka_architectures}" != "sm_120" ]]; then
        report_failure_log "phase5-k1-boruvka-cuobjdump-elf" \
            "${PHASE5_K1_BORUVKA_ELF_LOG}"
        die "Le replay K1 Boruvka complet Phase 5 doit contenir uniquement un ELF sm_120; observé : ${phase5_k1_boruvka_architectures:-aucun}."
    fi

    begin_unit "phase5-k1-boruvka-cuobjdump-ptx"
    if ! run_container_split_output "phase5-k1-boruvka-cuobjdump-ptx" \
        "${PHASE5_K1_BORUVKA_PTX_LOG}" \
        "${PHASE5_K1_BORUVKA_PTX_STDERR_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lptx \
        "${PHASE5_K1_BORUVKA_FULL_REPLAY_PATH}"; then
        report_failure_log "phase5-k1-boruvka-cuobjdump-ptx-stderr" \
            "${PHASE5_K1_BORUVKA_PTX_STDERR_LOG}"
        die "cuobjdump n'a pas pu auditer le PTX du replay K1 Boruvka complet Phase 5."
    fi
    if grep -q '[^[:space:]]' "${PHASE5_K1_BORUVKA_PTX_LOG}"; then
        report_failure_log "phase5-k1-boruvka-cuobjdump-ptx" \
            "${PHASE5_K1_BORUVKA_PTX_LOG}"
        die "Une entrée PTX a été détectée dans le replay K1 Boruvka complet Phase 5."
    fi

    begin_unit "phase5-k1-boruvka-memcheck"
    if ! run_container "phase5-k1-boruvka-memcheck" \
        "${PHASE5_K1_BORUVKA_MEMCHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool memcheck \
        --leak-check full \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE5_K1_BORUVKA_FULL_REPLAY_PATH}"; then
        report_failure_log "phase5-k1-boruvka-memcheck" \
            "${PHASE5_K1_BORUVKA_MEMCHECK_LOG}"
        die "Le memcheck du replay K1 Boruvka complet Phase 5 a échoué."
    fi

    begin_unit "phase5-k1-boruvka-racecheck"
    if ! run_container "phase5-k1-boruvka-racecheck" \
        "${PHASE5_K1_BORUVKA_RACECHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool racecheck \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE5_K1_BORUVKA_FULL_REPLAY_PATH}"; then
        report_failure_log "phase5-k1-boruvka-racecheck" \
            "${PHASE5_K1_BORUVKA_RACECHECK_LOG}"
        die "Le racecheck du replay K1 Boruvka complet Phase 5 a échoué."
    fi
fi

if [[ -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" ]]; then
    begin_unit "phase5-exact-search-profile-build"
    if ! run_container "phase5-exact-search-profile-build" \
        "${LOG_DIR}/phase5-exact-search-profile-build.log" \
        cmake --build "${MODULE_DIR}" \
        --target morsehgp3d_gpu_k1_boruvka_exact_search_work_profile \
        --parallel 4; then
        report_failure_log "phase5-exact-search-profile-build" \
            "${LOG_DIR}/phase5-exact-search-profile-build.log"
        die "La cible CUDA du profil external-1NN exact Phase 5 n'a pas pu être construite."
    fi
    [[ -f "${BUILD_DIR}/${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_RELATIVE#build/}" && \
        ! -L "${BUILD_DIR}/${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_RELATIVE#build/}" && \
        -x "${BUILD_DIR}/${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_RELATIVE#build/}" ]] || \
        die "Le binaire CUDA du profil external-1NN exact Phase 5 n'a pas été construit sûrement."
    for point_count in 1024 4096 16384; do
        for family in uniform clusters lattice; do
            exact_search_profile_label="phase5-exact-search-profile-${point_count}-${family}"
            exact_search_profile_log="${RESULT_DIR}/${exact_search_profile_label}.json"
            exact_search_profile_stderr_log="${LOG_DIR}/${exact_search_profile_label}.stderr.log"
            exact_search_profile_checker_log="${LOG_DIR}/${exact_search_profile_label}.checker.log"
            PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_LOGS+=(
                "${exact_search_profile_log}"
            )
            begin_unit "${exact_search_profile_label}"
            if ! run_container_split_output "${exact_search_profile_label}" \
                "${exact_search_profile_log}" \
                "${exact_search_profile_stderr_log}" \
                "${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_PATH}" \
                --family "${family}" \
                --point-count "${point_count}" \
                --window-radius 16 \
                --seed 1 \
                --git-sha "${HEAD_SHA}"; then
                report_failure_log "${exact_search_profile_label}" \
                    "${exact_search_profile_log}"
                report_failure_log "${exact_search_profile_label}-stderr" \
                    "${exact_search_profile_stderr_log}"
                die "Le profil external-1NN exact Phase 5 a échoué pour n=${point_count}, famille=${family}."
            fi
            [[ -s "${exact_search_profile_log}" ]] || \
                die "Le profil external-1NN exact Phase 5 est vide pour n=${point_count}, famille=${family}."
            begin_unit "${exact_search_profile_label}-checker"
            if ! run_until_work_deadline "${exact_search_profile_label}-checker" \
                python3 -B \
                "${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_CHECKER}" \
                --profile-log "${exact_search_profile_log}" \
                --expected-backend cuda_g4 \
                --git-sha "${HEAD_SHA}" \
                --family "${family}" \
                --point-count "${point_count}" \
                --window-radius 16 \
                --seed 1 >"${exact_search_profile_checker_log}" 2>&1; then
                report_failure_log "${exact_search_profile_label}-checker" \
                    "${exact_search_profile_checker_log}"
                die "Le checker a rejeté le profil external-1NN exact Phase 5 pour n=${point_count}, famille=${family}."
            fi
        done
    done
    [[ "${#PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_LOGS[@]}" -eq 9 ]] || \
        die "La matrice du profil external-1NN exact Phase 5 doit contenir exactement neuf mesures."
fi

if [[ -n "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" ]]; then
    [[ -f "${BUILD_DIR}/morsehgp3d-cuda-release/morsehgp3d_gpu_k1_boruvka_morton_work_profile" && \
        ! -L "${BUILD_DIR}/morsehgp3d-cuda-release/morsehgp3d_gpu_k1_boruvka_morton_work_profile" && \
        -x "${BUILD_DIR}/morsehgp3d-cuda-release/morsehgp3d_gpu_k1_boruvka_morton_work_profile" ]] || \
        die "Le binaire CUDA du profil de travail Morton Phase 5 n'a pas été construit sûrement."
    for point_count in 64 256 1024; do
        candidate_record_budget=$((point_count - 1))
        for family in uniform clusters lattice; do
            work_profile_label="phase5-work-profile-${point_count}-${family}"
            work_profile_log="${RESULT_DIR}/${work_profile_label}.json"
            work_profile_stderr_log="${LOG_DIR}/${work_profile_label}.stderr.log"
            PHASE5_K1_BORUVKA_WORK_PROFILE_LOGS+=("${work_profile_log}")
            begin_unit "${work_profile_label}"
            if ! run_container_split_output "${work_profile_label}" \
                "${work_profile_log}" "${work_profile_stderr_log}" \
                "${PHASE5_K1_BORUVKA_WORK_PROFILE_PATH}" \
                --family "${family}" \
                --point-count "${point_count}" \
                --candidate-record-budget "${candidate_record_budget}" \
                --window-radii 1,4,16 \
                --seed 1 \
                --git-sha "${HEAD_SHA}"; then
                report_failure_log "${work_profile_label}" "${work_profile_log}"
                report_failure_log "${work_profile_label}-stderr" \
                    "${work_profile_stderr_log}"
                die "Le profil de travail Morton Phase 5 a échoué pour n=${point_count}, famille=${family}."
            fi
            [[ -s "${work_profile_log}" ]] || \
                die "Le profil de travail Morton Phase 5 est vide pour n=${point_count}, famille=${family}."
        done
    done
    [[ "${#PHASE5_K1_BORUVKA_WORK_PROFILE_LOGS[@]}" -eq 9 ]] || \
        die "La matrice du profil de travail Morton Phase 5 doit contenir exactement neuf mesures."
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

if [[ -n "${PHASE4_OUTPUT_PATH}" ]]; then
    python3 -B "${PHASE4_ASSEMBLER}" \
        --git-sha "${HEAD_SHA}" \
        --base-image-ref "${BASE_IMAGE_REF}" \
        --image-ref "${IMAGE_REF}" \
        --image-id "${IMAGE_ID}" \
        --environment-artifact "${PUBLISH_TEMP}" \
        --differential-summary "${PHASE4_DIFFERENTIAL_SUMMARY}" \
        --quick-summary "${PHASE4_QUICK_SUMMARY}" \
        --differential-log "${PHASE4_DIFFERENTIAL_LOG}" \
        --elf-log "${PHASE4_ELF_LOG}" \
        --ptx-log "${PHASE4_PTX_LOG}" \
        --ptx-stderr-log "${PHASE4_PTX_STDERR_LOG}" \
        --sanitizer-log "${PHASE4_SANITIZER_LOG}" \
        --replay "${BUILD_DIR}/morsehgp3d-cuda-release/morsehgp3d_gpu_spatial_reference_replay" \
        --checker "${PHASE4_DIFFERENTIAL}" \
        --lbvh-differential-summary "${PHASE4_LBVH_DIFFERENTIAL_SUMMARY}" \
        --lbvh-memcheck-summary "${PHASE4_LBVH_MEMCHECK_SUMMARY}" \
        --lbvh-differential-log "${PHASE4_LBVH_DIFFERENTIAL_LOG}" \
        --lbvh-elf-log "${PHASE4_LBVH_ELF_LOG}" \
        --lbvh-ptx-log "${PHASE4_LBVH_PTX_LOG}" \
        --lbvh-ptx-stderr-log "${PHASE4_LBVH_PTX_STDERR_LOG}" \
        --lbvh-sanitizer-log "${PHASE4_LBVH_SANITIZER_LOG}" \
        --lbvh-racecheck-log "${PHASE4_LBVH_RACECHECK_LOG}" \
        --lbvh-replay "${BUILD_DIR}/morsehgp3d-cuda-release/morsehgp3d_gpu_spatial_lbvh_replay" \
        --lbvh-checker "${PHASE4_LBVH_DIFFERENTIAL}" \
        --output "${PHASE4_PUBLISH_TEMP}"
fi

if [[ -n "${PHASE5_OUTPUT_PATH}" ]]; then
    python3 -B "${PHASE5_K1_BORUVKA_ASSEMBLER}" \
        --git-sha "${HEAD_SHA}" \
        --base-image-ref "${BASE_IMAGE_REF}" \
        --image-ref "${IMAGE_REF}" \
        --image-id "${IMAGE_ID}" \
        --environment-artifact "${PUBLISH_TEMP}" \
        --replay-log "${PHASE5_K1_BORUVKA_FULL_REPLAY_LOG}" \
        --elf-log "${PHASE5_K1_BORUVKA_ELF_LOG}" \
        --ptx-log "${PHASE5_K1_BORUVKA_PTX_LOG}" \
        --ptx-stderr-log "${PHASE5_K1_BORUVKA_PTX_STDERR_LOG}" \
        --memcheck-log "${PHASE5_K1_BORUVKA_MEMCHECK_LOG}" \
        --racecheck-log "${PHASE5_K1_BORUVKA_RACECHECK_LOG}" \
        --replay "${BUILD_DIR}/morsehgp3d-cuda-release/morsehgp3d_gpu_k1_boruvka_full_replay" \
        --output "${PHASE5_PUBLISH_TEMP}"
fi

if [[ -n "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" ]]; then
    declare -a phase5_work_profile_assembler_arguments=(
        --git-sha "${HEAD_SHA}"
        --base-image-ref "${BASE_IMAGE_REF}"
        --image-ref "${IMAGE_REF}"
        --image-id "${IMAGE_ID}"
        --environment-artifact "${PUBLISH_TEMP}"
    )
    for work_profile_log in "${PHASE5_K1_BORUVKA_WORK_PROFILE_LOGS[@]}"; do
        phase5_work_profile_assembler_arguments+=(
            --profile-log "${work_profile_log}"
        )
    done
    phase5_work_profile_assembler_arguments+=(
        --binary "${BUILD_DIR}/morsehgp3d-cuda-release/morsehgp3d_gpu_k1_boruvka_morton_work_profile"
        --output "${PHASE5_WORK_PROFILE_PUBLISH_TEMP}"
    )
    python3 -B "${PHASE5_K1_BORUVKA_WORK_PROFILE_ASSEMBLER}" \
        "${phase5_work_profile_assembler_arguments[@]}"
fi

if [[ -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" ]]; then
    declare -a phase5_exact_search_work_profile_assembler_arguments=(
        --git-sha "${HEAD_SHA}"
        --base-image-ref "${BASE_IMAGE_REF}"
        --image-ref "${IMAGE_REF}"
        --image-id "${IMAGE_ID}"
        --environment-artifact "${PUBLISH_TEMP}"
    )
    for exact_search_profile_log in \
        "${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_LOGS[@]}"; do
        phase5_exact_search_work_profile_assembler_arguments+=(
            --profile-log "${exact_search_profile_log}"
        )
    done
    phase5_exact_search_work_profile_assembler_arguments+=(
        --binary "${BUILD_DIR}/${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_RELATIVE#build/}"
        --output "${PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP}"
    )
    python3 -B "${PHASE5_K1_BORUVKA_EXACT_SEARCH_WORK_PROFILE_ASSEMBLER}" \
        "${phase5_exact_search_work_profile_assembler_arguments[@]}"
fi

if [[ -n "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" ]]; then
    python3 -B "${PHASE7_H_POLYTOPE_ASSEMBLER}" \
        --git-sha "${HEAD_SHA}" \
        --base-image-ref "${BASE_IMAGE_REF}" \
        --image-ref "${IMAGE_REF}" \
        --image-id "${IMAGE_ID}" \
        --environment-artifact "${PUBLISH_TEMP}" \
        --qualification-log "${PHASE7_H_POLYTOPE_QUALIFICATION_LOG}" \
        --elf-log "${PHASE7_H_POLYTOPE_ELF_LOG}" \
        --ptx-log "${PHASE7_H_POLYTOPE_PTX_LOG}" \
        --ptx-stderr-log "${PHASE7_H_POLYTOPE_PTX_STDERR_LOG}" \
        --memcheck-log "${PHASE7_H_POLYTOPE_MEMCHECK_LOG}" \
        --racecheck-log "${PHASE7_H_POLYTOPE_RACECHECK_LOG}" \
        --binary "${BUILD_DIR}/${PHASE7_H_POLYTOPE_BINARY_RELATIVE#build/}" \
        --output "${PHASE7_H_POLYTOPE_PUBLISH_TEMP}"
fi

python3 - "${PUBLISH_TEMP}" "${OUTPUT_PATH}" \
    "${PHASE4_PUBLISH_TEMP}" "${PHASE4_OUTPUT_PATH}" \
    "${PHASE5_PUBLISH_TEMP}" "${PHASE5_OUTPUT_PATH}" \
    "${PHASE5_WORK_PROFILE_PUBLISH_TEMP}" \
    "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" \
    "${PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP}" \
    "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" \
    "${PHASE7_H_POLYTOPE_PUBLISH_TEMP}" \
    "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" <<'PY'
import json
import os
from pathlib import Path
import stat
import sys

pairs = [(Path(sys.argv[1]), Path(sys.argv[2]), "Phase 3")]
if sys.argv[3] or sys.argv[4]:
    if not sys.argv[3] or not sys.argv[4]:
        raise SystemExit("incomplete Phase 4 publication pair")
    pairs.append((Path(sys.argv[3]), Path(sys.argv[4]), "Phase 4"))
if sys.argv[5] or sys.argv[6]:
    if not sys.argv[5] or not sys.argv[6]:
        raise SystemExit("incomplete Phase 5 publication pair")
    pairs.append((Path(sys.argv[5]), Path(sys.argv[6]), "Phase 5"))
if sys.argv[7] or sys.argv[8]:
    if not sys.argv[7] or not sys.argv[8]:
        raise SystemExit("incomplete Phase 5 Morton work-profile publication pair")
    pairs.append(
        (Path(sys.argv[7]), Path(sys.argv[8]), "Phase 5 Morton work profile")
    )
if sys.argv[9] or sys.argv[10]:
    if not sys.argv[9] or not sys.argv[10]:
        raise SystemExit("incomplete Phase 5 exact-search work-profile publication pair")
    pairs.append(
        (Path(sys.argv[9]), Path(sys.argv[10]), "Phase 5 exact-search work profile")
    )
if sys.argv[11] or sys.argv[12]:
    if not sys.argv[11] or not sys.argv[12]:
        raise SystemExit("incomplete Phase 7 H-polytope publication pair")
    pairs.append(
        (Path(sys.argv[11]), Path(sys.argv[12]), "Phase 7 H-polytope")
    )
for temporary, _, label in pairs:
    with temporary.open(encoding="utf-8") as stream:
        value = json.load(stream)
    if not isinstance(value, dict) or value.get("status") != "worker_passed_pending_shutdown":
        raise SystemExit(f"the {label} worker artifact is not pending targeted shutdown")

published = []
try:
    for temporary, target, label in pairs:
        os.link(temporary, target, follow_symlinks=False)
        published.append((temporary, target, label))
    for parent in {target.parent for _, target, _ in pairs}:
        directory_fd = os.open(parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(directory_fd)
        finally:
            os.close(directory_fd)
except OSError as error:
    rollback_failures = []
    for temporary, target, label in reversed(published):
        try:
            source_stat = os.stat(temporary, follow_symlinks=False)
            target_stat = os.stat(target, follow_symlinks=False)
            if (
                not stat.S_ISREG(source_stat.st_mode)
                or not stat.S_ISREG(target_stat.st_mode)
                or (source_stat.st_dev, source_stat.st_ino)
                != (target_stat.st_dev, target_stat.st_ino)
            ):
                rollback_failures.append(f"{label}:identity-changed")
                continue
            os.unlink(target)
        except OSError as rollback_error:
            rollback_failures.append(f"{label}:{rollback_error}")
    published_label = ",".join(str(target) for _, target, _ in published) or "none"
    rollback_label = ",".join(rollback_failures) or "complete"
    raise SystemExit(
        f"worker artifact publication failed: {error}; published={published_label}; "
        f"rollback={rollback_label}"
    )
PY
rm -f -- "${PUBLISH_TEMP}"
if [[ -n "${PHASE4_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE4_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE5_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE5_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE5_WORK_PROFILE_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE5_WORK_PROFILE_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE7_H_POLYTOPE_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE7_H_POLYTOPE_PUBLISH_TEMP}"
fi
PUBLISH_TEMP=""
PHASE4_PUBLISH_TEMP=""
PHASE5_PUBLISH_TEMP=""
PHASE5_WORK_PROFILE_PUBLISH_TEMP=""
PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP=""
PHASE7_H_POLYTOPE_PUBLISH_TEMP=""

printf '[SUCCÈS WORKER] Artefact Phase 3 provisoire publié sans remplacement : %s\n' \
    "${OUTPUT_PATH}"
if [[ -n "${PHASE4_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Compagnon Phase 4 provisoire publié sans remplacement : %s\n' \
        "${PHASE4_OUTPUT_PATH}"
fi
if [[ -n "${PHASE5_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Compagnon Phase 5 provisoire publié sans remplacement : %s\n' \
        "${PHASE5_OUTPUT_PATH}"
fi
if [[ -n "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Profil de travail Morton Phase 5 provisoire publié sans remplacement : %s\n' \
        "${PHASE5_WORK_PROFILE_OUTPUT_PATH}"
fi
if [[ -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Profil external-1NN exact Phase 5 provisoire publié sans remplacement : %s\n' \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}"
fi
if [[ -n "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Compagnon H-polytope Phase 7 provisoire publié sans remplacement : %s\n' \
        "${PHASE7_H_POLYTOPE_OUTPUT_PATH}"
fi
printf '[CYCLE DE VIE] Le worker invité ne ferme pas la VM; l’orchestrateur externe doit certifier TERMINATED.\n'
