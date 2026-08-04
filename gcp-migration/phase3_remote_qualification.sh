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
readonly PHASE9_PAIR_SUPPORT_PHI_ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase9_pair_support_phi_qualification.py"
readonly PHASE15_EXACT_DIAMETRAL_PHI_ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase15_exact_diametral_phi_qualification.py"
readonly PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase15_device_frontier_50k_qualification.py"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase15_device_frontier_strict_interior_qualification.py"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase15_ranked_pair_classifier_qualification.py"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase15_exact_pair_block_witness_cuda_qualification.py"
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
readonly PHASE9_PAIR_SUPPORT_PHI_BINARY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_pair_support_phi_qualification"
readonly PHASE9_PAIR_SUPPORT_PHI_BINARY_PATH="${CONTAINER_REPOSITORY}/${PHASE9_PAIR_SUPPORT_PHI_BINARY_RELATIVE}"
readonly PHASE15_EXACT_DIAMETRAL_PHI_BINARY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_exact_diametral_phi_qualification"
readonly PHASE15_EXACT_DIAMETRAL_PHI_BINARY_PATH="${CONTAINER_REPOSITORY}/${PHASE15_EXACT_DIAMETRAL_PHI_BINARY_RELATIVE}"
readonly PHASE15_DEVICE_FRONTIER_50K_BINARY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification"
readonly PHASE15_DEVICE_FRONTIER_50K_BINARY_PATH="${CONTAINER_REPOSITORY}/${PHASE15_DEVICE_FRONTIER_50K_BINARY_RELATIVE}"
readonly PHASE15_DEVICE_FRONTIER_50K_SEED=1558325537444281125
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_RELATIVE="${PHASE15_DEVICE_FRONTIER_50K_BINARY_RELATIVE}"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_PATH="${CONTAINER_REPOSITORY}/${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_RELATIVE}"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_qualification"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_PATH="${CONTAINER_REPOSITORY}/${PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_RELATIVE}"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_exact_pair_block_witness_cuda_qualification"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_PATH="${CONTAINER_REPOSITORY}/${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_RELATIVE}"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_BINARY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_qualification"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_BINARY_PATH="${CONTAINER_REPOSITORY}/${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_BINARY_RELATIVE}"
readonly PHASE15_REDUCER_SOURCE_BINARY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_exact_pair_block_to_reducer_source_cuda_qualification"
readonly PHASE15_REDUCER_SOURCE_BINARY_PATH="${CONTAINER_REPOSITORY}/${PHASE15_REDUCER_SOURCE_BINARY_RELATIVE}"
readonly MORTON_YAO48_RADIAL_SUBTREE_FILTER_BINARY_RELATIVE="build/morsehgp3d-cuda-release/morsehgp3d_gpu_morton_yao48_radial_subtree_filter_qualification"
readonly MORTON_YAO48_RADIAL_SUBTREE_FILTER_BINARY_PATH="${CONTAINER_REPOSITORY}/${MORTON_YAO48_RADIAL_SUBTREE_FILTER_BINARY_RELATIVE}"
readonly MORTON_YAO48_SEED_WORK_PROFILE_BINARY_PATH="${CONTAINER_BUILD}/morsehgp3d-cuda-release/morsehgp3d_gpu_morton_yao48_seed_work_profile"
readonly MODULE_DIR="${CONTAINER_BUILD}/morsehgp3d-cuda-release"
readonly AUDIT_MODULE_DIR="${CONTAINER_BUILD}/morsehgp3d-cuda-audit"
readonly DEFAULT_GUEST_GUARD_MIN_REMAINING_SECONDS=1800
readonly DEFAULT_GUEST_GUARD_MAX_REMAINING_SECONDS=2820
readonly PHASE15_RESIDENT_GUEST_GUARD_MIN_REMAINING_SECONDS=10800
readonly PHASE15_RESIDENT_GUEST_GUARD_MAX_REMAINING_SECONDS=12900
readonly WORK_RESERVE_SECONDS=1800
readonly MAX_GUARDED_SESSION_SECONDS=28800
readonly PHASE15_RESIDENT_SHORT_TIMEOUT_SECONDS=60
readonly PHASE15_RESIDENT_REDUCER_TIMEOUT_SECONDS=300
readonly PHASE15_RESIDENT_FRONTIER_50K_TIMEOUT_SECONDS=600
readonly PHASE15_RESIDENT_DIRECT_10M_TIMEOUT_SECONDS=2400
readonly PHASE15_RESIDENT_DIRECT_30M_TIMEOUT_SECONDS=5400
readonly PHASE15_RESIDENT_FIXED_TIMEOUT_TOTAL_SECONDS=$((
    2 * PHASE15_RESIDENT_SHORT_TIMEOUT_SECONDS +
    2 * PHASE15_RESIDENT_REDUCER_TIMEOUT_SECONDS +
    PHASE15_RESIDENT_FRONTIER_50K_TIMEOUT_SECONDS +
    PHASE15_RESIDENT_DIRECT_10M_TIMEOUT_SECONDS +
    PHASE15_RESIDENT_DIRECT_30M_TIMEOUT_SECONDS
))
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
PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW=""
PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH=""
PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PARENT=""
PHASE9_PAIR_SUPPORT_PHI_OUTPUT_BASE=""
PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW=""
PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH=""
PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PARENT=""
PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_BASE=""
PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW=""
PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH=""
PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PARENT=""
PHASE15_DEVICE_FRONTIER_50K_OUTPUT_BASE=""
PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK=11
PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK_PROVIDED=0
PHASE15_DEVICE_FRONTIER_50K_WARM_PROFILE=0
PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW=""
PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH=""
PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PARENT=""
PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_BASE=""
PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW=""
PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH=""
PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PARENT=""
PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_BASE=""
PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW=""
PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH=""
PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PARENT=""
PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_BASE=""
PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_RAW=""
GUEST_GUARD_MIN_REMAINING_SECONDS="${DEFAULT_GUEST_GUARD_MIN_REMAINING_SECONDS}"
GUEST_GUARD_MAX_REMAINING_SECONDS="${DEFAULT_GUEST_GUARD_MAX_REMAINING_SECONDS}"
PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH=""
PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PARENT=""
PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_BASE=""
MORTON_YAO48_SEED_WORK_PROFILE=0
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
PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP=""
PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP=""
PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP=""
PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP=""
PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP=""
PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP=""
PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP=""
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
Usage : ./gcp-migration/phase3_remote_qualification.sh --yes --gce-deadline-epoch EPOCH --output /CHEMIN/ABSOLU.json [--phase4-spatial-output /CHEMIN/ABSOLU.json] [--phase5-k1-boruvka-output /CHEMIN/ABSOLU.json] [--phase5-k1-boruvka-work-profile-output /CHEMIN/ABSOLU.json] [--morton-yao48-seed-work-profile-output /CHEMIN/ABSOLU.json] [--phase5-k1-boruvka-exact-search-work-profile-output /CHEMIN/ABSOLU.json] [--phase7-h-polytope-output /CHEMIN/ABSOLU.json] [--phase9-pair-support-phi-output /CHEMIN/ABSOLU.json] [--phase15-exact-diametral-phi-output /CHEMIN/ABSOLU.json] [--phase15-device-frontier-50k-output /CHEMIN/ABSOLU.json [--phase15-device-frontier-50k-maximum-closed-rank 6|11] [--phase15-device-frontier-50k-warm-profile]] [--phase15-device-frontier-strict-interior-output /CHEMIN/ABSOLU.json] [--phase15-ranked-pair-classifier-output /CHEMIN/ABSOLU.json] [--phase15-exact-pair-block-witness-cuda-output /CHEMIN/ABSOLU.json] [--phase15-resident-transactional-semantic-output /CHEMIN/ABSOLU.json]

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
L'option pair-support Phi Phase 9 construit et exécute le lot CUDA borné,
audite son ELF AOT sm_120 sans PTX, le passe sous memcheck et racecheck, puis
exige une proposition stricte et une descente aux epochs 1 et 2. La décision
stricte est recertifiée exactement sur CPU; le compagnon reste non scientifique,
sans statut public ni publication d'un produit global de supports.
L'option exact diametral-Phi Phase 15 qualifie seulement le prédicat ponctuel
exact : intervalle dirigé, limbs dyadiques 128/256 et lot BigInt de repli. Elle
audite les builds Release et audit, l'ELF AOT sm_120 sans PTX, memcheck et
racecheck. Le compagnon ne revendique ni catalogue, ni SLO, ni statut public.
L'option device-frontier 50k Phase 15 exécute exclusivement la qualification
standard non-smoke à 50 000 points, rang fermé 11 par défaut ou rang fermé 6
sur demande explicite, tuile 4096, famille adversarial_mixed_dyadic et graine
enregistrée. Toute censure ou masse non résolue échoue fermé; le compagnon
reste un résultat de composant sans catalogue scientifique, exactitude de
hiérarchie, SLO ou statut public.
Avec --phase15-device-frontier-50k-warm-profile, le rang fermé 6 exécute une
passe complète d'échauffement hors chrono puis une passe complète mesurée dans
le même processus. Les contextes de rang sont indépendants et les masses,
le digest et la fermeture doivent rester identiques.
L'option device-frontier strict-interior Phase 15 qualifie séparément le mode
q=3 Gabriel pair-carrier negative-only à deux témoins strictement intérieurs
sur la fixture discriminante A/X, shell égal et intérieur strict. Elle exécute
la matrice native complète, puis le même exécutable sous memcheck et racecheck;
aucun smoke, aucune suppression dans Gamma2, aucun catalogue scientifique,
aucune hiérarchie ou promotion de statut n'en découle.
L'option ranked-pair-classifier Phase 15 qualifie le chemin CUDA natif borné
LBVH, frontière tuilée, classification exacte et catalogue final pour les
rangs fermés 2 et 3 contre l'oracle CPU exact. Elle impose deux capacités de
tuile pour la qualification directe, puis un passage borné séparé sous
memcheck et racecheck. Tout repli exact, désaccord, censure ou sortie non
fermée échoue; ce composant support-2 ne revendique ni Gamma2 complet, ni
hiérarchie K2, ni échelle industrielle, ni SLO, ni statut public.
L'option exact pair-block witness CUDA Phase 15 qualifie le lot natif borné
A×B×W n=128 contre l'oracle exact hôte, puis audite l'ELF AOT sm_120 sans
PTX et répète le binaire sous memcheck et racecheck. Elle s'exécute seule ou
avec le seul profil device-frontier Kmax5 warm; elle ne ferme aucun catalogue
global, produit de supports, arbre, SLO ou statut public.
L'option resident-transactional-semantic Phase 15 exécute exclusivement la
fixture sémantique n=16 pour K=5 puis K=10. Chaque exécution est bornée à
60 secondes et chronométrée depuis l'hôte invité; masses, transitions,
recettes de prune et autorité terminale sont validées strictement. L'ELF doit
être AOT sm_120 sans PTX. Elle enchaîne le full-chain eight_clusters_12 jusqu'à
la forêt H0 conditionnelle et la worklist verticale explicite pour K=5 puis
K=10 (300 s chacun), puis une unique build de la frontière device suivie de
50k adversarial rang 11 (600 s), 10M affine
direct-scale rang 11 (2400 s) et 30M affine direct-scale rang 11 (5400 s). Le
50k doit fermer son contrat; 10M/30M peuvent rester censurés ou incomplets
avec JSON valide et code zéro. Pour ces deux seules unités, le code 124 sans
JSON devient un reçu de timeout canonique non qualifiant seulement si stderr
est strictement vide et si le temps mural externe atteint au moins la borne
fixe, puis l'unité suivante est lancée; tout autre code reste fatal. Ces
profils sont component/profile-only, sans revendication
exact/full-pipeline ou scale_eligible, catalogue, hiérarchie, SLO ni statut
public.
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
        --morton-yao48-seed-work-profile-output)
            (($# >= 2)) || \
                die "Valeur manquante après --morton-yao48-seed-work-profile-output."
            [[ -z "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" ]] || \
                die "Les sorties de profils Morton sont mutuellement exclusives."
            MORTON_YAO48_SEED_WORK_PROFILE=1
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
        --phase9-pair-support-phi-output)
            (($# >= 2)) || \
                die "Valeur manquante après --phase9-pair-support-phi-output."
            [[ -z "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" ]] || \
                die "--phase9-pair-support-phi-output ne peut être fourni qu'une fois."
            PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW="$2"
            shift 2
            ;;
        --phase15-exact-diametral-phi-output)
            (($# >= 2)) || \
                die "Valeur manquante après --phase15-exact-diametral-phi-output."
            [[ -z "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" ]] || \
                die "--phase15-exact-diametral-phi-output ne peut être fourni qu'une fois."
            PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW="$2"
            shift 2
            ;;
        --phase15-device-frontier-50k-output)
            (($# >= 2)) || \
                die "Valeur manquante après --phase15-device-frontier-50k-output."
            [[ -z "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" ]] || \
                die "--phase15-device-frontier-50k-output ne peut être fourni qu'une fois."
            PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW="$2"
            shift 2
            ;;
        --phase15-device-frontier-50k-maximum-closed-rank)
            (($# >= 2)) || \
                die "Valeur manquante après --phase15-device-frontier-50k-maximum-closed-rank."
            ((PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK_PROVIDED == 0)) || \
                die "--phase15-device-frontier-50k-maximum-closed-rank ne peut être fourni qu'une fois."
            case "$2" in
                6|11)
                    PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK="$2"
                    ;;
                *)
                    die "--phase15-device-frontier-50k-maximum-closed-rank doit valoir exactement 6 ou 11."
                    ;;
            esac
            PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK_PROVIDED=1
            shift 2
            ;;
        --phase15-device-frontier-50k-warm-profile)
            ((PHASE15_DEVICE_FRONTIER_50K_WARM_PROFILE == 0)) || \
                die "--phase15-device-frontier-50k-warm-profile ne peut être fourni qu'une fois."
            PHASE15_DEVICE_FRONTIER_50K_WARM_PROFILE=1
            shift
            ;;
        --phase15-device-frontier-strict-interior-output)
            (($# >= 2)) || \
                die "Valeur manquante après --phase15-device-frontier-strict-interior-output."
            [[ -z "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}" ]] || \
                die "--phase15-device-frontier-strict-interior-output ne peut être fourni qu'une fois."
            PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW="$2"
            shift 2
            ;;
        --phase15-ranked-pair-classifier-output)
            (($# >= 2)) || \
                die "Valeur manquante après --phase15-ranked-pair-classifier-output."
            [[ -z "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}" ]] || \
                die "--phase15-ranked-pair-classifier-output ne peut être fourni qu'une fois."
            PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW="$2"
            shift 2
            ;;
        --phase15-exact-pair-block-witness-cuda-output)
            (($# >= 2)) || \
                die "Valeur manquante après --phase15-exact-pair-block-witness-cuda-output."
            [[ -z "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW}" ]] || \
                die "--phase15-exact-pair-block-witness-cuda-output ne peut être fourni qu'une fois."
            PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW="$2"
            shift 2
            ;;
        --phase15-resident-transactional-semantic-output)
            (($# >= 2)) || \
                die "Valeur manquante après --phase15-resident-transactional-semantic-output."
            [[ -z "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_RAW}" ]] || \
                die "--phase15-resident-transactional-semantic-output ne peut être fourni qu'une fois."
            PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_RAW="$2"
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
if ((PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK_PROVIDED == 1)) && \
    [[ -z "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" ]]; then
    die "--phase15-device-frontier-50k-maximum-closed-rank exige --phase15-device-frontier-50k-output."
fi
if ((PHASE15_DEVICE_FRONTIER_50K_WARM_PROFILE == 1)) && \
    [[ -z "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" ]]; then
    die "--phase15-device-frontier-50k-warm-profile exige --phase15-device-frontier-50k-output."
fi
if ((PHASE15_DEVICE_FRONTIER_50K_WARM_PROFILE == 1 && \
    PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK != 6)); then
    die "--phase15-device-frontier-50k-warm-profile exige le rang fermé 6."
fi
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
if [[ -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" ]]; then
    case "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase9-pair-support-phi-output doit être un chemin absolu." ;;
    esac
    [[ "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" != "${OUTPUT_RAW}" && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" != "${PHASE4_OUTPUT_RAW}" && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" != "${PHASE5_OUTPUT_RAW}" && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" != \
            "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" != \
            "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" != \
            "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" ]] || \
        die "La sortie pair-support Phi Phase 9 doit être distincte de toutes les autres sorties."
fi
if [[ -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" ]]; then
    case "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase15-exact-diametral-phi-output doit être un chemin absolu." ;;
    esac
    [[ "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" != "${OUTPUT_RAW}" && \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" != "${PHASE4_OUTPUT_RAW}" && \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" != "${PHASE5_OUTPUT_RAW}" && \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" != \
            "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" && \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" != \
            "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" && \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" != \
            "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" && \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" != \
            "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" ]] || \
        die "La sortie exact diametral-Phi Phase 15 doit être distincte de toutes les autres sorties."
fi
if [[ -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" ]] && \
    [[ -n "${PHASE4_OUTPUT_RAW}" || -n "${PHASE5_OUTPUT_RAW}" || \
       -n "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" || \
       -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" || \
       -n "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" || \
       -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" || \
       -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" || \
       -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}" || \
       -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}" || \
       -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW}" ]]; then
    die "--phase15-exact-diametral-phi-output est exclusive des autres compagnons."
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" ]]; then
    case "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase15-device-frontier-50k-output doit être un chemin absolu." ;;
    esac
    for existing_output in \
        "${OUTPUT_RAW}" "${PHASE4_OUTPUT_RAW}" "${PHASE5_OUTPUT_RAW}" \
        "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" \
        "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}"; do
        [[ -z "${existing_output}" || \
            "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" != \
                "${existing_output}" ]] || \
            die "La sortie device-frontier 50k Phase 15 doit être distincte de toutes les autres sorties."
    done
    if [[ -n "${PHASE4_OUTPUT_RAW}" || -n "${PHASE5_OUTPUT_RAW}" || \
          -n "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" || \
          -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" || \
          -n "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" || \
          -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" || \
          -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" || \
          -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}" || \
          -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}" ]]; then
        die "--phase15-device-frontier-50k-output est exclusive des autres compagnons."
    fi
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}" ]]; then
    case "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase15-device-frontier-strict-interior-output doit être un chemin absolu." ;;
    esac
    [[ "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}" != \
        "${OUTPUT_RAW}" ]] || \
        die "La sortie device-frontier strict-interior Phase 15 doit être distincte de la sortie Phase 3."
    if [[ -n "${PHASE4_OUTPUT_RAW}" || -n "${PHASE5_OUTPUT_RAW}" || \
          -n "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" || \
          -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" || \
          -n "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" || \
          -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" || \
          -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" || \
          -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" || \
          -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}" || \
          -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW}" ]]; then
        die "--phase15-device-frontier-strict-interior-output est exclusive des autres compagnons."
    fi
fi
if [[ -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}" ]]; then
    case "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase15-ranked-pair-classifier-output doit être un chemin absolu." ;;
    esac
    [[ "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}" != "${OUTPUT_RAW}" ]] || \
        die "La sortie ranked-pair-classifier Phase 15 doit être distincte de la sortie Phase 3."
    if [[ -n "${PHASE4_OUTPUT_RAW}" || -n "${PHASE5_OUTPUT_RAW}" || \
          -n "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" || \
          -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" || \
          -n "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" || \
          -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" || \
          -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" || \
          -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" || \
          -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}" || \
          -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW}" ]]; then
        die "--phase15-ranked-pair-classifier-output est exclusive des autres compagnons."
    fi
fi
if [[ -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW}" ]]; then
    case "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase15-exact-pair-block-witness-cuda-output doit être un chemin absolu." ;;
    esac
    for existing_output in \
        "${OUTPUT_RAW}" "${PHASE4_OUTPUT_RAW}" "${PHASE5_OUTPUT_RAW}" \
        "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" \
        "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" \
        "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}"; do
        [[ -z "${existing_output}" || \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW}" != \
                "${existing_output}" ]] || \
            die "La sortie exact pair-block witness CUDA Phase 15 doit être distincte de toutes les autres sorties."
    done
    if [[ -n "${PHASE4_OUTPUT_RAW}" || -n "${PHASE5_OUTPUT_RAW}" || \
          -n "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" || \
          -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" || \
          -n "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" || \
          -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" || \
          -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" || \
          -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}" || \
          -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}" ]]; then
        die "--phase15-exact-pair-block-witness-cuda-output est exclusive des autres compagnons hors warm Kmax5."
    fi
    if [[ -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" ]] && \
       ((PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK != 6 || \
         PHASE15_DEVICE_FRONTIER_50K_WARM_PROFILE != 1)); then
        die "--phase15-exact-pair-block-witness-cuda-output ne peut accompagner que le device-frontier rang fermé 6 warm."
    fi
fi
if [[ -n "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_RAW}" ]]; then
    case "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_RAW}" in
        /*) ;;
        *) die "--phase15-resident-transactional-semantic-output doit être un chemin absolu." ;;
    esac
    [[ "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_RAW}" != \
        "${OUTPUT_RAW}" ]] || \
        die "Les sorties Phase 3 et resident-transactional-semantic doivent être distinctes."
    if [[ -n "${PHASE4_OUTPUT_RAW}" || -n "${PHASE5_OUTPUT_RAW}" || \
          -n "${PHASE5_WORK_PROFILE_OUTPUT_RAW}" || \
          -n "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_RAW}" || \
          -n "${PHASE7_H_POLYTOPE_OUTPUT_RAW}" || \
          -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" || \
          -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" || \
          -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" || \
          -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}" || \
          -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}" || \
          -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW}" ]]; then
        die "--phase15-resident-transactional-semantic-output est exclusive de tous les autres compagnons."
    fi
    GUEST_GUARD_MIN_REMAINING_SECONDS="${PHASE15_RESIDENT_GUEST_GUARD_MIN_REMAINING_SECONDS}"
    GUEST_GUARD_MAX_REMAINING_SECONDS="${PHASE15_RESIDENT_GUEST_GUARD_MAX_REMAINING_SECONDS}"
    ((PHASE15_RESIDENT_FIXED_TIMEOUT_TOTAL_SECONDS < MAX_GUARDED_SESSION_SECONDS)) || \
        die "La somme contractuelle des unités Phase 15 dépasse le garde-fou absolu de huit heures."
fi
readonly GUEST_GUARD_MIN_REMAINING_SECONDS
readonly GUEST_GUARD_MAX_REMAINING_SECONDS
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
if [[ -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}" ]]; then
    PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PARENT="$(dirname -- \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}")"
    PHASE9_PAIR_SUPPORT_PHI_OUTPUT_BASE="$(basename -- \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_RAW}")"
    [[ -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_BASE}" && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_BASE}" != "." && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_BASE}" != ".." ]] || \
        die "Nom d'artefact pair-support Phi Phase 9 invalide."
    [[ -d "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PARENT}" ]] || \
        die "Le répertoire parent de --phase9-pair-support-phi-output doit déjà exister."
    PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PARENT="$(cd -- \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie pair-support Phi Phase 9 illisible."
    PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH="${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PARENT}/${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_BASE}"
    [[ "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PARENT}" == "${OUTPUT_PARENT}" ]] || \
        die "Toutes les sorties doivent partager le même répertoire physique sûr."
    [[ "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" != "${OUTPUT_PATH}" && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" != "${PHASE4_OUTPUT_PATH}" && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" != "${PHASE5_OUTPUT_PATH}" && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" != \
            "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" != \
            "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" && \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" != \
            "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" ]] || \
        die "La sortie pair-support Phi Phase 9 doit être distincte de toutes les autres sorties."
    case "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*)
            die "--phase9-pair-support-phi-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" && \
        ! -L "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" ]] || \
        die "La sortie pair-support Phi Phase 9 doit être inexistante : ${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}."
fi
if [[ -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}" ]]; then
    PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PARENT="$(dirname -- \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}")"
    PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_BASE="$(basename -- \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_RAW}")"
    [[ -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_BASE}" && \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_BASE}" != "." && \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_BASE}" != ".." ]] || \
        die "Nom d'artefact exact diametral-Phi Phase 15 invalide."
    [[ -d "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PARENT}" ]] || \
        die "Le répertoire parent de --phase15-exact-diametral-phi-output doit déjà exister."
    PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PARENT="$(cd -- \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie exact diametral-Phi Phase 15 illisible."
    PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH="${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PARENT}/${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_BASE}"
    [[ "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PARENT}" == "${OUTPUT_PARENT}" ]] || \
        die "Toutes les sorties doivent partager le même répertoire physique sûr."
    for existing_output in \
        "${OUTPUT_PATH}" "${PHASE4_OUTPUT_PATH}" "${PHASE5_OUTPUT_PATH}" \
        "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" \
        "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}"; do
        [[ -z "${existing_output}" || \
            "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}" != "${existing_output}" ]] || \
            die "La sortie exact diametral-Phi Phase 15 doit être distincte de toutes les autres sorties."
    done
    case "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*)
            die "--phase15-exact-diametral-phi-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}" && \
        ! -L "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}" ]] || \
        die "La sortie exact diametral-Phi Phase 15 doit être inexistante : ${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}."
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}" ]]; then
    PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PARENT="$(dirname -- \
        "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}")"
    PHASE15_DEVICE_FRONTIER_50K_OUTPUT_BASE="$(basename -- \
        "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_RAW}")"
    [[ -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_BASE}" && \
        "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_BASE}" != "." && \
        "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_BASE}" != ".." ]] || \
        die "Nom d'artefact device-frontier 50k Phase 15 invalide."
    [[ -d "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PARENT}" ]] || \
        die "Le répertoire parent de --phase15-device-frontier-50k-output doit déjà exister."
    PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PARENT="$(cd -- \
        "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie device-frontier 50k Phase 15 illisible."
    PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH="${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PARENT}/${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_BASE}"
    [[ "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PARENT}" == \
        "${OUTPUT_PARENT}" ]] || \
        die "Toutes les sorties doivent partager le même répertoire physique sûr."
    for existing_output in \
        "${OUTPUT_PATH}" "${PHASE4_OUTPUT_PATH}" "${PHASE5_OUTPUT_PATH}" \
        "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" \
        "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}"; do
        [[ -z "${existing_output}" || \
            "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}" != \
                "${existing_output}" ]] || \
            die "La sortie device-frontier 50k Phase 15 doit être distincte de toutes les autres sorties."
    done
    case "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*)
            die "--phase15-device-frontier-50k-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}" && \
        ! -L "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}" ]] || \
        die "La sortie device-frontier 50k Phase 15 doit être inexistante : ${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}."
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}" ]]; then
    PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PARENT="$(dirname -- \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}")"
    PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_BASE="$(basename -- \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_RAW}")"
    [[ -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_BASE}" && \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_BASE}" != "." && \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_BASE}" != ".." ]] || \
        die "Nom d'artefact device-frontier strict-interior Phase 15 invalide."
    [[ -d "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PARENT}" ]] || \
        die "Le parent de --phase15-device-frontier-strict-interior-output doit déjà exister."
    PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PARENT="$(cd -- \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie device-frontier strict-interior Phase 15 illisible."
    PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH="${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PARENT}/${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_BASE}"
    [[ "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PARENT}" == \
        "${OUTPUT_PARENT}" ]] || \
        die "Toutes les sorties doivent partager le même répertoire physique sûr."
    [[ "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}" != \
        "${OUTPUT_PATH}" ]] || \
        die "Les sorties Phase 3 et device-frontier strict-interior doivent être distinctes."
    case "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*)
            die "--phase15-device-frontier-strict-interior-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}" && \
        ! -L "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}" ]] || \
        die "La sortie device-frontier strict-interior Phase 15 doit être inexistante : ${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}."
fi
if [[ -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}" ]]; then
    PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PARENT="$(dirname -- \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}")"
    PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_BASE="$(basename -- \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_RAW}")"
    [[ -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_BASE}" && \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_BASE}" != "." && \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_BASE}" != ".." ]] || \
        die "Nom d'artefact ranked-pair-classifier Phase 15 invalide."
    [[ -d "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PARENT}" ]] || \
        die "Le parent de --phase15-ranked-pair-classifier-output doit déjà exister."
    PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PARENT="$(cd -- \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie ranked-pair-classifier Phase 15 illisible."
    PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH="${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PARENT}/${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_BASE}"
    [[ "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PARENT}" == \
        "${OUTPUT_PARENT}" ]] || \
        die "Toutes les sorties doivent partager le même répertoire physique sûr."
    [[ "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}" != \
        "${OUTPUT_PATH}" ]] || \
        die "Les sorties Phase 3 et ranked-pair-classifier doivent être distinctes."
    case "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*)
            die "--phase15-ranked-pair-classifier-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}" && \
        ! -L "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}" ]] || \
        die "La sortie ranked-pair-classifier Phase 15 doit être inexistante : ${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}."
fi
if [[ -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW}" ]]; then
    PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PARENT="$(dirname -- \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW}")"
    PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_BASE="$(basename -- \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_RAW}")"
    [[ -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_BASE}" && \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_BASE}" != "." && \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_BASE}" != ".." ]] || \
        die "Nom d'artefact exact pair-block witness CUDA Phase 15 invalide."
    [[ -d "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PARENT}" ]] || \
        die "Le parent de --phase15-exact-pair-block-witness-cuda-output doit déjà exister."
    PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PARENT="$(cd -- \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie exact pair-block witness CUDA Phase 15 illisible."
    PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH="${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PARENT}/${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_BASE}"
    [[ "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PARENT}" == \
        "${OUTPUT_PARENT}" ]] || \
        die "Toutes les sorties doivent partager le même répertoire physique sûr."
    for existing_output in \
        "${OUTPUT_PATH}" "${PHASE4_OUTPUT_PATH}" "${PHASE5_OUTPUT_PATH}" \
        "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" \
        "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" \
        "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}" \
        "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}"; do
        [[ -z "${existing_output}" || \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}" != \
                "${existing_output}" ]] || \
            die "La sortie exact pair-block witness CUDA Phase 15 doit être distincte de toutes les autres sorties."
    done
    case "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*)
            die "--phase15-exact-pair-block-witness-cuda-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}" && \
        ! -L "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}" ]] || \
        die "La sortie exact pair-block witness CUDA Phase 15 doit être inexistante : ${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}."
fi
if [[ -n "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_RAW}" ]]; then
    PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PARENT="$(dirname -- \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_RAW}")"
    PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_BASE="$(basename -- \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_RAW}")"
    [[ -n "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_BASE}" && \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_BASE}" != "." && \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_BASE}" != ".." ]] || \
        die "Nom d'artefact resident-transactional-semantic Phase 15 invalide."
    [[ -d "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PARENT}" ]] || \
        die "Le parent de --phase15-resident-transactional-semantic-output doit déjà exister."
    PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PARENT="$(cd -- \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PARENT}" && pwd -P)" || \
        die "Parent de sortie resident-transactional-semantic Phase 15 illisible."
    PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH="${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PARENT}/${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_BASE}"
    [[ "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PARENT}" == \
        "${OUTPUT_PARENT}" ]] || \
        die "Toutes les sorties doivent partager le même répertoire physique sûr."
    [[ "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}" != \
        "${OUTPUT_PATH}" ]] || \
        die "Les sorties Phase 3 et resident-transactional-semantic doivent être distinctes."
    case "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}/" in
        "${REPOSITORY_ROOT}/"*)
            die "--phase15-resident-transactional-semantic-output doit rester hors du worktree ${REPOSITORY_ROOT}." ;;
    esac
    [[ ! -e "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}" && \
        ! -L "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}" ]] || \
        die "La sortie resident-transactional-semantic Phase 15 doit être inexistante : ${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}."
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
readonly PHASE9_PAIR_SUPPORT_PHI_ASSEMBLER="${REPOSITORY_ROOT}/${PHASE9_PAIR_SUPPORT_PHI_ASSEMBLER_RELATIVE}"
readonly PHASE15_EXACT_DIAMETRAL_PHI_ASSEMBLER="${REPOSITORY_ROOT}/${PHASE15_EXACT_DIAMETRAL_PHI_ASSEMBLER_RELATIVE}"
readonly PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER="${REPOSITORY_ROOT}/${PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER_RELATIVE}"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ASSEMBLER="${REPOSITORY_ROOT}/${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ASSEMBLER_RELATIVE}"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_ASSEMBLER="${REPOSITORY_ROOT}/${PHASE15_RANKED_PAIR_CLASSIFIER_ASSEMBLER_RELATIVE}"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ASSEMBLER="${REPOSITORY_ROOT}/${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ASSEMBLER_RELATIVE}"
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
if [[ -n "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" ]] && \
    ((MORTON_YAO48_SEED_WORK_PROFILE == 0)); then
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
if [[ -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE9_PAIR_SUPPORT_PHI_ASSEMBLER}" && \
        ! -L "${PHASE9_PAIR_SUPPORT_PHI_ASSEMBLER}" ]] || \
        die "Assembleur pair-support Phi Phase 9 absent ou symbolique : ${PHASE9_PAIR_SUPPORT_PHI_ASSEMBLER}."
fi
if [[ -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE15_EXACT_DIAMETRAL_PHI_ASSEMBLER}" && \
        ! -L "${PHASE15_EXACT_DIAMETRAL_PHI_ASSEMBLER}" ]] || \
        die "Assembleur exact diametral-Phi Phase 15 absent ou symbolique : ${PHASE15_EXACT_DIAMETRAL_PHI_ASSEMBLER}."
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER}" && \
        ! -L "${PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER}" ]] || \
        die "Assembleur device-frontier 50k Phase 15 absent ou symbolique : ${PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER}."
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ASSEMBLER}" && \
        ! -L "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ASSEMBLER}" ]] || \
        die "Assembleur device-frontier strict-interior Phase 15 absent ou symbolique : ${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ASSEMBLER}."
fi
if [[ -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE15_RANKED_PAIR_CLASSIFIER_ASSEMBLER}" && \
        ! -L "${PHASE15_RANKED_PAIR_CLASSIFIER_ASSEMBLER}" ]] || \
        die "Assembleur ranked-pair-classifier Phase 15 absent ou symbolique : ${PHASE15_RANKED_PAIR_CLASSIFIER_ASSEMBLER}."
fi
if [[ -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ASSEMBLER}" && \
        ! -L "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ASSEMBLER}" ]] || \
        die "Assembleur exact pair-block witness CUDA Phase 15 absent ou symbolique : ${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ASSEMBLER}."
fi
if [[ -n "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}" ]]; then
    [[ -f "${PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER}" && \
        ! -L "${PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER}" ]] || \
        die "Validateur device-frontier 50k requis par la campagne resident absent ou symbolique : ${PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER}."
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
    if [[ -n "${PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP}" && \
        -f "${PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP}" || true
    fi
    if [[ -n "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}" && \
        -f "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}" || true
    fi
    if [[ -n "${PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP}" && \
        -f "${PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP}" || true
    fi
    if [[ -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP}" && \
        -f "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP}" || true
    fi
    if [[ -n "${PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP}" && \
        -f "${PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP}" || true
    fi
    if [[ -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP}" && \
        -f "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP}" || true
    fi
    if [[ -n "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP}" && \
        -f "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP}" ]]; then
        rm -f -- "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP}" || true
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
if [[ -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" ]]; then
    PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP="$(mktemp \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PARENT}/.${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact temporaire pair-support Phi Phase 9."
    rm -f -- "${PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP}" || \
        die "Impossible de libérer le nom temporaire pair-support Phi Phase 9."
    [[ ! -e "${PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP}" && \
        ! -L "${PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP}" ]] || \
        die "Le nom temporaire pair-support Phi Phase 9 reste occupé."
fi
if [[ -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}" ]]; then
    PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP="$(mktemp \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PARENT}/.${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact temporaire exact diametral-Phi Phase 15."
    rm -f -- "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}" || \
        die "Impossible de libérer le nom temporaire exact diametral-Phi Phase 15."
    [[ ! -e "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}" && \
        ! -L "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}" ]] || \
        die "Le nom temporaire exact diametral-Phi Phase 15 reste occupé."
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}" ]]; then
    PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP="$(mktemp \
        "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PARENT}/.${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact temporaire device-frontier 50k Phase 15."
    rm -f -- "${PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP}" || \
        die "Impossible de libérer le nom temporaire device-frontier 50k Phase 15."
    [[ ! -e "${PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP}" && \
        ! -L "${PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP}" ]] || \
        die "Le nom temporaire device-frontier 50k Phase 15 reste occupé."
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}" ]]; then
    PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP="$(mktemp \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PARENT}/.${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact temporaire device-frontier strict-interior Phase 15."
    rm -f -- "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP}" || \
        die "Impossible de libérer le nom temporaire device-frontier strict-interior Phase 15."
    [[ ! -e "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP}" && \
        ! -L "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP}" ]] || \
        die "Le nom temporaire device-frontier strict-interior Phase 15 reste occupé."
fi
if [[ -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}" ]]; then
    PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP="$(mktemp \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PARENT}/.${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact temporaire ranked-pair-classifier Phase 15."
    rm -f -- "${PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP}" || \
        die "Impossible de libérer le nom temporaire ranked-pair-classifier Phase 15."
    [[ ! -e "${PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP}" && \
        ! -L "${PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP}" ]] || \
        die "Le nom temporaire ranked-pair-classifier Phase 15 reste occupé."
fi
if [[ -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}" ]]; then
    PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP="$(mktemp \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PARENT}/.${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact temporaire exact pair-block witness CUDA Phase 15."
    rm -f -- "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP}" || \
        die "Impossible de libérer le nom temporaire exact pair-block witness CUDA Phase 15."
    [[ ! -e "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP}" && \
        ! -L "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP}" ]] || \
        die "Le nom temporaire exact pair-block witness CUDA Phase 15 reste occupé."
fi
if [[ -n "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}" ]]; then
    PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP="$(mktemp \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PARENT}/.${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_BASE}.XXXXXXXX.partial")" || \
        die "Impossible de réserver l'artefact temporaire resident-transactional-semantic Phase 15."
    rm -f -- "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP}" || \
        die "Impossible de libérer le nom temporaire resident-transactional-semantic Phase 15."
    [[ ! -e "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP}" && \
        ! -L "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP}" ]] || \
        die "Le nom temporaire resident-transactional-semantic Phase 15 reste occupé."
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
readonly MORTON_YAO48_RADIAL_SUBTREE_FILTER_LOG="${LOG_DIR}/morton-yao48-radial-subtree-filter.log"
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
readonly PHASE9_PAIR_SUPPORT_PHI_QUALIFICATION_LOG="${RESULT_DIR}/phase9-pair-support-phi-qualification.json"
readonly PHASE9_PAIR_SUPPORT_PHI_QUALIFICATION_STDERR_LOG="${LOG_DIR}/phase9-pair-support-phi-qualification.stderr.log"
readonly PHASE9_PAIR_SUPPORT_PHI_BUILD_LOG="${LOG_DIR}/phase9-pair-support-phi-build.log"
readonly PHASE9_PAIR_SUPPORT_PHI_ELF_LOG="${LOG_DIR}/phase9-pair-support-phi-cuobjdump-elf.log"
readonly PHASE9_PAIR_SUPPORT_PHI_PTX_LOG="${LOG_DIR}/phase9-pair-support-phi-cuobjdump-ptx.log"
readonly PHASE9_PAIR_SUPPORT_PHI_PTX_STDERR_LOG="${LOG_DIR}/phase9-pair-support-phi-cuobjdump-ptx.stderr.log"
readonly PHASE9_PAIR_SUPPORT_PHI_MEMCHECK_LOG="${LOG_DIR}/phase9-pair-support-phi-memcheck.log"
readonly PHASE9_PAIR_SUPPORT_PHI_RACECHECK_LOG="${LOG_DIR}/phase9-pair-support-phi-racecheck.log"
readonly PHASE15_EXACT_DIAMETRAL_PHI_QUALIFICATION_LOG="${RESULT_DIR}/phase15-exact-diametral-phi-qualification.json"
readonly PHASE15_EXACT_DIAMETRAL_PHI_QUALIFICATION_STDERR_LOG="${LOG_DIR}/phase15-exact-diametral-phi-qualification.stderr.log"
readonly PHASE15_EXACT_DIAMETRAL_PHI_RELEASE_BUILD_LOG="${LOG_DIR}/phase15-exact-diametral-phi-release-build.log"
readonly PHASE15_EXACT_DIAMETRAL_PHI_AUDIT_BUILD_LOG="${LOG_DIR}/phase15-exact-diametral-phi-audit-build.log"
readonly PHASE15_EXACT_DIAMETRAL_PHI_ELF_LOG="${LOG_DIR}/phase15-exact-diametral-phi-cuobjdump-elf.log"
readonly PHASE15_EXACT_DIAMETRAL_PHI_PTX_LOG="${LOG_DIR}/phase15-exact-diametral-phi-cuobjdump-ptx.log"
readonly PHASE15_EXACT_DIAMETRAL_PHI_PTX_STDERR_LOG="${LOG_DIR}/phase15-exact-diametral-phi-cuobjdump-ptx.stderr.log"
readonly PHASE15_EXACT_DIAMETRAL_PHI_MEMCHECK_LOG="${LOG_DIR}/phase15-exact-diametral-phi-memcheck.log"
readonly PHASE15_EXACT_DIAMETRAL_PHI_RACECHECK_LOG="${LOG_DIR}/phase15-exact-diametral-phi-racecheck.log"
readonly PHASE15_DEVICE_FRONTIER_50K_BUILD_LOG="${LOG_DIR}/phase15-device-frontier-50k-build.log"
readonly PHASE15_DEVICE_FRONTIER_50K_QUALIFICATION_LOG="${RESULT_DIR}/phase15-device-frontier-50k-qualification.json"
readonly PHASE15_DEVICE_FRONTIER_50K_STDERR_LOG="${LOG_DIR}/phase15-device-frontier-50k-qualification.stderr.log"
readonly PHASE15_DEVICE_FRONTIER_50K_TIMING_LOG="${RESULT_DIR}/phase15-device-frontier-50k-timing.json"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_LOG="${RESULT_DIR}/phase15-device-frontier-strict-interior-qualification.json"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_STDERR_LOG="${LOG_DIR}/phase15-device-frontier-strict-interior-qualification.stderr.log"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_RELEASE_BUILD_LOG="${LOG_DIR}/phase15-device-frontier-strict-interior-release-build.log"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_AUDIT_BUILD_LOG="${LOG_DIR}/phase15-device-frontier-strict-interior-audit-build.log"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ELF_LOG="${LOG_DIR}/phase15-device-frontier-strict-interior-cuobjdump-elf.log"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PTX_LOG="${LOG_DIR}/phase15-device-frontier-strict-interior-cuobjdump-ptx.log"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PTX_STDERR_LOG="${LOG_DIR}/phase15-device-frontier-strict-interior-cuobjdump-ptx.stderr.log"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_MEMCHECK_LOG="${LOG_DIR}/phase15-device-frontier-strict-interior-memcheck.log"
readonly PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_RACECHECK_LOG="${LOG_DIR}/phase15-device-frontier-strict-interior-racecheck.log"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_LOG="${RESULT_DIR}/phase15-ranked-pair-classifier-qualification.json"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_STDERR_LOG="${LOG_DIR}/phase15-ranked-pair-classifier-qualification.stderr.log"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_RELEASE_BUILD_LOG="${LOG_DIR}/phase15-ranked-pair-classifier-release-build.log"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_AUDIT_BUILD_LOG="${LOG_DIR}/phase15-ranked-pair-classifier-audit-build.log"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_ELF_LOG="${LOG_DIR}/phase15-ranked-pair-classifier-cuobjdump-elf.log"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_PTX_LOG="${LOG_DIR}/phase15-ranked-pair-classifier-cuobjdump-ptx.log"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_PTX_STDERR_LOG="${LOG_DIR}/phase15-ranked-pair-classifier-cuobjdump-ptx.stderr.log"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_MEMCHECK_LOG="${LOG_DIR}/phase15-ranked-pair-classifier-memcheck.log"
readonly PHASE15_RANKED_PAIR_CLASSIFIER_RACECHECK_LOG="${LOG_DIR}/phase15-ranked-pair-classifier-racecheck.log"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_QUALIFICATION_LOG="${RESULT_DIR}/phase15-exact-pair-block-witness-cuda-qualification.json"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_QUALIFICATION_STDERR_LOG="${LOG_DIR}/phase15-exact-pair-block-witness-cuda-qualification.stderr.log"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_RELEASE_BUILD_LOG="${LOG_DIR}/phase15-exact-pair-block-witness-cuda-release-build.log"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_AUDIT_BUILD_LOG="${LOG_DIR}/phase15-exact-pair-block-witness-cuda-audit-build.log"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ELF_LOG="${LOG_DIR}/phase15-exact-pair-block-witness-cuda-cuobjdump-elf.log"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PTX_LOG="${LOG_DIR}/phase15-exact-pair-block-witness-cuda-cuobjdump-ptx.log"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PTX_STDERR_LOG="${LOG_DIR}/phase15-exact-pair-block-witness-cuda-cuobjdump-ptx.stderr.log"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_MEMCHECK_LOG="${LOG_DIR}/phase15-exact-pair-block-witness-cuda-memcheck.log"
readonly PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_RACECHECK_LOG="${LOG_DIR}/phase15-exact-pair-block-witness-cuda-racecheck.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_RELEASE_BUILD_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-release-build.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_AUDIT_BUILD_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-audit-build.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_LOG="${RESULT_DIR}/phase15-resident-transactional-semantic-k5.json"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_STDERR_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-k5.stderr.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_LOG="${RESULT_DIR}/phase15-resident-transactional-semantic-k10.json"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_STDERR_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-k10.stderr.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_ELF_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-cuobjdump-elf.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PTX_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-cuobjdump-ptx.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PTX_STDERR_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-cuobjdump-ptx.stderr.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_BUILD_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-reducer-build.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K5_LOG="${RESULT_DIR}/phase15-resident-transactional-semantic-reducer-k5.json"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K5_STDERR_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-reducer-k5.stderr.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K10_LOG="${RESULT_DIR}/phase15-resident-transactional-semantic-reducer-k10.json"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K10_STDERR_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-reducer-k10.stderr.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_FRONTIER_BUILD_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-frontier-build.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_FRONTIER_50K_LOG="${RESULT_DIR}/phase15-resident-transactional-semantic-frontier-50k.json"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_FRONTIER_50K_STDERR_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-frontier-50k.stderr.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_10M_LOG="${RESULT_DIR}/phase15-resident-transactional-semantic-direct-10m.json"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_10M_STDERR_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-direct-10m.stderr.log"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_30M_LOG="${RESULT_DIR}/phase15-resident-transactional-semantic-direct-30m.json"
readonly PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_30M_STDERR_LOG="${LOG_DIR}/phase15-resident-transactional-semantic-direct-30m.stderr.log"
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

run_until_work_deadline_split_output() {
    local label="$1"
    local stdout_path="$2"
    local stderr_path="$3"
    local now=0
    local remaining=0
    local soft_timeout=0
    shift 3

    [[ "${stdout_path}" != "${stderr_path}" ]] || {
        printf '[ERREUR] stdout et stderr partagent le même chemin pour %s.\n' \
            "${label}" >&2
        return 125
    }
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
        "${soft_timeout}s" "$@" >"${stdout_path}" 2>"${stderr_path}"
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
        2>>"${DOCKER_LOG}")"; then
        return 125
    fi
    [[ -z "${collision}" ]] || \
        die "Collision de nom Docker refusée avant run : ${container_name}."
    ACTIVE_CONTAINER_CIDFILE="${cidfile}"
    ACTIVE_CONTAINER_LOG="${DOCKER_LOG}"
    ACTIVE_CONTAINER_NAME="${container_name}"
    if run_until_work_deadline_split_output \
        "${label}" "${stdout_path}" "${stderr_path}" \
        "${DOCKER[@]}" run \
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
        "${IMAGE_REF}" "$@"; then
        run_status=0
    else
        run_status=$?
    fi
    if remove_container_from_cidfile \
        "${cidfile}" "${container_name}" "${DOCKER_LOG}"; then
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
[[ -f "${BUILD_DIR}/${MORTON_YAO48_RADIAL_SUBTREE_FILTER_BINARY_RELATIVE#build/}" && \
    ! -L "${BUILD_DIR}/${MORTON_YAO48_RADIAL_SUBTREE_FILTER_BINARY_RELATIVE#build/}" && \
    -x "${BUILD_DIR}/${MORTON_YAO48_RADIAL_SUBTREE_FILTER_BINARY_RELATIVE#build/}" ]] || \
    die "Le binaire radial Morton/Yao48 n'a pas été produit par cuda-release."
begin_unit "morton-yao48-radial-subtree-filter"
if ! run_container "morton-yao48-radial-subtree-filter" \
    "${MORTON_YAO48_RADIAL_SUBTREE_FILTER_LOG}" \
    "${MORTON_YAO48_RADIAL_SUBTREE_FILTER_BINARY_PATH}"; then
    report_failure_log "morton-yao48-radial-subtree-filter" \
        "${MORTON_YAO48_RADIAL_SUBTREE_FILTER_LOG}"
    die "La qualification radiale Morton/Yao48 a échoué; voir ${MORTON_YAO48_RADIAL_SUBTREE_FILTER_LOG}."
fi
[[ "$(grep -Fc 'Morton/Yao48 CUDA radial-subtree qualification passed' \
    "${MORTON_YAO48_RADIAL_SUBTREE_FILTER_LOG}" || true)" == "1" ]] || \
    die "Le journal radial Morton/Yao48 ne contient pas son unique témoin de succès."
printf '[QUALIFICATION] Filtre radial Morton/Yao48 CUDA exécuté avec succès.\n'
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

if [[ -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" ]]; then
    begin_unit "phase9-pair-support-phi-build"
    if ! run_container "phase9-pair-support-phi-build" \
        "${PHASE9_PAIR_SUPPORT_PHI_BUILD_LOG}" \
        cmake --build "${MODULE_DIR}" \
        --target morsehgp3d_gpu_pair_support_phi_qualification \
        --parallel 4; then
        report_failure_log "phase9-pair-support-phi-build" \
            "${PHASE9_PAIR_SUPPORT_PHI_BUILD_LOG}"
        die "La cible CUDA pair-support Phi Phase 9 n'a pas pu être construite."
    fi
    [[ -f "${BUILD_DIR}/${PHASE9_PAIR_SUPPORT_PHI_BINARY_RELATIVE#build/}" && \
        ! -L "${BUILD_DIR}/${PHASE9_PAIR_SUPPORT_PHI_BINARY_RELATIVE#build/}" && \
        -x "${BUILD_DIR}/${PHASE9_PAIR_SUPPORT_PHI_BINARY_RELATIVE#build/}" ]] || \
        die "Le binaire CUDA pair-support Phi Phase 9 n'a pas été construit sûrement."

    begin_unit "phase9-pair-support-phi-qualification"
    if ! run_container_split_output "phase9-pair-support-phi-qualification" \
        "${PHASE9_PAIR_SUPPORT_PHI_QUALIFICATION_LOG}" \
        "${PHASE9_PAIR_SUPPORT_PHI_QUALIFICATION_STDERR_LOG}" \
        "${PHASE9_PAIR_SUPPORT_PHI_BINARY_PATH}"; then
        report_failure_log "phase9-pair-support-phi-qualification" \
            "${PHASE9_PAIR_SUPPORT_PHI_QUALIFICATION_LOG}"
        report_failure_log "phase9-pair-support-phi-qualification-stderr" \
            "${PHASE9_PAIR_SUPPORT_PHI_QUALIFICATION_STDERR_LOG}"
        die "La qualification CUDA pair-support Phi Phase 9 a échoué."
    fi
    if ! python3 -B - "${PHASE9_PAIR_SUPPORT_PHI_QUALIFICATION_LOG}" <<'PY'
import json
from pathlib import Path
import sys

raw = Path(sys.argv[1]).read_text(encoding="utf-8")
lines = raw.splitlines()
if len(lines) != 1 or not lines[0]:
    raise SystemExit("qualification stdout must contain exactly one JSON line")
value = json.loads(lines[0])
expected_keys = {
    "backend", "certified_receipt_count", "decision_semantics", "descend_count",
    "deterministic", "first_epoch", "global_support_product_prune_published",
    "mode", "node_count", "phase", "profile", "proposal_digest_fnv1a",
    "proposal_semantics", "public_status", "schema", "second_epoch",
    "strict_interior_count",
}
if not isinstance(value, dict) or set(value) != expected_keys:
    raise SystemExit("qualification stdout JSON does not respect its closed schema")
for key, expected in {
    "backend": "cuda_g4",
    "certified_receipt_count": 1,
    "descend_count": 1,
    "deterministic": True,
    "first_epoch": 1,
    "global_support_product_prune_published": False,
    "mode": "certified",
    "node_count": 5,
    "phase": "9",
    "profile": "hgp_reduced",
    "public_status": None,
    "schema": "morsehgp3d.phase9.pair_support_phi_cuda_qualification.v1",
    "second_epoch": 2,
    "strict_interior_count": 1,
}.items():
    if value.get(key) != expected:
        raise SystemExit(f"qualification field {key} differs")
canonical = json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":")) + "\n"
if raw != canonical:
    raise SystemExit("qualification stdout must be canonical JSON")
PY
    then
        report_failure_log "phase9-pair-support-phi-qualification" \
            "${PHASE9_PAIR_SUPPORT_PHI_QUALIFICATION_LOG}"
        die "La sortie pair-support Phi Phase 9 ne ferme pas les comptes strict=1, descend=1, epochs=1/2."
    fi

    begin_unit "phase9-pair-support-phi-cuobjdump-elf"
    if ! run_container "phase9-pair-support-phi-cuobjdump-elf" \
        "${PHASE9_PAIR_SUPPORT_PHI_ELF_LOG}" /usr/local/cuda/bin/cuobjdump \
        -lelf "${PHASE9_PAIR_SUPPORT_PHI_BINARY_PATH}"; then
        report_failure_log "phase9-pair-support-phi-cuobjdump-elf" \
            "${PHASE9_PAIR_SUPPORT_PHI_ELF_LOG}"
        die "cuobjdump n'a pas pu lister les ELF pair-support Phi Phase 9."
    fi
    phase9_pair_support_phi_architectures="$(grep -Eo 'sm_[0-9]+' \
        "${PHASE9_PAIR_SUPPORT_PHI_ELF_LOG}" | sort -u || true)"
    if [[ "${phase9_pair_support_phi_architectures}" != "sm_120" ]]; then
        report_failure_log "phase9-pair-support-phi-cuobjdump-elf" \
            "${PHASE9_PAIR_SUPPORT_PHI_ELF_LOG}"
        die "Le binaire pair-support Phi Phase 9 doit contenir uniquement un ELF sm_120; observé : ${phase9_pair_support_phi_architectures:-aucun}."
    fi

    begin_unit "phase9-pair-support-phi-cuobjdump-ptx"
    if ! run_container_split_output "phase9-pair-support-phi-cuobjdump-ptx" \
        "${PHASE9_PAIR_SUPPORT_PHI_PTX_LOG}" \
        "${PHASE9_PAIR_SUPPORT_PHI_PTX_STDERR_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lptx \
        "${PHASE9_PAIR_SUPPORT_PHI_BINARY_PATH}"; then
        report_failure_log "phase9-pair-support-phi-cuobjdump-ptx-stderr" \
            "${PHASE9_PAIR_SUPPORT_PHI_PTX_STDERR_LOG}"
        die "cuobjdump n'a pas pu auditer le PTX pair-support Phi Phase 9."
    fi
    if grep -q '[^[:space:]]' "${PHASE9_PAIR_SUPPORT_PHI_PTX_LOG}"; then
        report_failure_log "phase9-pair-support-phi-cuobjdump-ptx" \
            "${PHASE9_PAIR_SUPPORT_PHI_PTX_LOG}"
        die "Une entrée PTX a été détectée dans le binaire pair-support Phi Phase 9."
    fi

    begin_unit "phase9-pair-support-phi-memcheck"
    if ! run_container "phase9-pair-support-phi-memcheck" \
        "${PHASE9_PAIR_SUPPORT_PHI_MEMCHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool memcheck \
        --leak-check full \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE9_PAIR_SUPPORT_PHI_BINARY_PATH}"; then
        report_failure_log "phase9-pair-support-phi-memcheck" \
            "${PHASE9_PAIR_SUPPORT_PHI_MEMCHECK_LOG}"
        die "Le memcheck pair-support Phi Phase 9 a échoué."
    fi

    begin_unit "phase9-pair-support-phi-racecheck"
    if ! run_container "phase9-pair-support-phi-racecheck" \
        "${PHASE9_PAIR_SUPPORT_PHI_RACECHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool racecheck \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE9_PAIR_SUPPORT_PHI_BINARY_PATH}"; then
        report_failure_log "phase9-pair-support-phi-racecheck" \
            "${PHASE9_PAIR_SUPPORT_PHI_RACECHECK_LOG}"
        die "Le racecheck pair-support Phi Phase 9 a échoué."
    fi
fi

if [[ -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}" ]]; then
    begin_unit "phase15-exact-diametral-phi-release-build"
    if ! run_container "phase15-exact-diametral-phi-release-build" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_RELEASE_BUILD_LOG}" \
        cmake --build "${MODULE_DIR}" \
        --target morsehgp3d_gpu_exact_diametral_phi_qualification \
        --parallel 4; then
        report_failure_log "phase15-exact-diametral-phi-release-build" \
            "${PHASE15_EXACT_DIAMETRAL_PHI_RELEASE_BUILD_LOG}"
        die "La cible CUDA exact diametral-Phi Phase 15 n'a pas pu être construite en Release."
    fi

    begin_unit "phase15-exact-diametral-phi-audit-build"
    if ! run_container "phase15-exact-diametral-phi-audit-build" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_AUDIT_BUILD_LOG}" \
        cmake --build "${AUDIT_MODULE_DIR}" \
        --target morsehgp3d_gpu_exact_diametral_phi_qualification \
        --parallel 4; then
        report_failure_log "phase15-exact-diametral-phi-audit-build" \
            "${PHASE15_EXACT_DIAMETRAL_PHI_AUDIT_BUILD_LOG}"
        die "La cible CUDA exact diametral-Phi Phase 15 n'a pas pu être construite sous le preset audit."
    fi
    [[ -f "${BUILD_DIR}/${PHASE15_EXACT_DIAMETRAL_PHI_BINARY_RELATIVE#build/}" && \
        ! -L "${BUILD_DIR}/${PHASE15_EXACT_DIAMETRAL_PHI_BINARY_RELATIVE#build/}" && \
        -x "${BUILD_DIR}/${PHASE15_EXACT_DIAMETRAL_PHI_BINARY_RELATIVE#build/}" ]] || \
        die "Le binaire CUDA exact diametral-Phi Phase 15 n'a pas été construit sûrement."

    begin_unit "phase15-exact-diametral-phi-qualification"
    if ! run_container_split_output "phase15-exact-diametral-phi-qualification" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_QUALIFICATION_LOG}" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_QUALIFICATION_STDERR_LOG}" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_BINARY_PATH}"; then
        report_failure_log "phase15-exact-diametral-phi-qualification" \
            "${PHASE15_EXACT_DIAMETRAL_PHI_QUALIFICATION_LOG}"
        report_failure_log "phase15-exact-diametral-phi-qualification-stderr" \
            "${PHASE15_EXACT_DIAMETRAL_PHI_QUALIFICATION_STDERR_LOG}"
        die "La qualification CUDA exact diametral-Phi Phase 15 a échoué."
    fi
    if ! python3 -B - "${PHASE15_EXACT_DIAMETRAL_PHI_QUALIFICATION_LOG}" \
        "$(dirname -- "${PHASE15_EXACT_DIAMETRAL_PHI_ASSEMBLER}")" <<'PY'
import json
from pathlib import Path
import sys

path = Path(sys.argv[1])
sys.path.insert(0, sys.argv[2])
import assemble_phase15_exact_diametral_phi_qualification as assembler

raw = path.read_text(encoding="utf-8")
lines = raw.splitlines()
if len(lines) != 1 or not lines[0]:
    raise SystemExit("qualification stdout must contain exactly one JSON line")
value = json.loads(lines[0])
assembler.validate_qualification(value)
canonical = json.dumps(
    value, ensure_ascii=False, sort_keys=True, separators=(",", ":")
) + "\n"
if raw != canonical:
    raise SystemExit("qualification stdout must be canonical JSON")
PY
    then
        report_failure_log "phase15-exact-diametral-phi-qualification" \
            "${PHASE15_EXACT_DIAMETRAL_PHI_QUALIFICATION_LOG}"
        die "La sortie exact diametral-Phi Phase 15 ne ferme pas son schéma exact component-only."
    fi

    begin_unit "phase15-exact-diametral-phi-cuobjdump-elf"
    if ! run_container "phase15-exact-diametral-phi-cuobjdump-elf" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_ELF_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lelf \
        "${PHASE15_EXACT_DIAMETRAL_PHI_BINARY_PATH}"; then
        report_failure_log "phase15-exact-diametral-phi-cuobjdump-elf" \
            "${PHASE15_EXACT_DIAMETRAL_PHI_ELF_LOG}"
        die "cuobjdump n'a pas pu lister les ELF exact diametral-Phi Phase 15."
    fi
    phase15_exact_diametral_phi_architectures="$(grep -Eo 'sm_[0-9]+' \
        "${PHASE15_EXACT_DIAMETRAL_PHI_ELF_LOG}" | sort -u || true)"
    if [[ "${phase15_exact_diametral_phi_architectures}" != "sm_120" ]]; then
        report_failure_log "phase15-exact-diametral-phi-cuobjdump-elf" \
            "${PHASE15_EXACT_DIAMETRAL_PHI_ELF_LOG}"
        die "Le binaire exact diametral-Phi Phase 15 doit contenir uniquement un ELF sm_120; observé : ${phase15_exact_diametral_phi_architectures:-aucun}."
    fi

    begin_unit "phase15-exact-diametral-phi-cuobjdump-ptx"
    if ! run_container_split_output "phase15-exact-diametral-phi-cuobjdump-ptx" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_PTX_LOG}" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_PTX_STDERR_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lptx \
        "${PHASE15_EXACT_DIAMETRAL_PHI_BINARY_PATH}"; then
        report_failure_log "phase15-exact-diametral-phi-cuobjdump-ptx-stderr" \
            "${PHASE15_EXACT_DIAMETRAL_PHI_PTX_STDERR_LOG}"
        die "cuobjdump n'a pas pu auditer le PTX exact diametral-Phi Phase 15."
    fi
    if grep -q '[^[:space:]]' "${PHASE15_EXACT_DIAMETRAL_PHI_PTX_LOG}"; then
        report_failure_log "phase15-exact-diametral-phi-cuobjdump-ptx" \
            "${PHASE15_EXACT_DIAMETRAL_PHI_PTX_LOG}"
        die "Une entrée PTX a été détectée dans le binaire exact diametral-Phi Phase 15."
    fi

    begin_unit "phase15-exact-diametral-phi-memcheck"
    if ! run_container "phase15-exact-diametral-phi-memcheck" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_MEMCHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool memcheck \
        --leak-check full \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE15_EXACT_DIAMETRAL_PHI_BINARY_PATH}"; then
        report_failure_log "phase15-exact-diametral-phi-memcheck" \
            "${PHASE15_EXACT_DIAMETRAL_PHI_MEMCHECK_LOG}"
        die "Le memcheck exact diametral-Phi Phase 15 a échoué."
    fi

    begin_unit "phase15-exact-diametral-phi-racecheck"
    if ! run_container "phase15-exact-diametral-phi-racecheck" \
        "${PHASE15_EXACT_DIAMETRAL_PHI_RACECHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool racecheck \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE15_EXACT_DIAMETRAL_PHI_BINARY_PATH}"; then
        report_failure_log "phase15-exact-diametral-phi-racecheck" \
            "${PHASE15_EXACT_DIAMETRAL_PHI_RACECHECK_LOG}"
        die "Le racecheck exact diametral-Phi Phase 15 a échoué."
    fi
fi

if [[ -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}" ]]; then
    declare -a phase15_device_frontier_50k_warm_command_option=()
    if ((PHASE15_DEVICE_FRONTIER_50K_WARM_PROFILE == 1)); then
        phase15_device_frontier_50k_warm_command_option=(
            --warmup-run-count 1
        )
    fi
    begin_unit "phase15-device-frontier-50k-build"
    if ! run_container "phase15-device-frontier-50k-build" \
        "${PHASE15_DEVICE_FRONTIER_50K_BUILD_LOG}" \
        cmake --build "${MODULE_DIR}" \
        --target \
            morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification \
        --parallel 8; then
        report_failure_log "phase15-device-frontier-50k-build" \
            "${PHASE15_DEVICE_FRONTIER_50K_BUILD_LOG}"
        die "La construction ciblée device-frontier 50k Phase 15 a échoué."
    fi
    [[ -f "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_50K_BINARY_RELATIVE#build/}" && \
        ! -L "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_50K_BINARY_RELATIVE#build/}" && \
        -x "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_50K_BINARY_RELATIVE#build/}" ]] || \
        die "Le binaire CUDA device-frontier 50k Phase 15 n'a pas été construit sûrement."

    begin_unit "phase15-device-frontier-50k-qualification"
    phase15_device_frontier_50k_started_ns="$("${DATE_BIN}" +%s%N)" || \
        die "Horloge nanoseconde illisible avant la qualification device-frontier 50k."
    [[ "${phase15_device_frontier_50k_started_ns}" =~ ^[0-9]{19}$ ]] || \
        die "Horodatage initial device-frontier 50k invalide."
    phase15_device_frontier_50k_status=0
    if run_container_split_output "phase15-device-frontier-50k-qualification" \
        "${PHASE15_DEVICE_FRONTIER_50K_QUALIFICATION_LOG}" \
        "${PHASE15_DEVICE_FRONTIER_50K_STDERR_LOG}" \
        "${PHASE15_DEVICE_FRONTIER_50K_BINARY_PATH}" \
        --point-count 50000 \
        --family adversarial_mixed_dyadic \
        --maximum-closed-rank \
            "${PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK}" \
        --anchor-tile-capacity 4096 \
        --seed "${PHASE15_DEVICE_FRONTIER_50K_SEED}" \
        "${phase15_device_frontier_50k_warm_command_option[@]}"; then
        phase15_device_frontier_50k_status=0
    else
        phase15_device_frontier_50k_status=$?
    fi
    phase15_device_frontier_50k_finished_ns="$("${DATE_BIN}" +%s%N)" || \
        die "Horloge nanoseconde illisible après la qualification device-frontier 50k."
    [[ "${phase15_device_frontier_50k_finished_ns}" =~ ^[0-9]{19}$ ]] || \
        die "Horodatage final device-frontier 50k invalide."
    python3 -B - \
        "${PHASE15_DEVICE_FRONTIER_50K_TIMING_LOG}" \
        "${phase15_device_frontier_50k_started_ns}" \
        "${phase15_device_frontier_50k_finished_ns}" \
        "${phase15_device_frontier_50k_status}" \
        "${PHASE15_DEVICE_FRONTIER_50K_BINARY_PATH}" \
        "${PHASE15_DEVICE_FRONTIER_50K_SEED}" \
        "${PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK}" \
        "${PHASE15_DEVICE_FRONTIER_50K_WARM_PROFILE}" <<'PY'
import json
from pathlib import Path
import sys

path = Path(sys.argv[1])
started = int(sys.argv[2])
finished = int(sys.argv[3])
status = int(sys.argv[4])
binary = sys.argv[5]
seed = sys.argv[6]
maximum_closed_rank = sys.argv[7]
warm_profile = sys.argv[8] == "1"
if finished <= started:
    raise SystemExit("qualification device-frontier 50k sans intervalle positif")
record = {
    "command": [
        binary,
        "--point-count", "50000",
        "--family", "adversarial_mixed_dyadic",
        "--maximum-closed-rank", maximum_closed_rank,
        "--anchor-tile-capacity", "4096",
        "--seed", seed,
    ],
    "elapsed_wall_ns": finished - started,
    "exit_status": status,
    "finished_epoch_ns": finished,
    "schema": "morsehgp3d.phase15.device_frontier_50k_timing.v1",
    "started_epoch_ns": started,
}
if warm_profile:
    record["command"].extend(["--warmup-run-count", "1"])
path.write_text(
    json.dumps(record, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
    + "\n",
    encoding="utf-8",
)
PY
    if ((phase15_device_frontier_50k_status != 0)); then
        report_failure_log "phase15-device-frontier-50k-qualification" \
            "${PHASE15_DEVICE_FRONTIER_50K_QUALIFICATION_LOG}"
        report_failure_log "phase15-device-frontier-50k-qualification-stderr" \
            "${PHASE15_DEVICE_FRONTIER_50K_STDERR_LOG}"
        die "La qualification standard non-smoke device-frontier 50k Phase 15 a échoué."
    fi
    if ! python3 -B - \
        "${PHASE15_DEVICE_FRONTIER_50K_QUALIFICATION_LOG}" \
        "${PHASE15_DEVICE_FRONTIER_50K_STDERR_LOG}" \
        "${PHASE15_DEVICE_FRONTIER_50K_TIMING_LOG}" \
        "$(dirname -- "${PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER}")" \
        "${HEAD_SHA}" \
        "${PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK}" \
        "${PHASE15_DEVICE_FRONTIER_50K_WARM_PROFILE}" <<'PY'
from pathlib import Path
import sys

qualification_path = Path(sys.argv[1])
stderr_path = Path(sys.argv[2])
timing_path = Path(sys.argv[3])
sys.path.insert(0, sys.argv[4])
git_sha = sys.argv[5]
maximum_closed_rank = int(sys.argv[6])
warm_profile = sys.argv[7] == "1"

import assemble_phase15_device_frontier_50k_qualification as assembler

qualification_raw = qualification_path.read_text(encoding="utf-8")
qualification = assembler.parse_single_line_json(
    qualification_raw, "qualification device-frontier 50k"
)
assembler.validate_qualification(
    qualification,
    git_sha=git_sha,
    maximum_closed_rank=maximum_closed_rank,
    warm_profile=warm_profile,
)
if stderr_path.read_text(encoding="utf-8") != "":
    raise SystemExit("stderr non vide pour la qualification device-frontier 50k")
timing_raw = timing_path.read_text(encoding="utf-8")
timing = assembler.parse_single_line_json(
    timing_raw, "temps device-frontier 50k"
)
assembler.validate_timing(
    timing,
    maximum_closed_rank=maximum_closed_rank,
    warm_profile=warm_profile,
)
PY
    then
        report_failure_log "phase15-device-frontier-50k-qualification" \
            "${PHASE15_DEVICE_FRONTIER_50K_QUALIFICATION_LOG}"
        report_failure_log "phase15-device-frontier-50k-qualification-stderr" \
            "${PHASE15_DEVICE_FRONTIER_50K_STDERR_LOG}"
        die "Le résultat device-frontier 50k Phase 15 a échoué à sa validation fermée."
    fi
fi

if [[ -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}" ]]; then
    begin_unit "phase15-device-frontier-strict-interior-release-build"
    if ! run_container "phase15-device-frontier-strict-interior-release-build" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_RELEASE_BUILD_LOG}" \
        cmake --build "${MODULE_DIR}" \
        --target \
            morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification \
        --parallel 8; then
        report_failure_log "phase15-device-frontier-strict-interior-release-build" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_RELEASE_BUILD_LOG}"
        die "La cible CUDA device-frontier strict-interior Phase 15 n'a pas pu être construite en Release."
    fi

    begin_unit "phase15-device-frontier-strict-interior-audit-build"
    if ! run_container "phase15-device-frontier-strict-interior-audit-build" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_AUDIT_BUILD_LOG}" \
        cmake --build "${AUDIT_MODULE_DIR}" \
        --target \
            morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification \
        --parallel 8; then
        report_failure_log "phase15-device-frontier-strict-interior-audit-build" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_AUDIT_BUILD_LOG}"
        die "La cible CUDA device-frontier strict-interior Phase 15 n'a pas pu être construite sous le preset audit."
    fi
    [[ -f "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_RELATIVE#build/}" && \
        ! -L "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_RELATIVE#build/}" && \
        -x "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_RELATIVE#build/}" ]] || \
        die "Le binaire CUDA device-frontier strict-interior Phase 15 n'a pas été construit sûrement."

    begin_unit "phase15-device-frontier-strict-interior-qualification"
    if ! run_container_split_output \
        "phase15-device-frontier-strict-interior-qualification" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_LOG}" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_STDERR_LOG}" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_PATH}" \
        --prune-semantics strict_interior_threshold \
        --scientific-scope q3_gabriel_exact_diametral_pair_support_negative_only \
        --fixture q3_shell_strict_discriminant; then
        report_failure_log "phase15-device-frontier-strict-interior-qualification" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_LOG}"
        report_failure_log "phase15-device-frontier-strict-interior-qualification-stderr" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_STDERR_LOG}"
        die "La qualification CUDA device-frontier strict-interior Phase 15 a échoué."
    fi
    if ! python3 -B - \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_LOG}" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_STDERR_LOG}" \
        "$(dirname -- "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ASSEMBLER}")" \
        "${HEAD_SHA}" <<'PY'
from pathlib import Path
import sys

qualification_path = Path(sys.argv[1])
stderr_path = Path(sys.argv[2])
sys.path.insert(0, sys.argv[3])
git_sha = sys.argv[4]

import assemble_phase15_device_frontier_strict_interior_qualification as assembler

raw = qualification_path.read_text(encoding="utf-8")
value = assembler.parse_single_json_line(
    raw, "device-frontier strict-interior qualification"
)
assembler.validate_qualification(value, git_sha=git_sha)
assembler.validate_qualification_infrastructure_stderr(
    stderr_path.read_text(encoding="utf-8")
)
PY
    then
        report_failure_log "phase15-device-frontier-strict-interior-qualification" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_LOG}"
        report_failure_log "phase15-device-frontier-strict-interior-qualification-stderr" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_STDERR_LOG}"
        die "La sortie device-frontier strict-interior Phase 15 ne ferme pas son contrat q=3 borné."
    fi

    begin_unit "phase15-device-frontier-strict-interior-cuobjdump-elf"
    if ! run_container "phase15-device-frontier-strict-interior-cuobjdump-elf" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ELF_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lelf \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_PATH}"; then
        report_failure_log "phase15-device-frontier-strict-interior-cuobjdump-elf" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ELF_LOG}"
        die "cuobjdump n'a pas pu lister les ELF device-frontier strict-interior Phase 15."
    fi
    phase15_device_frontier_strict_interior_architectures="$(grep -Eo 'sm_[0-9]+' \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ELF_LOG}" | sort -u || true)"
    if [[ "${phase15_device_frontier_strict_interior_architectures}" != "sm_120" ]]; then
        report_failure_log "phase15-device-frontier-strict-interior-cuobjdump-elf" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ELF_LOG}"
        die "Le binaire device-frontier strict-interior Phase 15 doit contenir uniquement un ELF sm_120; observé : ${phase15_device_frontier_strict_interior_architectures:-aucun}."
    fi

    begin_unit "phase15-device-frontier-strict-interior-cuobjdump-ptx"
    if ! run_container_split_output \
        "phase15-device-frontier-strict-interior-cuobjdump-ptx" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PTX_LOG}" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PTX_STDERR_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lptx \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_PATH}"; then
        report_failure_log "phase15-device-frontier-strict-interior-cuobjdump-ptx-stderr" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PTX_STDERR_LOG}"
        die "cuobjdump n'a pas pu auditer le PTX device-frontier strict-interior Phase 15."
    fi
    if grep -q '[^[:space:]]' "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PTX_LOG}"; then
        report_failure_log "phase15-device-frontier-strict-interior-cuobjdump-ptx" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PTX_LOG}"
        die "Une entrée PTX a été détectée dans le binaire device-frontier strict-interior Phase 15."
    fi

    begin_unit "phase15-device-frontier-strict-interior-memcheck"
    if ! run_container "phase15-device-frontier-strict-interior-memcheck" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_MEMCHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool memcheck \
        --leak-check full \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_PATH}" \
        --prune-semantics strict_interior_threshold \
        --scientific-scope q3_gabriel_exact_diametral_pair_support_negative_only \
        --fixture q3_shell_strict_discriminant; then
        report_failure_log "phase15-device-frontier-strict-interior-memcheck" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_MEMCHECK_LOG}"
        die "Le memcheck device-frontier strict-interior Phase 15 a échoué."
    fi

    begin_unit "phase15-device-frontier-strict-interior-racecheck"
    if ! run_container "phase15-device-frontier-strict-interior-racecheck" \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_RACECHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool racecheck \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_PATH}" \
        --prune-semantics strict_interior_threshold \
        --scientific-scope q3_gabriel_exact_diametral_pair_support_negative_only \
        --fixture q3_shell_strict_discriminant; then
        report_failure_log "phase15-device-frontier-strict-interior-racecheck" \
            "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_RACECHECK_LOG}"
        die "Le racecheck device-frontier strict-interior Phase 15 a échoué."
    fi
fi

if [[ -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}" ]]; then
    begin_unit "phase15-ranked-pair-classifier-release-build"
    if ! run_container "phase15-ranked-pair-classifier-release-build" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_RELEASE_BUILD_LOG}" \
        cmake --build "${MODULE_DIR}" \
        --target \
            morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_qualification \
        --parallel 8; then
        report_failure_log "phase15-ranked-pair-classifier-release-build" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_RELEASE_BUILD_LOG}"
        die "La cible CUDA ranked-pair-classifier Phase 15 n'a pas pu être construite en Release."
    fi

    begin_unit "phase15-ranked-pair-classifier-audit-build"
    if ! run_container "phase15-ranked-pair-classifier-audit-build" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_AUDIT_BUILD_LOG}" \
        cmake --build "${AUDIT_MODULE_DIR}" \
        --target \
            morsehgp3d_gpu_morton_yao48_ranked_pair_tile_classifier_qualification \
        --parallel 8; then
        report_failure_log "phase15-ranked-pair-classifier-audit-build" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_AUDIT_BUILD_LOG}"
        die "La cible CUDA ranked-pair-classifier Phase 15 n'a pas pu être construite sous le preset audit."
    fi
    [[ -f "${BUILD_DIR}/${PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_RELATIVE#build/}" && \
        ! -L "${BUILD_DIR}/${PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_RELATIVE#build/}" && \
        -x "${BUILD_DIR}/${PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_RELATIVE#build/}" ]] || \
        die "Le binaire CUDA ranked-pair-classifier Phase 15 n'a pas été construit sûrement."

    begin_unit "phase15-ranked-pair-classifier-qualification"
    if ! run_container_split_output "phase15-ranked-pair-classifier-qualification" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_LOG}" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_STDERR_LOG}" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_PATH}"; then
        report_failure_log "phase15-ranked-pair-classifier-qualification" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_LOG}"
        report_failure_log "phase15-ranked-pair-classifier-qualification-stderr" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_STDERR_LOG}"
        die "La qualification CUDA ranked-pair-classifier Phase 15 a échoué."
    fi
    if ! python3 -B - \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_LOG}" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_STDERR_LOG}" \
        "$(dirname -- "${PHASE15_RANKED_PAIR_CLASSIFIER_ASSEMBLER}")" \
        "${HEAD_SHA}" <<'PY'
from pathlib import Path
import sys

qualification_path = Path(sys.argv[1])
stderr_path = Path(sys.argv[2])
sys.path.insert(0, sys.argv[3])
git_sha = sys.argv[4]

import assemble_phase15_ranked_pair_classifier_qualification as assembler

raw = qualification_path.read_text(encoding="utf-8")
value = assembler.parse_single_json_line(raw, "rank-2/3 qualification")
assembler.validate_qualification(value, git_sha=git_sha, single_run=False)
assembler.validate_qualification_infrastructure_stderr(
    stderr_path.read_text(encoding="utf-8")
)
PY
    then
        report_failure_log "phase15-ranked-pair-classifier-qualification" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_LOG}"
        report_failure_log "phase15-ranked-pair-classifier-qualification-stderr" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_STDERR_LOG}"
        die "La sortie ranked-pair-classifier Phase 15 ne ferme pas son contrat exact support-2 rangs 2 et 3."
    fi

    begin_unit "phase15-ranked-pair-classifier-cuobjdump-elf"
    if ! run_container "phase15-ranked-pair-classifier-cuobjdump-elf" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_ELF_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lelf \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_PATH}"; then
        report_failure_log "phase15-ranked-pair-classifier-cuobjdump-elf" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_ELF_LOG}"
        die "cuobjdump n'a pas pu lister les ELF ranked-pair-classifier Phase 15."
    fi
    phase15_ranked_pair_classifier_architectures="$(grep -Eo 'sm_[0-9]+' \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_ELF_LOG}" | sort -u || true)"
    if [[ "${phase15_ranked_pair_classifier_architectures}" != "sm_120" ]]; then
        report_failure_log "phase15-ranked-pair-classifier-cuobjdump-elf" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_ELF_LOG}"
        die "Le binaire ranked-pair-classifier Phase 15 doit contenir uniquement un ELF sm_120; observé : ${phase15_ranked_pair_classifier_architectures:-aucun}."
    fi

    begin_unit "phase15-ranked-pair-classifier-cuobjdump-ptx"
    if ! run_container_split_output "phase15-ranked-pair-classifier-cuobjdump-ptx" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_PTX_LOG}" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_PTX_STDERR_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lptx \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_PATH}"; then
        report_failure_log "phase15-ranked-pair-classifier-cuobjdump-ptx-stderr" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_PTX_STDERR_LOG}"
        die "cuobjdump n'a pas pu auditer le PTX ranked-pair-classifier Phase 15."
    fi
    if grep -q '[^[:space:]]' "${PHASE15_RANKED_PAIR_CLASSIFIER_PTX_LOG}"; then
        report_failure_log "phase15-ranked-pair-classifier-cuobjdump-ptx" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_PTX_LOG}"
        die "Une entrée PTX a été détectée dans le binaire ranked-pair-classifier Phase 15."
    fi

    begin_unit "phase15-ranked-pair-classifier-memcheck"
    if ! run_container "phase15-ranked-pair-classifier-memcheck" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_MEMCHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool memcheck \
        --leak-check full \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_PATH}" \
        --single-run; then
        report_failure_log "phase15-ranked-pair-classifier-memcheck" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_MEMCHECK_LOG}"
        die "Le memcheck ranked-pair-classifier Phase 15 a échoué."
    fi

    begin_unit "phase15-ranked-pair-classifier-racecheck"
    if ! run_container "phase15-ranked-pair-classifier-racecheck" \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_RACECHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer \
        --target-processes all \
        --tool racecheck \
        --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_PATH}" \
        --single-run; then
        report_failure_log "phase15-ranked-pair-classifier-racecheck" \
            "${PHASE15_RANKED_PAIR_CLASSIFIER_RACECHECK_LOG}"
        die "Le racecheck ranked-pair-classifier Phase 15 a échoué."
    fi
fi

if [[ -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}" ]]; then
    begin_unit "phase15-exact-pair-block-witness-cuda-release-build"
    if ! run_container "phase15-exact-pair-block-witness-cuda-release-build" \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_RELEASE_BUILD_LOG}" \
        cmake --build "${MODULE_DIR}" \
        --target morsehgp3d_gpu_exact_pair_block_witness_cuda_qualification \
        --parallel 2; then
        report_failure_log "phase15-exact-pair-block-witness-cuda-release-build" \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_RELEASE_BUILD_LOG}"
        die "La cible exact pair-block witness CUDA Phase 15 n'a pas pu être construite en Release."
    fi
    begin_unit "phase15-exact-pair-block-witness-cuda-audit-build"
    if ! run_container "phase15-exact-pair-block-witness-cuda-audit-build" \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_AUDIT_BUILD_LOG}" \
        cmake --build "${AUDIT_MODULE_DIR}" \
        --target morsehgp3d_gpu_exact_pair_block_witness_cuda_qualification \
        --parallel 2; then
        report_failure_log "phase15-exact-pair-block-witness-cuda-audit-build" \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_AUDIT_BUILD_LOG}"
        die "La cible exact pair-block witness CUDA Phase 15 n'a pas pu être construite sous le preset audit."
    fi
    [[ -f "${BUILD_DIR}/${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_RELATIVE#build/}" && \
        ! -L "${BUILD_DIR}/${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_RELATIVE#build/}" && \
        -x "${BUILD_DIR}/${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_RELATIVE#build/}" ]] || \
        die "Le binaire exact pair-block witness CUDA Phase 15 n'a pas été construit sûrement."

    begin_unit "phase15-exact-pair-block-witness-cuda-qualification"
    if ! run_container_split_output \
        "phase15-exact-pair-block-witness-cuda-qualification" \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_QUALIFICATION_LOG}" \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_QUALIFICATION_STDERR_LOG}" \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_PATH}"; then
        report_failure_log "phase15-exact-pair-block-witness-cuda-qualification" \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_QUALIFICATION_LOG}"
        report_failure_log "phase15-exact-pair-block-witness-cuda-qualification-stderr" \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_QUALIFICATION_STDERR_LOG}"
        die "La qualification différentielle exact pair-block witness CUDA Phase 15 a échoué."
    fi
    if ! python3 -B - \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_QUALIFICATION_LOG}" \
        "$(dirname -- "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ASSEMBLER}")" <<'PY'
import json
from pathlib import Path
import sys

path = Path(sys.argv[1])
sys.path.insert(0, sys.argv[2])
import assemble_phase15_exact_pair_block_witness_cuda_qualification as assembler

raw = path.read_text(encoding="utf-8")
lines = raw.splitlines()
if len(lines) != 1 or not lines[0]:
    raise SystemExit("qualification stdout must contain exactly one JSON line")
value = json.loads(lines[0])
assembler.validate_qualification(value)
if raw != json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":")) + "\n":
    raise SystemExit("qualification stdout must be canonical JSON")
PY
    then
        report_failure_log "phase15-exact-pair-block-witness-cuda-qualification" \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_QUALIFICATION_LOG}"
        die "La sortie différentielle exact pair-block witness CUDA Phase 15 est incomplète ou non canonique."
    fi

    begin_unit "phase15-exact-pair-block-witness-cuda-cuobjdump-elf"
    if ! run_container "phase15-exact-pair-block-witness-cuda-cuobjdump-elf" \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ELF_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lelf \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_PATH}"; then
        report_failure_log "phase15-exact-pair-block-witness-cuda-cuobjdump-elf" \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ELF_LOG}"
        die "cuobjdump n'a pas pu lister l'ELF exact pair-block witness CUDA Phase 15."
    fi
    phase15_exact_pair_block_witness_cuda_architectures="$(grep -Eo 'sm_[0-9]+' \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ELF_LOG}" | sort -u || true)"
    [[ "${phase15_exact_pair_block_witness_cuda_architectures}" == "sm_120" ]] || \
        die "Le binaire exact pair-block witness CUDA Phase 15 doit contenir uniquement un ELF sm_120; observé : ${phase15_exact_pair_block_witness_cuda_architectures:-aucun}."

    begin_unit "phase15-exact-pair-block-witness-cuda-cuobjdump-ptx"
    if ! run_container_split_output \
        "phase15-exact-pair-block-witness-cuda-cuobjdump-ptx" \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PTX_LOG}" \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PTX_STDERR_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lptx \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_PATH}"; then
        report_failure_log "phase15-exact-pair-block-witness-cuda-cuobjdump-ptx-stderr" \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PTX_STDERR_LOG}"
        die "cuobjdump n'a pas pu auditer le PTX exact pair-block witness CUDA Phase 15."
    fi
    if grep -q '[^[:space:]]' "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PTX_LOG}"; then
        report_failure_log "phase15-exact-pair-block-witness-cuda-cuobjdump-ptx" \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PTX_LOG}"
        die "Une entrée PTX a été détectée dans le binaire exact pair-block witness CUDA Phase 15."
    fi

    begin_unit "phase15-exact-pair-block-witness-cuda-memcheck"
    if ! run_container "phase15-exact-pair-block-witness-cuda-memcheck" \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_MEMCHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer --target-processes all \
        --tool memcheck --leak-check full --report-api-errors no \
        --error-exitcode=86 \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_PATH}"; then
        report_failure_log "phase15-exact-pair-block-witness-cuda-memcheck" \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_MEMCHECK_LOG}"
        die "Le memcheck exact pair-block witness CUDA Phase 15 a échoué."
    fi
    begin_unit "phase15-exact-pair-block-witness-cuda-racecheck"
    if ! run_container "phase15-exact-pair-block-witness-cuda-racecheck" \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_RACECHECK_LOG}" \
        /usr/local/cuda/bin/compute-sanitizer --target-processes all \
        --tool racecheck --report-api-errors no --error-exitcode=86 \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_PATH}"; then
        report_failure_log "phase15-exact-pair-block-witness-cuda-racecheck" \
            "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_RACECHECK_LOG}"
        die "Le racecheck exact pair-block witness CUDA Phase 15 a échoué."
    fi
fi

if [[ -n "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}" ]]; then
    begin_unit "phase15-resident-transactional-semantic-release-build"
    if ! run_container "phase15-resident-transactional-semantic-release-build" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_RELEASE_BUILD_LOG}" \
        cmake --build "${MODULE_DIR}" \
        --target morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_qualification \
        --parallel 2; then
        report_failure_log "phase15-resident-transactional-semantic-release-build" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_RELEASE_BUILD_LOG}"
        die "La cible resident-transactional-semantic Phase 15 n'a pas pu être construite en Release."
    fi
    begin_unit "phase15-resident-transactional-semantic-audit-build"
    if ! run_container "phase15-resident-transactional-semantic-audit-build" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_AUDIT_BUILD_LOG}" \
        cmake --build "${AUDIT_MODULE_DIR}" \
        --target morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_qualification \
        --parallel 2; then
        report_failure_log "phase15-resident-transactional-semantic-audit-build" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_AUDIT_BUILD_LOG}"
        die "La cible resident-transactional-semantic Phase 15 n'a pas pu être construite sous le preset audit."
    fi
    [[ -f "${BUILD_DIR}/${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_BINARY_RELATIVE#build/}" && \
        ! -L "${BUILD_DIR}/${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_BINARY_RELATIVE#build/}" && \
        -x "${BUILD_DIR}/${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_BINARY_RELATIVE#build/}" ]] || \
        die "Le binaire resident-transactional-semantic Phase 15 n'a pas été construit sûrement."

    validate_phase15_resident_transactional_semantic_run() {
        local qualification_path="$1"
        local expected_maximum_order="$2"
        python3 -B - "${qualification_path}" "${expected_maximum_order}" <<'PY'
import json
from pathlib import Path
import sys


def fail(message):
    raise SystemExit(message)


def reject_duplicates(pairs):
    value = {}
    for key, item in pairs:
        if key in value:
            fail(f"duplicate JSON key: {key}")
        value[key] = item
    return value


def exact_object(value, keys, context):
    if not isinstance(value, dict) or set(value) != set(keys):
        fail(f"{context}: unexpected object shape")
    return value


def integer(value, context, minimum=0):
    if type(value) is not int or value < minimum:
        fail(f"{context}: invalid integer")
    return value


path = Path(sys.argv[1])
maximum_order = int(sys.argv[2])
raw = path.read_text(encoding="utf-8")
lines = raw.splitlines()
if len(lines) != 1 or not lines[0] or raw != lines[0] + "\n":
    fail("qualification stdout must contain exactly one newline-terminated JSON object")
value = json.loads(lines[0], object_pairs_hook=reject_duplicates)
exact_object(
    value,
    {
        "schema", "backend", "profile", "mode", "public_status",
        "point_count", "maximum_order", "fixture", "maximum_closed_rank",
        "require_complete", "status", "qualified_runtime_contract",
        "runtime_smoke_passed", "rollback_contract_passed",
        "semantic_recipe_contract_passed", "qualified",
        "global_pair_coverage_closed", "terminal_authority_complete",
        "capacity_wave_rollback", "mass", "counts", "cuda",
        "timings_nanoseconds", "claims", "qualified_scope",
    },
    "qualification",
)
expected_scalars = {
    "schema": "morsehgp3d.phase15.resident_transactional_frontier_qualification.v1",
    "backend": "cuda_g4",
    "profile": "hgp_reduced",
    "mode": "resident_transactional_pair_partition",
    "public_status": "not_claimed",
    "point_count": 16,
    "maximum_order": maximum_order,
    "fixture": "semantic_line_16",
    "maximum_closed_rank": maximum_order + 1,
    "require_complete": True,
    "status": "qualified_cuda_terminal_authority",
    "qualified_runtime_contract": True,
    "runtime_smoke_passed": True,
    "rollback_contract_passed": False,
    "semantic_recipe_contract_passed": True,
    "qualified": True,
    "global_pair_coverage_closed": True,
    "terminal_authority_complete": True,
    "capacity_wave_rollback": False,
    "qualified_scope": "pair_block_partition_scheduler_only",
}
for key, expected in expected_scalars.items():
    if value.get(key) != expected or type(value.get(key)) is not type(expected):
        fail(f"qualification.{key}: unexpected value")
mass = exact_object(value.get("mass"), {"universe", "pending", "inflight", "pruned", "terminal"}, "mass")
if mass != {"universe": 120, "pending": 0, "inflight": 0, "pruned": 4, "terminal": 116}:
    fail("mass: semantic fixture partition mismatch")
counts = exact_object(
    value.get("counts"),
    {
        "waves", "commits", "rollbacks", "pending_blocks", "terminal_pairs",
        "prune_receipts", "submitted_prune_recipes", "matched_prune_recipes",
        "exact_prune_attempts", "certified_prunes", "fail_open_prunes",
    },
    "counts",
)
for key in counts:
    integer(counts[key], f"counts.{key}")
if counts["waves"] < 1 or counts["commits"] != counts["waves"]:
    fail("counts: every non-empty wave must commit")
expected_counts = {
    "rollbacks": 0, "pending_blocks": 0, "terminal_pairs": 116,
    "prune_receipts": 1, "submitted_prune_recipes": 2,
    "matched_prune_recipes": 2, "exact_prune_attempts": 2,
    "certified_prunes": 1, "fail_open_prunes": 1,
}
for key, expected in expected_counts.items():
    if counts[key] != expected:
        fail(f"counts.{key}: semantic fixture mismatch")
cuda = exact_object(
    value.get("cuda"),
    {
        "kernel_launches", "synchronizations", "intermediate_control_readbacks",
        "kernel_elapsed_nanoseconds", "device_arena_bytes",
        "serial_device_reference", "scale_eligible",
    },
    "cuda",
)
for key, expected in {
    "kernel_launches": 1,
    "synchronizations": 3,
    "intermediate_control_readbacks": 0,
    "serial_device_reference": True,
    "scale_eligible": False,
}.items():
    if cuda.get(key) != expected or type(cuda.get(key)) is not type(expected):
        fail(f"cuda.{key}: unexpected value")
integer(cuda.get("kernel_elapsed_nanoseconds"), "cuda.kernel_elapsed_nanoseconds", 1)
integer(cuda.get("device_arena_bytes"), "cuda.device_arena_bytes", 1)
timings = exact_object(
    value.get("timings_nanoseconds"),
    {"generation", "canonicalization", "lbvh_build", "scheduler_wall", "total"},
    "timings_nanoseconds",
)
for key in timings:
    integer(timings[key], f"timings_nanoseconds.{key}")
if timings["total"] < sum(timings[key] for key in ("generation", "canonicalization", "lbvh_build", "scheduler_wall")):
    fail("timings_nanoseconds.total: smaller than measured components")
if value.get("claims") != {
    "pair_catalog_complete": False,
    "supports_3_4": False,
    "hierarchy_or_tree": False,
    "min_cluster_size_applied": False,
    "slo": False,
}:
    fail("claims: unexpected scientific claim")
PY
    }

    begin_unit "phase15-resident-transactional-semantic-k5"
    phase15_resident_transactional_semantic_k5_started_ns="$("${DATE_BIN}" +%s%N)" || \
        die "Horloge murale illisible avant resident-transactional-semantic K=5."
    if run_container_split_output \
        "phase15-resident-transactional-semantic-k5" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_STDERR_LOG}" \
        /usr/bin/timeout --kill-after=5s --foreground 60s \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_BINARY_PATH}" \
        --point-count 16 --K 5 --semantic-line-fixture --require-complete; then
        phase15_resident_transactional_semantic_k5_status=0
    else
        phase15_resident_transactional_semantic_k5_status=$?
    fi
    phase15_resident_transactional_semantic_k5_finished_ns="$("${DATE_BIN}" +%s%N)" || \
        die "Horloge murale illisible après resident-transactional-semantic K=5."
    if ((phase15_resident_transactional_semantic_k5_status != 0)); then
        report_failure_log "phase15-resident-transactional-semantic-k5" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_LOG}"
        report_failure_log "phase15-resident-transactional-semantic-k5-stderr" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_STDERR_LOG}"
        die "La qualification resident-transactional-semantic K=5 a échoué ou dépassé 60 secondes."
    fi
    [[ "${phase15_resident_transactional_semantic_k5_started_ns}" =~ ^[0-9]{19}$ && \
        "${phase15_resident_transactional_semantic_k5_finished_ns}" =~ ^[0-9]{19}$ && \
        phase15_resident_transactional_semantic_k5_finished_ns -gt \
            phase15_resident_transactional_semantic_k5_started_ns ]] || \
        die "Mesure murale externe K=5 invalide."
    if grep -q '[^[:space:]]' "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_STDERR_LOG}" || \
       ! validate_phase15_resident_transactional_semantic_run \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_LOG}" 5; then
        report_failure_log "phase15-resident-transactional-semantic-k5" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_LOG}"
        report_failure_log "phase15-resident-transactional-semantic-k5-stderr" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_STDERR_LOG}"
        die "La sortie resident-transactional-semantic K=5 viole le contrat JSON strict."
    fi

    begin_unit "phase15-resident-transactional-semantic-k10"
    phase15_resident_transactional_semantic_k10_started_ns="$("${DATE_BIN}" +%s%N)" || \
        die "Horloge murale illisible avant resident-transactional-semantic K=10."
    if run_container_split_output \
        "phase15-resident-transactional-semantic-k10" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_STDERR_LOG}" \
        /usr/bin/timeout --kill-after=5s --foreground 60s \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_BINARY_PATH}" \
        --point-count 16 --K 10 --semantic-line-fixture --require-complete; then
        phase15_resident_transactional_semantic_k10_status=0
    else
        phase15_resident_transactional_semantic_k10_status=$?
    fi
    phase15_resident_transactional_semantic_k10_finished_ns="$("${DATE_BIN}" +%s%N)" || \
        die "Horloge murale illisible après resident-transactional-semantic K=10."
    if ((phase15_resident_transactional_semantic_k10_status != 0)); then
        report_failure_log "phase15-resident-transactional-semantic-k10" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_LOG}"
        report_failure_log "phase15-resident-transactional-semantic-k10-stderr" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_STDERR_LOG}"
        die "La qualification resident-transactional-semantic K=10 a échoué ou dépassé 60 secondes."
    fi
    [[ "${phase15_resident_transactional_semantic_k10_started_ns}" =~ ^[0-9]{19}$ && \
        "${phase15_resident_transactional_semantic_k10_finished_ns}" =~ ^[0-9]{19}$ && \
        phase15_resident_transactional_semantic_k10_finished_ns -gt \
            phase15_resident_transactional_semantic_k10_started_ns ]] || \
        die "Mesure murale externe K=10 invalide."
    if grep -q '[^[:space:]]' "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_STDERR_LOG}" || \
       ! validate_phase15_resident_transactional_semantic_run \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_LOG}" 10; then
        report_failure_log "phase15-resident-transactional-semantic-k10" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_LOG}"
        report_failure_log "phase15-resident-transactional-semantic-k10-stderr" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_STDERR_LOG}"
        die "La sortie resident-transactional-semantic K=10 viole le contrat JSON strict."
    fi

    begin_unit "phase15-resident-transactional-semantic-cuobjdump-elf"
    if ! run_container "phase15-resident-transactional-semantic-cuobjdump-elf" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_ELF_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lelf \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_BINARY_PATH}"; then
        report_failure_log "phase15-resident-transactional-semantic-cuobjdump-elf" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_ELF_LOG}"
        die "cuobjdump n'a pas pu lister l'ELF resident-transactional-semantic Phase 15."
    fi
    phase15_resident_transactional_semantic_architectures="$(grep -Eo 'sm_[0-9]+' \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_ELF_LOG}" | sort -u || true)"
    [[ "${phase15_resident_transactional_semantic_architectures}" == "sm_120" ]] || \
        die "Le binaire resident-transactional-semantic Phase 15 doit contenir uniquement un ELF sm_120; observé : ${phase15_resident_transactional_semantic_architectures:-aucun}."

    begin_unit "phase15-resident-transactional-semantic-cuobjdump-ptx"
    if ! run_container_split_output \
        "phase15-resident-transactional-semantic-cuobjdump-ptx" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PTX_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PTX_STDERR_LOG}" \
        /usr/local/cuda/bin/cuobjdump -lptx \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_BINARY_PATH}"; then
        report_failure_log "phase15-resident-transactional-semantic-cuobjdump-ptx-stderr" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PTX_STDERR_LOG}"
        die "cuobjdump n'a pas pu auditer le PTX resident-transactional-semantic Phase 15."
    fi
    if grep -q '[^[:space:]]' "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PTX_LOG}"; then
        report_failure_log "phase15-resident-transactional-semantic-cuobjdump-ptx" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PTX_LOG}"
        die "Une entrée PTX a été détectée dans le binaire resident-transactional-semantic Phase 15."
    fi

    run_phase15_resident_campaign_unit() {
        local label="$1"
        local fixed_timeout_seconds="$2"
        local stdout_path="$3"
        local stderr_path="$4"
        local exit_policy="$5"
        local run_status=0
        shift 5
        [[ "${fixed_timeout_seconds}" =~ ^[1-9][0-9]*$ ]] || \
            die "Borne fixe de campagne resident non canonique : ${fixed_timeout_seconds}."
        ((fixed_timeout_seconds == PHASE15_RESIDENT_SHORT_TIMEOUT_SECONDS || \
          fixed_timeout_seconds == PHASE15_RESIDENT_REDUCER_TIMEOUT_SECONDS || \
          fixed_timeout_seconds == PHASE15_RESIDENT_FRONTIER_50K_TIMEOUT_SECONDS || \
          fixed_timeout_seconds == PHASE15_RESIDENT_DIRECT_10M_TIMEOUT_SECONDS || \
          fixed_timeout_seconds == PHASE15_RESIDENT_DIRECT_30M_TIMEOUT_SECONDS)) || \
            die "Borne fixe de campagne resident invalide : ${fixed_timeout_seconds}."
        case "${exit_policy}" in
            strict|canonical-direct-scale-timeout) ;;
            *) die "Politique de sortie de campagne resident invalide : ${exit_policy}." ;;
        esac
        begin_unit "${label}"
        PHASE15_RESIDENT_CAMPAIGN_LAST_STARTED_NS="$("${DATE_BIN}" +%s%N)" || \
            die "Horloge murale illisible avant ${label}."
        if run_container_split_output \
            "${label}" "${stdout_path}" "${stderr_path}" \
            /usr/bin/timeout --kill-after=5s --foreground \
            "${fixed_timeout_seconds}s" "$@"; then
            run_status=0
        else
            run_status=$?
        fi
        PHASE15_RESIDENT_CAMPAIGN_LAST_FINISHED_NS="$("${DATE_BIN}" +%s%N)" || \
            die "Horloge murale illisible après ${label}."
        PHASE15_RESIDENT_CAMPAIGN_LAST_EXIT_STATUS="${run_status}"
        if ((run_status != 0)); then
            if [[ "${exit_policy}" != "canonical-direct-scale-timeout" ]] || \
               ((run_status != 124)); then
                report_failure_log "${label}" "${stdout_path}"
                report_failure_log "${label}-stderr" "${stderr_path}"
                die "L'unité ${label} a échoué ou dépassé ${fixed_timeout_seconds} secondes."
            fi
        fi
        [[ "${PHASE15_RESIDENT_CAMPAIGN_LAST_STARTED_NS}" =~ ^[0-9]{19}$ && \
            "${PHASE15_RESIDENT_CAMPAIGN_LAST_FINISHED_NS}" =~ ^[0-9]{19}$ && \
            PHASE15_RESIDENT_CAMPAIGN_LAST_FINISHED_NS -gt \
                PHASE15_RESIDENT_CAMPAIGN_LAST_STARTED_NS ]] || \
            die "Mesure murale externe invalide pour ${label}."
        if ((run_status == 124 && \
            PHASE15_RESIDENT_CAMPAIGN_LAST_FINISHED_NS - \
                PHASE15_RESIDENT_CAMPAIGN_LAST_STARTED_NS < \
                fixed_timeout_seconds * 1000000000)); then
            report_failure_log "${label}" "${stdout_path}"
            report_failure_log "${label}-stderr" "${stderr_path}"
            die "Le code 124 de ${label} est antérieur à la borne fixe de ${fixed_timeout_seconds} secondes."
        fi
        if [[ -s "${stderr_path}" ]]; then
            report_failure_log "${label}-stderr" "${stderr_path}"
            die "L'unité ${label} doit conserver stderr vide."
        fi
        if ((run_status == 124)); then
            printf '[TIMEOUT CANONIQUE] unité=%s, borne=%ss, résultat non qualifiant; poursuite autorisée.\n' \
                "${label}" "${fixed_timeout_seconds}"
        fi
    }

    validate_phase15_reducer_source_run() {
        local result_path="$1"
        local expected_maximum_order="$2"
        python3 -B - "${result_path}" "${expected_maximum_order}" "${HEAD_SHA}" <<'PY'
import json
from pathlib import Path
import sys


def fail(message):
    raise SystemExit(message)


def reject_duplicates(pairs):
    value = {}
    for key, item in pairs:
        if key in value:
            fail(f"duplicate reducer JSON key: {key}")
        value[key] = item
    return value


def exact(value, keys, context):
    if not isinstance(value, dict) or set(value) != set(keys):
        fail(f"{context}: unexpected object shape")
    return value


def integer(value, context, minimum=0):
    if type(value) is not int or value < minimum:
        fail(f"{context}: invalid integer")
    return value


path = Path(sys.argv[1])
maximum_order = int(sys.argv[2])
git_sha = sys.argv[3]
raw = path.read_text(encoding="utf-8")
lines = raw.splitlines()
if len(lines) != 1 or raw != lines[0] + "\n":
    fail("reducer stdout must contain one newline-terminated JSON object")
value = json.loads(lines[0], object_pairs_hook=reject_duplicates)
exact(
    value,
    {
        "schema", "backend", "git_sha", "profile", "mode", "public_status", "fixture",
        "point_count", "maximum_order", "maximum_closed_rank",
        "require_complete", "qualified",
        "recipe_catalog_certified",
        "cut_certified", "pair_authority_certified",
        "shared_product_traversal_certified", "higher_adapter_certified",
        "higher_terminal", "higher_authority_certified",
        "bridge_certified", "provider_replay_certified",
        "forest_reduction_certified", "vertical_target_pipeline_certified",
        "vertical_journal_certified",
        "shared_product_traversal", "higher_support_positive_adapter",
        "automatic_recipe_catalog", "pair_cut", "pair_classification", "reducer_source",
        "forest_reduction", "vertical_target_pipeline", "vertical_journal", "digests",
        "timings_nanoseconds", "claims", "qualified_scope",
    },
    "reducer qualification",
)
for key, expected in {
    "schema": "morsehgp3d.phase15.transactional_pair_to_conditional_forest_qualification.v5",
    "backend": "cuda_g4_plus_reference_cpu",
    "git_sha": git_sha,
    "profile": "hgp_reduced",
    "mode": "automatic_exact_prune_recipes_to_complete_direct_terminal_source_to_conditional_forest_to_forest_relative_multiorder_vertical_targets_and_zero_missing_label_vertical_journal",
    "public_status": "not_claimed",
    "fixture": "eight_clusters_12",
    "point_count": 12,
    "maximum_order": maximum_order,
    "maximum_closed_rank": maximum_order + 1,
    "require_complete": True,
    "qualified": True,
    "recipe_catalog_certified": True,
    "cut_certified": True,
    "pair_authority_certified": True,
    "shared_product_traversal_certified": True,
    "higher_adapter_certified": True,
    "higher_terminal": True,
    "higher_authority_certified": True,
    "bridge_certified": True,
    "provider_replay_certified": True,
    "forest_reduction_certified": True,
    "vertical_target_pipeline_certified": True,
    "vertical_journal_certified": True,
    "qualified_scope": "automatic_exact_prune_cut_to_terminal_direct_supports_bounded_conditional_h0_forest_forest_relative_multiorder_vertical_target_proposals_and_zero_missing_label_conditional_vertical_journal_only",
}.items():
    if value.get(key) != expected or type(value.get(key)) is not type(expected):
        fail(f"reducer qualification.{key}: unexpected value")
shared_traversal = exact(
    value.get("shared_product_traversal"),
    {
        "retained_snapshots", "capacity_accountings", "snapshot_copies",
        "source_lease_consumptions", "pair_views", "higher_support_views",
        "pair_lifetime_completed", "higher_support_lifetime_completed",
        "source_snapshot_epoch",
    },
    "shared_product_traversal",
)
for key in {
    "retained_snapshots", "capacity_accountings", "snapshot_copies",
    "source_lease_consumptions", "pair_views", "higher_support_views",
    "source_snapshot_epoch",
}:
    integer(shared_traversal.get(key), f"shared_product_traversal.{key}")
if shared_traversal != {
    "retained_snapshots": 1,
    "capacity_accountings": 1,
    "snapshot_copies": 0,
    "source_lease_consumptions": 1,
    "pair_views": 1,
    "higher_support_views": 1,
    "pair_lifetime_completed": True,
    "higher_support_lifetime_completed": True,
    "source_snapshot_epoch": shared_traversal["source_snapshot_epoch"],
} or shared_traversal["source_snapshot_epoch"] < 1:
    fail("shared_product_traversal: unique pair-then-higher snapshot contract did not close")
if (
    shared_traversal.get("pair_lifetime_completed") is not True
    or shared_traversal.get("higher_support_lifetime_completed") is not True
):
    fail("shared_product_traversal: view lifetime flags are not strict booleans")
higher_adapter = exact(
    value.get("higher_support_positive_adapter"),
    {
        "submitted_tasks", "prefetched_tasks", "on_demand_tasks",
        "support_prune_tasks", "query_strict_interior_tasks",
        "evaluate_calls", "synchronizations", "maximum_batch_size",
        "cache_hits", "cache_misses", "native_exact_authority",
        "terminal_classification_native_cuda",
    },
    "higher_support_positive_adapter",
)
for key in {
    "submitted_tasks", "prefetched_tasks", "on_demand_tasks",
    "support_prune_tasks", "query_strict_interior_tasks",
    "evaluate_calls", "synchronizations", "maximum_batch_size",
    "cache_hits", "cache_misses",
}:
    integer(higher_adapter.get(key), f"higher_support_positive_adapter.{key}")
if (
    higher_adapter["submitted_tasks"] < 1
    or higher_adapter["submitted_tasks"] !=
        higher_adapter["support_prune_tasks"] +
        higher_adapter["query_strict_interior_tasks"]
    or higher_adapter["prefetched_tasks"] != higher_adapter["submitted_tasks"]
    or higher_adapter["on_demand_tasks"] != 0
    or higher_adapter["evaluate_calls"] < 1
    or higher_adapter["synchronizations"] != higher_adapter["evaluate_calls"]
    or higher_adapter["maximum_batch_size"] <= 1
    or higher_adapter["cache_misses"] != 0
    or higher_adapter.get("native_exact_authority") is not True
    or higher_adapter.get("terminal_classification_native_cuda") is not False
):
    fail("higher_support_positive_adapter: exact positive-only CUDA receipt did not close")
catalog = exact(
    value.get("automatic_recipe_catalog"),
    {
        "decision", "product_visits", "maximum_frontier_blocks",
        "receipt_attempts", "certified_receipts", "inconclusive_receipts",
        "automatic_search_node_visits",
        "exact_phi_aabb_maximum_evaluations", "recipes",
        "terminal_pairs_without_payload", "pruned_mass", "terminal_mass",
        "mass_closed",
    },
    "automatic_recipe_catalog",
)
for key in set(catalog) - {"mass_closed"}:
    integer(catalog[key], f"automatic_recipe_catalog.{key}")
if (
    catalog["decision"] != 9
    or catalog.get("mass_closed") is not True
    or catalog["recipes"] < 1
    or catalog["pruned_mass"] < 1
    or catalog["terminal_mass"] < 1
    or catalog["recipes"] != catalog["certified_receipts"]
    or catalog["receipt_attempts"] !=
        catalog["certified_receipts"] + catalog["inconclusive_receipts"]
    or catalog["terminal_pairs_without_payload"] != catalog["terminal_mass"]
):
    fail("automatic_recipe_catalog: exact recipe catalog did not close")
pair_cut = exact(
    value.get("pair_cut"),
    {
        "universe", "pruned", "terminal", "submitted_recipes",
        "matched_recipes", "unused_recipes", "certified_prunes", "kernel_launches",
        "synchronizations", "kernel_elapsed_nanoseconds", "cuda_device",
        "serial_device_reference", "scale_eligible",
    },
    "pair_cut",
)
for key in (
    "universe", "pruned", "terminal", "submitted_recipes",
    "matched_recipes", "unused_recipes", "certified_prunes",
    "kernel_launches", "synchronizations", "cuda_device",
):
    integer(pair_cut.get(key), f"pair_cut.{key}")
if (
    pair_cut["universe"] != 66
    or pair_cut["pruned"] + pair_cut["terminal"] != 66
    or catalog["pruned_mass"] + catalog["terminal_mass"] != pair_cut["universe"]
    or catalog["pruned_mass"] != pair_cut["pruned"]
    or catalog["terminal_mass"] != pair_cut["terminal"]
    or pair_cut["submitted_recipes"] != catalog["recipes"]
    or pair_cut["matched_recipes"] != catalog["recipes"]
    or pair_cut["unused_recipes"] != 0
    or pair_cut["certified_prunes"] != catalog["recipes"]
    or pair_cut["terminal"] < 1
    or pair_cut["kernel_launches"] != 1
    or pair_cut["synchronizations"] != 3
    or pair_cut.get("serial_device_reference") is not True
    or pair_cut.get("scale_eligible") is not False
):
    fail("reducer pair cut is not the closed eight-clusters partition")
integer(pair_cut.get("kernel_elapsed_nanoseconds"), "pair_cut.kernel_elapsed_nanoseconds", 1)
classification = exact(
    value.get("pair_classification"),
    {"terminal_pairs", "above_rank", "records", "node_visits"},
    "pair_classification",
)
for key in classification:
    integer(classification[key], f"pair_classification.{key}")
if classification["terminal_pairs"] != pair_cut["terminal"] or classification["above_rank"] > classification["terminal_pairs"] or classification["records"] != classification["terminal_pairs"] - classification["above_rank"] or classification["node_visits"] < 1:
    fail("reducer pair classification does not close")
source = exact(
    value.get("reducer_source"),
    {"events", "seeds", "batches", "visited_batches"},
    "reducer_source",
)
for key in source:
    integer(source[key], f"reducer_source.{key}")
if source["visited_batches"] != source["batches"]:
    fail("reducer source provider replay is incomplete")
forest = exact(
    value.get("forest_reduction"),
    {
        "decision", "plan_decision", "last_preparation_decision",
        "last_live_commit_decision", "last_reducer_fold_decision",
        "plan_lanes", "prepared_tickets", "committed_batches",
        "resolved_keys", "arm_joins", "maximum_transient_closure_nodes",
        "birth_records", "saddles", "atomic_groups", "nodes",
        "final_roots", "logical_output_entries", "conditional_h0_candidate",
    },
    "forest_reduction",
)
for key in set(forest) - {"conditional_h0_candidate"}:
    integer(forest[key], f"forest_reduction.{key}")
for key, expected in {
    "decision": 9,
    "plan_decision": 7,
    "last_preparation_decision": 5,
    "last_live_commit_decision": 8,
    "last_reducer_fold_decision": 8,
}.items():
    if forest[key] != expected:
        fail(f"forest_reduction.{key}: incomplete certified reduction")
if (
    forest.get("conditional_h0_candidate") is not True
    or source["batches"] < 1
    or forest["prepared_tickets"] != source["batches"]
    or forest["committed_batches"] != source["batches"]
    or forest["final_roots"] > forest["nodes"]
    or forest["logical_output_entries"] < forest["final_roots"]
):
    fail("forest_reduction: conditional H0 forest did not close")
pipeline = exact(
    value.get("vertical_target_pipeline"),
    {
        "decision", "source_batches_scanned", "source_groups_scanned",
        "target_order_lookups", "required_sessions", "initialized_sessions",
        "session_audits", "required_groups", "preflight_plans",
        "executed_plans", "replay_advances", "closure_builds",
        "proposal_adapters", "group_audits", "representatives",
        "projected_target_facets", "distinct_target_facets",
        "retained_key_point_references", "closure_terminal_summaries",
        "closure_unresolved_terminals", "closure_active_latent_terminals",
        "closure_resolved_terminals", "proposals", "unresolved_proposals",
        "resolved_proposals", "equal_level_same_target_order_groups",
        "matching_canonical_point_namespace_required",
        "forest_to_cloud_namespace_identity_certified", "forest_relative_only",
        "global_forbidden_structure_materialized",
        "external_target_authority_replayed", "vertical_maps_complete",
    },
    "vertical_target_pipeline",
)
pipeline_boolean_keys = {
    "matching_canonical_point_namespace_required",
    "forest_to_cloud_namespace_identity_certified", "forest_relative_only",
    "global_forbidden_structure_materialized",
    "external_target_authority_replayed", "vertical_maps_complete",
}
for key in set(pipeline) - pipeline_boolean_keys:
    integer(pipeline[key], f"vertical_target_pipeline.{key}")
if (
    pipeline["required_sessions"] < 1
    or pipeline["required_groups"] < 1
    or pipeline["proposals"] < 1
    or pipeline["source_batches_scanned"] != forest["committed_batches"]
    or pipeline["source_groups_scanned"] != pipeline["required_groups"]
    or pipeline["target_order_lookups"] < pipeline["required_sessions"]
    or pipeline["initialized_sessions"] != pipeline["required_sessions"]
    or pipeline["session_audits"] != pipeline["required_sessions"]
    or pipeline["preflight_plans"] != pipeline["required_groups"]
    or pipeline["executed_plans"] != pipeline["required_groups"]
    or pipeline["replay_advances"] != pipeline["required_groups"]
    or pipeline["closure_builds"] != pipeline["required_groups"]
    or pipeline["proposal_adapters"] != pipeline["required_groups"]
    or pipeline["group_audits"] != pipeline["required_groups"]
    or pipeline["representatives"] != pipeline["proposals"]
    or pipeline["projected_target_facets"] < 2 * pipeline["representatives"]
    or pipeline["distinct_target_facets"] < 1
    or pipeline["distinct_target_facets"] > pipeline["projected_target_facets"]
    or pipeline["retained_key_point_references"] <
        pipeline["projected_target_facets"]
    or pipeline["closure_terminal_summaries"] !=
        pipeline["distinct_target_facets"]
    or pipeline["closure_unresolved_terminals"] +
        pipeline["closure_active_latent_terminals"] +
        pipeline["closure_resolved_terminals"] !=
        pipeline["closure_terminal_summaries"]
    or pipeline["unresolved_proposals"] + pipeline["resolved_proposals"] !=
        pipeline["proposals"]
    or pipeline["equal_level_same_target_order_groups"] >
        pipeline["required_groups"]
    or pipeline["decision"] !=
        (15 if pipeline["unresolved_proposals"] != 0 else 14)
    or pipeline.get("matching_canonical_point_namespace_required") is not True
    or pipeline.get("forest_to_cloud_namespace_identity_certified") is not True
    or pipeline.get("forest_relative_only") is not True
    or pipeline.get("global_forbidden_structure_materialized") is not False
    or pipeline.get("external_target_authority_replayed") is not False
    or pipeline.get("vertical_maps_complete") is not False
):
    fail("vertical_target_pipeline: forest-relative proposal pipeline did not close")
vertical = exact(
    value.get("vertical_journal"),
    {
        "decision", "expected_labels", "missing_labels",
        "unresolved_labels", "resolved_labels", "partial_groups",
        "complete_groups", "external_target_authority_replayed",
        "all_naturality_squares_replayed", "vertical_maps_complete",
        "public_status_claimed",
    },
    "vertical_journal",
)
vertical_boolean_keys = {
    "external_target_authority_replayed", "all_naturality_squares_replayed",
    "vertical_maps_complete", "public_status_claimed",
}
for key in set(vertical) - vertical_boolean_keys:
    integer(vertical[key], f"vertical_journal.{key}")
expected_vertical_decision = (
    10
    if vertical["unresolved_labels"] == 0 and vertical["partial_groups"] == 0
    else 9
)
if (
    vertical["decision"] != expected_vertical_decision
    or vertical["expected_labels"] != pipeline["proposals"]
    or vertical["missing_labels"] != 0
    or vertical["unresolved_labels"] + vertical["resolved_labels"] !=
        vertical["expected_labels"]
    or vertical["unresolved_labels"] != pipeline["unresolved_proposals"]
    or vertical["resolved_labels"] != pipeline["resolved_proposals"]
    or vertical["partial_groups"] + vertical["complete_groups"] !=
        pipeline["required_groups"]
    or vertical.get("external_target_authority_replayed") is not False
    or vertical.get("all_naturality_squares_replayed") is not False
    or vertical.get("vertical_maps_complete") is not False
    or vertical.get("public_status_claimed") is not False
):
    fail("vertical_journal: zero-missing conditional journal did not close")
digests = exact(
    value.get("digests"),
    {
        "submitted_recipe_fnv1a", "final_cut_fnv1a",
        "pair_cloud_sha256", "pair_lbvh_sha256", "pair_output_sha256",
        "pair_semantic_sha256", "higher_semantic_sha256",
        "higher_output_chain_sha256", "higher_checkpoint_sha256",
        "normalized_terminal_output_sha256",
        "reducer_source_manifest_sha256",
        "source_forest_canonical_cloud_sha256",
        "replayed_canonical_cloud_sha256",
    },
    "reducer digests",
)
integer(digests.get("submitted_recipe_fnv1a"), "digests.submitted_recipe_fnv1a", 1)
integer(digests.get("final_cut_fnv1a"), "digests.final_cut_fnv1a", 1)
for key in set(digests) - {"submitted_recipe_fnv1a", "final_cut_fnv1a"}:
    digest = digests[key]
    if (
        not isinstance(digest, str)
        or len(digest) != 64
        or any(character not in "0123456789abcdef" for character in digest)
        or digest == "0" * 64
    ):
        fail(f"digests.{key}: invalid certified SHA-256")
if (
    digests["source_forest_canonical_cloud_sha256"] !=
    digests["replayed_canonical_cloud_sha256"]
):
    fail("digests: forest source and replayed canonical cloud identities diverge")
timings = exact(
    value.get("timings_nanoseconds"),
    {
        "generation", "canonicalization", "lbvh_build", "scheduler_setup_wall",
        "automatic_recipe_catalog_wall", "scheduler_wall",
        "cut_validation_wall", "pair_adapter_wall", "higher_support_wall",
        "bridge_wall", "bridge_output_inspection_wall",
        "provider_replay_wall", "forest_reduction_wrapper_wall",
        "forest_plan_wall", "forest_reducer_setup_wall",
        "forest_reducer_stream_wall", "forest_finish_wall",
        "forest_reduction_internal_total_wall",
        "vertical_target_proposal_pipeline_wall", "vertical_journal_wall",
        "total",
    },
    "reducer timings",
)
for key in timings:
    integer(timings[key], f"reducer timings.{key}")
forest_internal_components = {
    "forest_plan_wall", "forest_reducer_setup_wall",
    "forest_reducer_stream_wall", "forest_finish_wall",
}
if timings["forest_reduction_internal_total_wall"] < sum(
    timings[key] for key in forest_internal_components
):
    fail("forest internal total timing is smaller than its components")
if (
    timings["forest_reduction_wrapper_wall"]
    < timings["forest_reduction_internal_total_wall"]
):
    fail("forest wrapper timing is smaller than its nested internal total")
disjoint_total_components = set(timings) - {
    "total", "forest_reduction_internal_total_wall", *forest_internal_components,
}
if timings["total"] < sum(timings[key] for key in disjoint_total_components):
    fail("reducer total timing is smaller than its disjoint components")
if value.get("claims") != {
    "ordinary_or_higher_order_delaunay": False,
    "global_pair_matrix": False,
    "hierarchy_reduction": True,
    "conditional_h0_only": True,
    "forest_relative_vertical_target_proposals": True,
    "vertical_target_authority": False,
    "vertical_maps_complete": False,
    "public_exact": False,
}:
    fail("reducer qualification makes an unexpected claim")
PY
    }

    validate_phase15_direct_scale_run() {
        local result_path="$1"
        local expected_point_count="$2"
        python3 -B - "${result_path}" "${expected_point_count}" "${HEAD_SHA}" <<'PY'
import json
from pathlib import Path
import sys


def fail(message):
    raise SystemExit(message)


def reject_duplicates(pairs):
    value = {}
    for key, item in pairs:
        if key in value:
            fail(f"duplicate direct-scale JSON key: {key}")
        value[key] = item
    return value


path = Path(sys.argv[1])
point_count = int(sys.argv[2])
git_sha = sys.argv[3]
raw = path.read_text(encoding="utf-8")
lines = raw.splitlines()
if len(lines) != 1 or raw != lines[0] + "\n":
    fail("direct-scale stdout must contain one newline-terminated JSON object")
value = json.loads(lines[0], object_pairs_hook=reject_duplicates)
if not isinstance(value, dict):
    fail("direct-scale result must be an object")
for key, expected in {
    "schema": "morsehgp3d.phase15.morton_yao48_device_tiled_pair_frontier_direct_scale.v1",
    "backend": "cuda_g4",
    "profile": "hgp_reduced",
    "mode": "device_resident_budgeted_morton_yao48_anchor_tiles_direct_scale",
    "public_status": "not_claimed",
    "deployment_status": "profile_only",
    "component_only": True,
    "profile_only": True,
    "run_kind": "direct_scale",
    "exactness_claimed": False,
    "product_claimed": False,
    "qualification_claimed": False,
    "scalability_claimed": False,
    "scientific_claimed": False,
}.items():
    if value.get(key) != expected or type(value.get(key)) is not type(expected):
        fail(f"direct-scale.{key}: unexpected value")
if value.get("git_sha") != git_sha:
    fail("direct-scale git binding mismatch")
if "error" in value:
    fail("direct-scale returned a failure diagnostic despite exit status zero")
for key, expected in {
    "point_count": point_count,
    "family": "affine_uniform_binary64",
    "maximum_closed_rank": 11,
    "prune_semantics": "closed_rank_window",
    "required_witness_count": 10,
    "direct_large_scale_guard_passed": True,
    "execution_success": True,
    "backpressure_validated": True,
    "dense_pair_fallback_performed": False,
    "global_pair_matrix_materialized": False,
    "higher_order_structure_materialized": False,
    "ordinary_delaunay_materialized": False,
    "ordinary_emst_computed": False,
    "raw_candidate_or_prune_view_accessed": False,
    "scientific_pair_catalog_published": False,
    "warm_e2e_slo_claimed": False,
}.items():
    if value.get(key) != expected or type(value.get(key)) is not type(expected):
        fail(f"direct-scale.{key}: invalid profile contract")
for key in (
    "generation_ns", "canonicalization_ns", "build_ns", "lease_release_ns",
    "host_release_ns", "context_creation_ns", "context_release_ns",
    "frontier_ns", "total_ns", "candidate_pair_mass",
    "certified_pruned_pair_mass", "unresolved_pair_mass",
    "unordered_pair_universe_count",
):
    if type(value.get(key)) is not int or value[key] < 0:
        fail(f"direct-scale.{key}: invalid integer")
if value["total_ns"] < value["generation_ns"] + value["canonicalization_ns"] + value["frontier_ns"]:
    fail("direct-scale total timing is smaller than major components")
if value["candidate_pair_mass"] + value["certified_pruned_pair_mass"] + value["unresolved_pair_mass"] != value["unordered_pair_universe_count"]:
    fail("direct-scale pair mass partition does not close")
if value.get("pair_mass_partition_validated") is not True:
    fail("direct-scale pair mass partition is not validated")
for key in ("coverage_complete", "coverage_success", "complete_pair_mass_validated", "censored"):
    if type(value.get(key)) is not bool:
        fail(f"direct-scale.{key}: expected a boolean diagnostic")
if value.get("scale_eligible", False) is not False:
    fail("direct-scale must not become scale_eligible")
if value["coverage_complete"] != value["coverage_success"] or value["complete_pair_mass_validated"] != (value["coverage_complete"] and value["unresolved_pair_mass"] == 0):
    fail("direct-scale completion diagnostics are incoherent")
if value["coverage_complete"]:
    if value["censored"] or value["unresolved_pair_mass"] != 0 or value.get("stop_reason") != "none":
        fail("direct-scale complete coverage carries a censored terminal receipt")
elif (
    not value["censored"]
    or value["unresolved_pair_mass"] == 0
    or value.get("stop_reason") in (None, "none")
):
    fail("direct-scale incomplete coverage lacks its terminal censure receipt")
PY
    }

    begin_unit "phase15-resident-transactional-semantic-reducer-build"
    if ! run_container "phase15-resident-transactional-semantic-reducer-build" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_BUILD_LOG}" \
        cmake --build "${MODULE_DIR}" \
        --target morsehgp3d_gpu_exact_pair_block_to_reducer_source_cuda_qualification \
        --parallel 2; then
        report_failure_log "phase15-resident-transactional-semantic-reducer-build" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_BUILD_LOG}"
        die "La cible pair-block-to-reducer-source Phase 15 n'a pas pu être construite."
    fi
    [[ -f "${BUILD_DIR}/${PHASE15_REDUCER_SOURCE_BINARY_RELATIVE#build/}" && \
        ! -L "${BUILD_DIR}/${PHASE15_REDUCER_SOURCE_BINARY_RELATIVE#build/}" && \
        -x "${BUILD_DIR}/${PHASE15_REDUCER_SOURCE_BINARY_RELATIVE#build/}" ]] || \
        die "Le binaire pair-block-to-reducer-source Phase 15 n'a pas été construit sûrement."

    run_phase15_resident_campaign_unit \
        "phase15-resident-transactional-semantic-reducer-k5" \
        "${PHASE15_RESIDENT_REDUCER_TIMEOUT_SECONDS}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K5_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K5_STDERR_LOG}" \
        strict \
        "${PHASE15_REDUCER_SOURCE_BINARY_PATH}" --K 5 --require-complete
    phase15_resident_reducer_k5_started_ns="${PHASE15_RESIDENT_CAMPAIGN_LAST_STARTED_NS}"
    phase15_resident_reducer_k5_finished_ns="${PHASE15_RESIDENT_CAMPAIGN_LAST_FINISHED_NS}"
    validate_phase15_reducer_source_run \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K5_LOG}" 5 || \
        die "La sortie pair-block-to-reducer-source K=5 viole le contrat JSON strict."

    run_phase15_resident_campaign_unit \
        "phase15-resident-transactional-semantic-reducer-k10" \
        "${PHASE15_RESIDENT_REDUCER_TIMEOUT_SECONDS}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K10_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K10_STDERR_LOG}" \
        strict \
        "${PHASE15_REDUCER_SOURCE_BINARY_PATH}" --K 10 --require-complete
    phase15_resident_reducer_k10_started_ns="${PHASE15_RESIDENT_CAMPAIGN_LAST_STARTED_NS}"
    phase15_resident_reducer_k10_finished_ns="${PHASE15_RESIDENT_CAMPAIGN_LAST_FINISHED_NS}"
    validate_phase15_reducer_source_run \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K10_LOG}" 10 || \
        die "La sortie pair-block-to-reducer-source K=10 viole le contrat JSON strict."

    begin_unit "phase15-resident-transactional-semantic-frontier-build"
    if ! run_container "phase15-resident-transactional-semantic-frontier-build" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_FRONTIER_BUILD_LOG}" \
        cmake --build "${MODULE_DIR}" \
        --target morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification \
        --parallel 8; then
        report_failure_log "phase15-resident-transactional-semantic-frontier-build" \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_FRONTIER_BUILD_LOG}"
        die "La cible device-frontier de la campagne resident Phase 15 n'a pas pu être construite."
    fi
    [[ -f "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_50K_BINARY_RELATIVE#build/}" && \
        ! -L "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_50K_BINARY_RELATIVE#build/}" && \
        -x "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_50K_BINARY_RELATIVE#build/}" ]] || \
        die "Le binaire device-frontier de la campagne resident Phase 15 n'a pas été construit sûrement."

    run_phase15_resident_campaign_unit \
        "phase15-resident-transactional-semantic-frontier-50k" \
        "${PHASE15_RESIDENT_FRONTIER_50K_TIMEOUT_SECONDS}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_FRONTIER_50K_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_FRONTIER_50K_STDERR_LOG}" \
        strict \
        "${PHASE15_DEVICE_FRONTIER_50K_BINARY_PATH}" \
        --point-count 50000 --family adversarial_mixed_dyadic \
        --maximum-closed-rank 11 --anchor-tile-capacity 4096 \
        --seed "${PHASE15_DEVICE_FRONTIER_50K_SEED}"
    phase15_resident_frontier_50k_started_ns="${PHASE15_RESIDENT_CAMPAIGN_LAST_STARTED_NS}"
    phase15_resident_frontier_50k_finished_ns="${PHASE15_RESIDENT_CAMPAIGN_LAST_FINISHED_NS}"
    python3 -B - \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_FRONTIER_50K_LOG}" \
        "$(dirname -- "${PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER}")" \
        "${HEAD_SHA}" <<'PY'
from pathlib import Path
import sys

sys.path.insert(0, sys.argv[2])
import assemble_phase15_device_frontier_50k_qualification as assembler

raw = Path(sys.argv[1]).read_text(encoding="utf-8")
value = assembler.parse_single_line_json(raw, "resident campaign frontier 50k")
assembler.validate_qualification(
    value,
    git_sha=sys.argv[3],
    maximum_closed_rank=11,
    warm_profile=False,
)
PY

    run_phase15_resident_campaign_unit \
        "phase15-resident-transactional-semantic-direct-10m" \
        "${PHASE15_RESIDENT_DIRECT_10M_TIMEOUT_SECONDS}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_10M_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_10M_STDERR_LOG}" \
        canonical-direct-scale-timeout \
        "${PHASE15_DEVICE_FRONTIER_50K_BINARY_PATH}" \
        --point-count 10000000 --family affine_uniform_binary64 \
        --maximum-closed-rank 11 --direct-scale \
        --anchor-tile-capacity 4096 --seed "${PHASE15_DEVICE_FRONTIER_50K_SEED}"
    phase15_resident_direct_10m_started_ns="${PHASE15_RESIDENT_CAMPAIGN_LAST_STARTED_NS}"
    phase15_resident_direct_10m_finished_ns="${PHASE15_RESIDENT_CAMPAIGN_LAST_FINISHED_NS}"
    phase15_resident_direct_10m_exit_status="${PHASE15_RESIDENT_CAMPAIGN_LAST_EXIT_STATUS}"
    if ((phase15_resident_direct_10m_exit_status == 0)); then
        validate_phase15_direct_scale_run \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_10M_LOG}" 10000000 || \
            die "La sortie direct-scale 10M viole le contrat JSON de diagnostic."
    fi

    run_phase15_resident_campaign_unit \
        "phase15-resident-transactional-semantic-direct-30m" \
        "${PHASE15_RESIDENT_DIRECT_30M_TIMEOUT_SECONDS}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_30M_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_30M_STDERR_LOG}" \
        canonical-direct-scale-timeout \
        "${PHASE15_DEVICE_FRONTIER_50K_BINARY_PATH}" \
        --point-count 30000000 --family affine_uniform_binary64 \
        --maximum-closed-rank 11 --direct-scale \
        --anchor-tile-capacity 4096 --seed "${PHASE15_DEVICE_FRONTIER_50K_SEED}"
    phase15_resident_direct_30m_started_ns="${PHASE15_RESIDENT_CAMPAIGN_LAST_STARTED_NS}"
    phase15_resident_direct_30m_finished_ns="${PHASE15_RESIDENT_CAMPAIGN_LAST_FINISHED_NS}"
    phase15_resident_direct_30m_exit_status="${PHASE15_RESIDENT_CAMPAIGN_LAST_EXIT_STATUS}"
    if ((phase15_resident_direct_30m_exit_status == 0)); then
        validate_phase15_direct_scale_run \
            "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_30M_LOG}" 30000000 || \
            die "La sortie direct-scale 30M viole le contrat JSON de diagnostic."
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
    if ((MORTON_YAO48_SEED_WORK_PROFILE == 1)); then
        [[ -f "${BUILD_DIR}/morsehgp3d-cuda-release/morsehgp3d_gpu_morton_yao48_seed_work_profile" && \
            ! -L "${BUILD_DIR}/morsehgp3d-cuda-release/morsehgp3d_gpu_morton_yao48_seed_work_profile" && \
            -x "${BUILD_DIR}/morsehgp3d-cuda-release/morsehgp3d_gpu_morton_yao48_seed_work_profile" ]] || \
            die "Le binaire CUDA du profil Morton/LBVH/Yao48 n'a pas été construit sûrement."
        for point_count in 50000 1000000 10000000 30000000; do
            work_profile_label="phase15-morton-yao48-seed-${point_count}"
            work_profile_log="${RESULT_DIR}/${work_profile_label}.json"
            work_profile_stderr_log="${LOG_DIR}/${work_profile_label}.stderr.log"
            PHASE5_K1_BORUVKA_WORK_PROFILE_LOGS+=("${work_profile_log}")
            begin_unit "${work_profile_label}"
            if ! run_container_split_output "${work_profile_label}" \
                "${work_profile_log}" "${work_profile_stderr_log}" \
                "${MORTON_YAO48_SEED_WORK_PROFILE_BINARY_PATH}" \
                --point-count "${point_count}" \
                --max-order 10 \
                --morton-window 64 \
                --warm-repetitions 1 \
                --maximum-directed-pair-checks 4000000000 \
                --maximum-neighbor-records 300000000 \
                --seed 1; then
                report_failure_log "${work_profile_label}" "${work_profile_log}"
                report_failure_log "${work_profile_label}-stderr" \
                    "${work_profile_stderr_log}"
                die "Le profil Morton/LBVH/Yao48 a échoué pour n=${point_count}."
            fi
            python3 -B - "${work_profile_log}" "${HEAD_SHA}" \
                "${point_count}" <<'PY'
import json
from pathlib import Path
import sys

path = Path(sys.argv[1])
raw = path.read_text(encoding="utf-8")
lines = raw.splitlines()
if len(lines) != 1 or raw != lines[0] + "\n":
    raise SystemExit("le profil Morton/Yao48 n'est pas un JSON mono-ligne terminé par LF")
value = json.loads(raw)
if value.get("schema") != "morsehgp3d.phase15.morton_yao48_seed_work_profile.v1":
    raise SystemExit("schéma du profil Morton/Yao48 invalide")
if value.get("artifact_role") != "benchmark_only":
    raise SystemExit("le profil Morton/Yao48 doit rester benchmark_only")
if value.get("git_sha") != sys.argv[2] or value.get("point_count") != int(sys.argv[3]):
    raise SystemExit("identité du profil Morton/Yao48 invalide")
mass = value.get("pair_mass", {})
if mass.get("survivors", -1) + mass.get("certified_pruned", -1) + mass.get("unresolved", -1) != mass.get("universe"):
    raise SystemExit("la partition de masse Morton/Yao48 ne ferme pas")
claims = value.get("claims", {})
for key in (
    "dense_pair_fallback_performed", "global_pair_matrix_materialized",
    "delaunay_materialized", "geogram_invoked", "pdel_invoked",
    "emst_or_boruvka_invoked", "exact_pair_frontier_claimed",
    "scientific_exactness_claimed", "public_status_claimed",
):
    if claims.get(key) is not False:
        raise SystemExit(f"revendication interdite dans le profil Morton/Yao48: {key}")
PY
        done
        [[ "${#PHASE5_K1_BORUVKA_WORK_PROFILE_LOGS[@]}" -eq 4 ]] || \
            die "La matrice Morton/LBVH/Yao48 doit contenir quatre tailles."
    else
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
    --radial-subtree-log "${MORTON_YAO48_RADIAL_SUBTREE_FILTER_LOG}" \
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
    if ((MORTON_YAO48_SEED_WORK_PROFILE == 1)); then
        python3 -B - "${HEAD_SHA}" "${PUBLISH_TEMP}" \
            "${BUILD_DIR}/morsehgp3d-cuda-release/morsehgp3d_gpu_morton_yao48_seed_work_profile" \
            "${PHASE5_WORK_PROFILE_PUBLISH_TEMP}" \
            "${PHASE5_K1_BORUVKA_WORK_PROFILE_LOGS[@]}" <<'PY'
import hashlib
import json
from pathlib import Path
import sys

git_sha = sys.argv[1]
environment_path = Path(sys.argv[2])
binary_path = Path(sys.argv[3])
output_path = Path(sys.argv[4])
log_paths = [Path(value) for value in sys.argv[5:]]
expected_sizes = [50000, 1000000, 10000000, 30000000]
if len(log_paths) != len(expected_sizes):
    raise SystemExit("quatre mesures Morton/Yao48 sont obligatoires")
measurements = []
for path, expected_size in zip(log_paths, expected_sizes, strict=True):
    value = json.loads(path.read_text(encoding="utf-8"))
    if value.get("point_count") != expected_size:
        raise SystemExit("ordre des tailles Morton/Yao48 invalide")
    measurements.append(value)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

artifact = {
    "artifact_role": "benchmark_only",
    "backend": "cuda_g4_plus_host_binary64",
    "binary_sha256": digest(binary_path),
    "environment_artifact_sha256": digest(environment_path),
    "expected_point_counts": expected_sizes,
    "git": {"clean": True, "sha": git_sha},
    "measurements": measurements,
    "mode": "bounded_morton_window_yao48_seed_proxy",
    "phase": "15",
    "profile": "hgp_reduced",
    "provenance": {
        "environment_artifact_schema": "morsehgp3d.phase3.qualification.v1",
        "environment_artifact_sha256": digest(environment_path),
    },
    "schema": "morsehgp3d.phase15.morton_yao48_seed_work_profile_artifact.v1",
    "status": "worker_passed_pending_shutdown",
    "scientific_exactness_claimed": False,
    "scalability_claimed": False,
    "public_status_claimed": False,
    "delaunay_materialized": False,
    "geogram_invoked": False,
    "pdel_invoked": False,
    "emst_or_boruvka_invoked": False,
    "vm_lifecycle": {
        "guest_shutdown_guard_verified": True,
        "stop_responsibility": "external_orchestrator",
        "worker_mutates_gcp": False,
    },
}
output_path.write_text(
    json.dumps(artifact, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
    + "\n",
    encoding="utf-8",
)
PY
    else
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

if [[ -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" ]]; then
    python3 -B "${PHASE9_PAIR_SUPPORT_PHI_ASSEMBLER}" \
        --git-sha "${HEAD_SHA}" \
        --base-image-ref "${BASE_IMAGE_REF}" \
        --image-ref "${IMAGE_REF}" \
        --image-id "${IMAGE_ID}" \
        --environment-artifact "${PUBLISH_TEMP}" \
        --qualification-log "${PHASE9_PAIR_SUPPORT_PHI_QUALIFICATION_LOG}" \
        --elf-log "${PHASE9_PAIR_SUPPORT_PHI_ELF_LOG}" \
        --ptx-log "${PHASE9_PAIR_SUPPORT_PHI_PTX_LOG}" \
        --ptx-stderr-log "${PHASE9_PAIR_SUPPORT_PHI_PTX_STDERR_LOG}" \
        --memcheck-log "${PHASE9_PAIR_SUPPORT_PHI_MEMCHECK_LOG}" \
        --racecheck-log "${PHASE9_PAIR_SUPPORT_PHI_RACECHECK_LOG}" \
        --binary "${BUILD_DIR}/${PHASE9_PAIR_SUPPORT_PHI_BINARY_RELATIVE#build/}" \
        --output "${PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP}"
fi

if [[ -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}" ]]; then
    python3 -B "${PHASE15_EXACT_DIAMETRAL_PHI_ASSEMBLER}" \
        --git-sha "${HEAD_SHA}" \
        --base-image-ref "${BASE_IMAGE_REF}" \
        --image-ref "${IMAGE_REF}" \
        --image-id "${IMAGE_ID}" \
        --environment-artifact "${PUBLISH_TEMP}" \
        --qualification-log "${PHASE15_EXACT_DIAMETRAL_PHI_QUALIFICATION_LOG}" \
        --elf-log "${PHASE15_EXACT_DIAMETRAL_PHI_ELF_LOG}" \
        --ptx-log "${PHASE15_EXACT_DIAMETRAL_PHI_PTX_LOG}" \
        --ptx-stderr-log "${PHASE15_EXACT_DIAMETRAL_PHI_PTX_STDERR_LOG}" \
        --memcheck-log "${PHASE15_EXACT_DIAMETRAL_PHI_MEMCHECK_LOG}" \
        --racecheck-log "${PHASE15_EXACT_DIAMETRAL_PHI_RACECHECK_LOG}" \
        --release-build-log "${PHASE15_EXACT_DIAMETRAL_PHI_RELEASE_BUILD_LOG}" \
        --audit-build-log "${PHASE15_EXACT_DIAMETRAL_PHI_AUDIT_BUILD_LOG}" \
        --binary "${BUILD_DIR}/${PHASE15_EXACT_DIAMETRAL_PHI_BINARY_RELATIVE#build/}" \
        --output "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}" ]]; then
    declare -a phase15_device_frontier_50k_warm_assembler_option=()
    if ((PHASE15_DEVICE_FRONTIER_50K_WARM_PROFILE == 1)); then
        phase15_device_frontier_50k_warm_assembler_option=(--warm-profile)
    fi
    python3 -B "${PHASE15_DEVICE_FRONTIER_50K_ASSEMBLER}" \
        --git-sha "${HEAD_SHA}" \
        --base-image-ref "${BASE_IMAGE_REF}" \
        --image-ref "${IMAGE_REF}" \
        --image-id "${IMAGE_ID}" \
        --environment-artifact "${PUBLISH_TEMP}" \
        --build-log "${PHASE15_DEVICE_FRONTIER_50K_BUILD_LOG}" \
        --qualification-log "${PHASE15_DEVICE_FRONTIER_50K_QUALIFICATION_LOG}" \
        --qualification-stderr-log "${PHASE15_DEVICE_FRONTIER_50K_STDERR_LOG}" \
        --timing-log "${PHASE15_DEVICE_FRONTIER_50K_TIMING_LOG}" \
        --binary "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_50K_BINARY_RELATIVE#build/}" \
        --maximum-closed-rank \
            "${PHASE15_DEVICE_FRONTIER_50K_MAXIMUM_CLOSED_RANK}" \
        "${phase15_device_frontier_50k_warm_assembler_option[@]}" \
        --output "${PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}" ]]; then
    python3 -B "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ASSEMBLER}" \
        --git-sha "${HEAD_SHA}" \
        --base-image-ref "${BASE_IMAGE_REF}" \
        --image-ref "${IMAGE_REF}" \
        --image-id "${IMAGE_ID}" \
        --environment-artifact "${PUBLISH_TEMP}" \
        --release-build-log "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_RELEASE_BUILD_LOG}" \
        --audit-build-log "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_AUDIT_BUILD_LOG}" \
        --qualification-log "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_LOG}" \
        --qualification-stderr-log "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_QUALIFICATION_STDERR_LOG}" \
        --elf-log "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_ELF_LOG}" \
        --ptx-log "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PTX_LOG}" \
        --ptx-stderr-log "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PTX_STDERR_LOG}" \
        --memcheck-log "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_MEMCHECK_LOG}" \
        --racecheck-log "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_RACECHECK_LOG}" \
        --binary "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_BINARY_RELATIVE#build/}" \
        --output "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}" ]]; then
    python3 -B "${PHASE15_RANKED_PAIR_CLASSIFIER_ASSEMBLER}" \
        --git-sha "${HEAD_SHA}" \
        --base-image-ref "${BASE_IMAGE_REF}" \
        --image-ref "${IMAGE_REF}" \
        --image-id "${IMAGE_ID}" \
        --environment-artifact "${PUBLISH_TEMP}" \
        --release-build-log "${PHASE15_RANKED_PAIR_CLASSIFIER_RELEASE_BUILD_LOG}" \
        --audit-build-log "${PHASE15_RANKED_PAIR_CLASSIFIER_AUDIT_BUILD_LOG}" \
        --qualification-log "${PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_LOG}" \
        --qualification-stderr-log "${PHASE15_RANKED_PAIR_CLASSIFIER_QUALIFICATION_STDERR_LOG}" \
        --elf-log "${PHASE15_RANKED_PAIR_CLASSIFIER_ELF_LOG}" \
        --ptx-log "${PHASE15_RANKED_PAIR_CLASSIFIER_PTX_LOG}" \
        --ptx-stderr-log "${PHASE15_RANKED_PAIR_CLASSIFIER_PTX_STDERR_LOG}" \
        --memcheck-log "${PHASE15_RANKED_PAIR_CLASSIFIER_MEMCHECK_LOG}" \
        --racecheck-log "${PHASE15_RANKED_PAIR_CLASSIFIER_RACECHECK_LOG}" \
        --binary "${BUILD_DIR}/${PHASE15_RANKED_PAIR_CLASSIFIER_BINARY_RELATIVE#build/}" \
        --output "${PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}" ]]; then
    python3 -B "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ASSEMBLER}" \
        --git-sha "${HEAD_SHA}" \
        --base-image-ref "${BASE_IMAGE_REF}" \
        --image-ref "${IMAGE_REF}" \
        --image-id "${IMAGE_ID}" \
        --environment-artifact "${PUBLISH_TEMP}" \
        --release-build-log "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_RELEASE_BUILD_LOG}" \
        --audit-build-log "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_AUDIT_BUILD_LOG}" \
        --qualification-log "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_QUALIFICATION_LOG}" \
        --qualification-stderr-log "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_QUALIFICATION_STDERR_LOG}" \
        --elf-log "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_ELF_LOG}" \
        --ptx-log "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PTX_LOG}" \
        --ptx-stderr-log "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PTX_STDERR_LOG}" \
        --memcheck-log "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_MEMCHECK_LOG}" \
        --racecheck-log "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_RACECHECK_LOG}" \
        --binary "${BUILD_DIR}/${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_BINARY_RELATIVE#build/}" \
        --output "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}" ]]; then
    python3 -B - \
        "${HEAD_SHA}" \
        "${PUBLISH_TEMP}" \
        "${BUILD_DIR}/${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_BINARY_RELATIVE#build/}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K5_STDERR_LOG}" \
        "${phase15_resident_transactional_semantic_k5_started_ns}" \
        "${phase15_resident_transactional_semantic_k5_finished_ns}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_K10_STDERR_LOG}" \
        "${phase15_resident_transactional_semantic_k10_started_ns}" \
        "${phase15_resident_transactional_semantic_k10_finished_ns}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_RELEASE_BUILD_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_AUDIT_BUILD_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_ELF_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PTX_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PTX_STDERR_LOG}" \
        "${BASE_IMAGE_REF}" "${IMAGE_REF}" "${IMAGE_ID}" \
        "${BUILD_DIR}/${PHASE15_REDUCER_SOURCE_BINARY_RELATIVE#build/}" \
        "${BUILD_DIR}/${PHASE15_DEVICE_FRONTIER_50K_BINARY_RELATIVE#build/}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_BUILD_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_FRONTIER_BUILD_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K5_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K5_STDERR_LOG}" \
        "${phase15_resident_reducer_k5_started_ns}" \
        "${phase15_resident_reducer_k5_finished_ns}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K10_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_REDUCER_K10_STDERR_LOG}" \
        "${phase15_resident_reducer_k10_started_ns}" \
        "${phase15_resident_reducer_k10_finished_ns}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_FRONTIER_50K_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_FRONTIER_50K_STDERR_LOG}" \
        "${phase15_resident_frontier_50k_started_ns}" \
        "${phase15_resident_frontier_50k_finished_ns}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_10M_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_10M_STDERR_LOG}" \
        "${phase15_resident_direct_10m_started_ns}" \
        "${phase15_resident_direct_10m_finished_ns}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_30M_LOG}" \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_DIRECT_30M_STDERR_LOG}" \
        "${phase15_resident_direct_30m_started_ns}" \
        "${phase15_resident_direct_30m_finished_ns}" \
        "${phase15_resident_direct_10m_exit_status}" \
        "${phase15_resident_direct_30m_exit_status}" \
        "${PHASE15_RESIDENT_REDUCER_TIMEOUT_SECONDS}" \
        "${PHASE15_RESIDENT_FRONTIER_50K_TIMEOUT_SECONDS}" \
        "${PHASE15_RESIDENT_DIRECT_10M_TIMEOUT_SECONDS}" \
        "${PHASE15_RESIDENT_DIRECT_30M_TIMEOUT_SECONDS}" <<'PY'
import hashlib
import json
import os
from pathlib import Path
import re
import sys


def fail(message):
    raise SystemExit(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


git_sha = sys.argv[1]
environment_path = Path(sys.argv[2])
binary_path = Path(sys.argv[3])
output_path = Path(sys.argv[4])
k5_path = Path(sys.argv[5])
k5_stderr_path = Path(sys.argv[6])
k5_started = int(sys.argv[7])
k5_finished = int(sys.argv[8])
k10_path = Path(sys.argv[9])
k10_stderr_path = Path(sys.argv[10])
k10_started = int(sys.argv[11])
k10_finished = int(sys.argv[12])
release_build_path = Path(sys.argv[13])
audit_build_path = Path(sys.argv[14])
elf_path = Path(sys.argv[15])
ptx_path = Path(sys.argv[16])
ptx_stderr_path = Path(sys.argv[17])
base_image_ref, image_ref, image_id = sys.argv[18:21]
reducer_binary_path = Path(sys.argv[21])
frontier_binary_path = Path(sys.argv[22])
reducer_build_path = Path(sys.argv[23])
frontier_build_path = Path(sys.argv[24])
reducer_k5_path = Path(sys.argv[25])
reducer_k5_stderr_path = Path(sys.argv[26])
reducer_k5_started = int(sys.argv[27])
reducer_k5_finished = int(sys.argv[28])
reducer_k10_path = Path(sys.argv[29])
reducer_k10_stderr_path = Path(sys.argv[30])
reducer_k10_started = int(sys.argv[31])
reducer_k10_finished = int(sys.argv[32])
frontier_50k_path = Path(sys.argv[33])
frontier_50k_stderr_path = Path(sys.argv[34])
frontier_50k_started = int(sys.argv[35])
frontier_50k_finished = int(sys.argv[36])
direct_10m_path = Path(sys.argv[37])
direct_10m_stderr_path = Path(sys.argv[38])
direct_10m_started = int(sys.argv[39])
direct_10m_finished = int(sys.argv[40])
direct_30m_path = Path(sys.argv[41])
direct_30m_stderr_path = Path(sys.argv[42])
direct_30m_started = int(sys.argv[43])
direct_30m_finished = int(sys.argv[44])
direct_10m_exit_status = int(sys.argv[45])
direct_30m_exit_status = int(sys.argv[46])
reducer_timeout_seconds = int(sys.argv[47])
frontier_50k_timeout_seconds = int(sys.argv[48])
direct_10m_timeout_seconds = int(sys.argv[49])
direct_30m_timeout_seconds = int(sys.argv[50])
if (
    reducer_timeout_seconds,
    frontier_50k_timeout_seconds,
    direct_10m_timeout_seconds,
    direct_30m_timeout_seconds,
) != (300, 600, 2400, 5400):
    fail("resident campaign timeout contract drift")

environment = json.loads(environment_path.read_text(encoding="utf-8"))
if (
    not isinstance(environment, dict)
    or environment.get("schema") != "morsehgp3d.phase3.qualification.v1"
    or environment.get("status") != "worker_passed_pending_shutdown"
    or environment.get("git") != {"clean": True, "sha": git_sha}
):
    fail("invalid Phase 3 environment artifact binding")
if not binary_path.is_file() or binary_path.is_symlink():
    fail("resident qualification binary is absent or symbolic")
for campaign_binary_path in (reducer_binary_path, frontier_binary_path):
    if not campaign_binary_path.is_file() or campaign_binary_path.is_symlink():
        fail("a resident campaign binary is absent or symbolic")

elf_text = elf_path.read_text(encoding="utf-8")
ptx_text = ptx_path.read_text(encoding="utf-8")
ptx_stderr_text = ptx_stderr_path.read_text(encoding="utf-8")
architectures = sorted(set(re.findall(r"sm_[0-9]+", elf_text)))
if architectures != ["sm_120"]:
    fail("resident qualification ELF is not exclusively sm_120")
if ptx_text.strip():
    fail("resident qualification binary embeds PTX")
if k5_stderr_path.read_text(encoding="utf-8").strip():
    fail("resident K=5 qualification wrote stderr")
if k10_stderr_path.read_text(encoding="utf-8").strip():
    fail("resident K=10 qualification wrote stderr")
for campaign_stderr_path in (
    reducer_k5_stderr_path,
    reducer_k10_stderr_path,
    frontier_50k_stderr_path,
    direct_10m_stderr_path,
    direct_30m_stderr_path,
):
    if campaign_stderr_path.read_text(encoding="utf-8").strip():
        fail("a resident campaign qualification wrote stderr")
if not (k5_finished > k5_started > 0 and k10_finished > k10_started > 0):
    fail("external resident wall-clock measurements are invalid")
for started, finished in (
    (reducer_k5_started, reducer_k5_finished),
    (reducer_k10_started, reducer_k10_finished),
    (frontier_50k_started, frontier_50k_finished),
    (direct_10m_started, direct_10m_finished),
    (direct_30m_started, direct_30m_finished),
):
    if not finished > started > 0:
        fail("a resident campaign wall-clock measurement is invalid")

k5_result = json.loads(k5_path.read_text(encoding="utf-8"))
k10_result = json.loads(k10_path.read_text(encoding="utf-8"))
reducer_k5_result = json.loads(reducer_k5_path.read_text(encoding="utf-8"))
reducer_k10_result = json.loads(reducer_k10_path.read_text(encoding="utf-8"))
frontier_50k_result = json.loads(frontier_50k_path.read_text(encoding="utf-8"))


def direct_scale_command(point_count, timeout_seconds):
    return [
        "/usr/bin/timeout", "--kill-after=5s", "--foreground",
        f"{timeout_seconds}s",
        "/workspace/repository/build/morsehgp3d-cuda-release/"
        "morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification",
        "--point-count", str(point_count), "--family",
        "affine_uniform_binary64", "--maximum-closed-rank", "11",
        "--direct-scale", "--anchor-tile-capacity", "4096", "--seed",
        "1558325537444281125",
    ]


def direct_scale_result(
    *, point_count, timeout_seconds, exit_status, stdout_path, stderr_path,
    started, finished,
):
    if exit_status == 0:
        return json.loads(stdout_path.read_text(encoding="utf-8"))
    if exit_status != 124:
        fail("a direct-scale result has an unauthorized exit status")
    return {
        "backend": "cuda_g4",
        "binary_sha256": digest(frontier_binary_path),
        "claims": {
            "exactness": False,
            "product": False,
            "qualification": False,
            "scalability": False,
            "scientific_result": False,
        },
        "command": direct_scale_command(point_count, timeout_seconds),
        "component_only": True,
        "deployment_status": "profile_only",
        "exit_status": 124,
        "external_wall_finished_epoch_nanoseconds": finished,
        "external_wall_nanoseconds": finished - started,
        "external_wall_started_epoch_nanoseconds": started,
        "family": "affine_uniform_binary64",
        "git_sha": git_sha,
        "internal_timings_available": False,
        "maximum_closed_rank": 11,
        "mode": "device_resident_budgeted_morton_yao48_anchor_tiles_direct_scale",
        "point_count": point_count,
        "profile": "hgp_reduced",
        "profile_only": True,
        "public_status": "not_claimed",
        "qualified": False,
        "run_kind": "direct_scale_timeout",
        "scale_eligible": False,
        "schema": "morsehgp3d.phase15.direct_scale_timeout.v1",
        "status": "timeout",
        "stderr_empty": True,
        "stderr_sha256": digest(stderr_path),
        "stdout_sha256": digest(stdout_path),
        "timed_out": True,
        "timeout_seconds": timeout_seconds,
        "timeout_source": "exit_status_124_with_external_wall_at_least_fixed_bound",
    }


direct_10m_result = direct_scale_result(
    point_count=10_000_000,
    timeout_seconds=direct_10m_timeout_seconds,
    exit_status=direct_10m_exit_status,
    stdout_path=direct_10m_path,
    stderr_path=direct_10m_stderr_path,
    started=direct_10m_started,
    finished=direct_10m_finished,
)
direct_30m_result = direct_scale_result(
    point_count=30_000_000,
    timeout_seconds=direct_30m_timeout_seconds,
    exit_status=direct_30m_exit_status,
    stdout_path=direct_30m_path,
    stderr_path=direct_30m_stderr_path,
    started=direct_30m_started,
    finished=direct_30m_finished,
)
log_paths = {
    "release_build": release_build_path,
    "audit_build": audit_build_path,
    "k5_stdout": k5_path,
    "k5_stderr": k5_stderr_path,
    "k10_stdout": k10_path,
    "k10_stderr": k10_stderr_path,
    "cuobjdump_elf": elf_path,
    "cuobjdump_ptx": ptx_path,
    "cuobjdump_ptx_stderr": ptx_stderr_path,
    "reducer_build": reducer_build_path,
    "frontier_build": frontier_build_path,
    "reducer_k5_stdout": reducer_k5_path,
    "reducer_k5_stderr": reducer_k5_stderr_path,
    "reducer_k10_stdout": reducer_k10_path,
    "reducer_k10_stderr": reducer_k10_stderr_path,
    "frontier_50k_stdout": frontier_50k_path,
    "frontier_50k_stderr": frontier_50k_stderr_path,
    "direct_10m_stdout": direct_10m_path,
    "direct_10m_stderr": direct_10m_stderr_path,
    "direct_30m_stdout": direct_30m_path,
    "direct_30m_stderr": direct_30m_stderr_path,
}
artifact = {
    "architecture": {
        "global_cells_materialized": False,
        "global_cofaces_materialized": False,
        "global_incidences_materialized": False,
        "higher_order_delaunay_materialized": False,
    },
    "artifact_role": "component_qualification_only",
    "backend": "cuda_g4",
    "binary": {
        "elf_architectures": architectures,
        "ptx_embedded": False,
        "relative_path": (
            "build/morsehgp3d-cuda-release/"
            "morsehgp3d_gpu_exact_pair_block_transactional_frontier_"
            "resident_cuda_qualification"
        ),
        "sha256": digest(binary_path),
    },
    "claims": {
        "hierarchy_or_tree": False,
        "min_cluster_size_applied": False,
        "pair_catalog_complete": False,
        "public_scientific_status": False,
        "scalability": False,
        "scale_eligible": False,
        "scientific_exactness_beyond_qualified_scope": False,
        "slo": False,
        "supports_3_4": False,
    },
    "evidence": {
        "cuobjdump_elf": elf_text,
        "cuobjdump_ptx": ptx_text,
        "cuobjdump_ptx_stderr": ptx_stderr_text,
        "k10_stderr": k10_stderr_path.read_text(encoding="utf-8"),
        "k10_stdout": k10_path.read_text(encoding="utf-8"),
        "k5_stderr": k5_stderr_path.read_text(encoding="utf-8"),
        "k5_stdout": k5_path.read_text(encoding="utf-8"),
        "reducer_k5_stdout": reducer_k5_path.read_text(encoding="utf-8"),
        "reducer_k5_stderr": reducer_k5_stderr_path.read_text(encoding="utf-8"),
        "reducer_k10_stdout": reducer_k10_path.read_text(encoding="utf-8"),
        "reducer_k10_stderr": reducer_k10_stderr_path.read_text(encoding="utf-8"),
        "frontier_50k_stdout": frontier_50k_path.read_text(encoding="utf-8"),
        "frontier_50k_stderr": frontier_50k_stderr_path.read_text(encoding="utf-8"),
        "direct_10m_stdout": direct_10m_path.read_text(encoding="utf-8"),
        "direct_10m_stderr": direct_10m_stderr_path.read_text(encoding="utf-8"),
        "direct_30m_stdout": direct_30m_path.read_text(encoding="utf-8"),
        "direct_30m_stderr": direct_30m_stderr_path.read_text(encoding="utf-8"),
    },
    "execution": {
        "campaign_run_order": [
            "resident_scheduler_k5",
            "resident_scheduler_k10",
            "reducer_source_k5",
            "reducer_source_k10",
            "frontier_50k_rank11",
            "direct_scale_10m_rank11",
            "direct_scale_30m_rank11",
        ],
        "external_clock": "guest_host_realtime_nanoseconds",
        "fixed_timeout_command": "/usr/bin/timeout --kill-after=5s --foreground 60s",
        "point_count": 16,
        "run_order": [5, 10],
        "timeouts_seconds": {
            "direct_scale_10m_rank11": direct_10m_timeout_seconds,
            "direct_scale_30m_rank11": direct_30m_timeout_seconds,
            "frontier_50k_rank11": frontier_50k_timeout_seconds,
            "reducer_source_k10": reducer_timeout_seconds,
            "reducer_source_k5": reducer_timeout_seconds,
            "resident_scheduler_k10": 60,
            "resident_scheduler_k5": 60,
        },
        "timeout_seconds_per_run": 60,
    },
    "fixture": "semantic_line_16",
    "git": {"clean": True, "sha": git_sha},
    "image": {"base_ref": base_image_ref, "id": image_id, "ref": image_ref},
    "campaign_binaries": {
        "device_frontier": {
            "relative_path": (
                "build/morsehgp3d-cuda-release/"
                "morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification"
            ),
            "sha256": digest(frontier_binary_path),
        },
        "reducer_source": {
            "relative_path": (
                "build/morsehgp3d-cuda-release/"
                "morsehgp3d_gpu_exact_pair_block_to_reducer_source_cuda_qualification"
            ),
            "sha256": digest(reducer_binary_path),
        },
        "resident_scheduler": {
            "relative_path": (
                "build/morsehgp3d-cuda-release/"
                "morsehgp3d_gpu_exact_pair_block_transactional_frontier_"
                "resident_cuda_qualification"
            ),
            "sha256": digest(binary_path),
        },
    },
    "build_commands": {
        "audit_resident_scheduler": [
            "cmake", "--build", "/workspace/repository/build/morsehgp3d-cuda-audit",
            "--target", "morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_qualification",
            "--parallel", "2",
        ],
        "release_device_frontier": [
            "cmake", "--build", "/workspace/repository/build/morsehgp3d-cuda-release",
            "--target", "morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification",
            "--parallel", "8",
        ],
        "release_reducer_source": [
            "cmake", "--build", "/workspace/repository/build/morsehgp3d-cuda-release",
            "--target", "morsehgp3d_gpu_exact_pair_block_to_reducer_source_cuda_qualification",
            "--parallel", "2",
        ],
        "release_resident_scheduler": [
            "cmake", "--build", "/workspace/repository/build/morsehgp3d-cuda-release",
            "--target", "morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_qualification",
            "--parallel", "2",
        ],
    },
    "log_sha256": {key: digest(path) for key, path in log_paths.items()},
    "mode": "resident_transactional_pair_partition",
    "phase": "15",
    "profile": "hgp_reduced",
    "provenance": {
        "environment_artifact_schema": "morsehgp3d.phase3.qualification.v1",
        "environment_artifact_sha256": digest(environment_path),
    },
    "public_status": "not_claimed",
    "qualified_scope": (
        "bounded_scheduler_conditional_forest_forest_relative_vertical_"
        "target_pipeline_and_device_frontier_components_only"
    ),
    "runs": [
        {
            "command": [
                "/usr/bin/timeout", "--kill-after=5s", "--foreground", "60s",
                "/workspace/repository/build/morsehgp3d-cuda-release/morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_qualification",
                "--point-count", "16", "--K", "5", "--semantic-line-fixture",
                "--require-complete",
            ],
            "external_wall_finished_epoch_nanoseconds": k5_finished,
            "external_wall_nanoseconds": k5_finished - k5_started,
            "external_wall_started_epoch_nanoseconds": k5_started,
            "maximum_order": 5,
            "result": k5_result,
            "timeout_seconds": 60,
        },
        {
            "command": [
                "/usr/bin/timeout", "--kill-after=5s", "--foreground", "60s",
                "/workspace/repository/build/morsehgp3d-cuda-release/morsehgp3d_gpu_exact_pair_block_transactional_frontier_resident_cuda_qualification",
                "--point-count", "16", "--K", "10", "--semantic-line-fixture",
                "--require-complete",
            ],
            "external_wall_finished_epoch_nanoseconds": k10_finished,
            "external_wall_nanoseconds": k10_finished - k10_started,
            "external_wall_started_epoch_nanoseconds": k10_started,
            "maximum_order": 10,
            "result": k10_result,
            "timeout_seconds": 60,
        },
    ],
    "reducer_source_runs": [
        {
            "command": [
                "/usr/bin/timeout", "--kill-after=5s", "--foreground",
                f"{reducer_timeout_seconds}s",
                "/workspace/repository/build/morsehgp3d-cuda-release/morsehgp3d_gpu_exact_pair_block_to_reducer_source_cuda_qualification",
                "--K", "5", "--require-complete",
            ],
            "external_wall_finished_epoch_nanoseconds": reducer_k5_finished,
            "external_wall_nanoseconds": reducer_k5_finished - reducer_k5_started,
            "external_wall_started_epoch_nanoseconds": reducer_k5_started,
            "maximum_order": 5,
            "result": reducer_k5_result,
            "timeout_seconds": reducer_timeout_seconds,
        },
        {
            "command": [
                "/usr/bin/timeout", "--kill-after=5s", "--foreground",
                f"{reducer_timeout_seconds}s",
                "/workspace/repository/build/morsehgp3d-cuda-release/morsehgp3d_gpu_exact_pair_block_to_reducer_source_cuda_qualification",
                "--K", "10", "--require-complete",
            ],
            "external_wall_finished_epoch_nanoseconds": reducer_k10_finished,
            "external_wall_nanoseconds": reducer_k10_finished - reducer_k10_started,
            "external_wall_started_epoch_nanoseconds": reducer_k10_started,
            "maximum_order": 10,
            "result": reducer_k10_result,
            "timeout_seconds": reducer_timeout_seconds,
        },
    ],
    "frontier_profile_runs": [
        {
            "command": [
                "/usr/bin/timeout", "--kill-after=5s", "--foreground",
                f"{frontier_50k_timeout_seconds}s",
                "/workspace/repository/build/morsehgp3d-cuda-release/morsehgp3d_gpu_morton_yao48_device_tiled_pair_frontier_qualification",
                "--point-count", "50000", "--family", "adversarial_mixed_dyadic",
                "--maximum-closed-rank", "11", "--anchor-tile-capacity", "4096",
                "--seed", "1558325537444281125",
            ],
            "external_wall_finished_epoch_nanoseconds": frontier_50k_finished,
            "external_wall_nanoseconds": frontier_50k_finished - frontier_50k_started,
            "external_wall_started_epoch_nanoseconds": frontier_50k_started,
            "exit_status": 0,
            "point_count": 50000,
            "result": frontier_50k_result,
            "run_kind": "component_qualification",
            "timeout_seconds": frontier_50k_timeout_seconds,
        },
        {
            "command": direct_scale_command(
                10_000_000, direct_10m_timeout_seconds
            ),
            "external_wall_finished_epoch_nanoseconds": direct_10m_finished,
            "external_wall_nanoseconds": direct_10m_finished - direct_10m_started,
            "external_wall_started_epoch_nanoseconds": direct_10m_started,
            "exit_status": direct_10m_exit_status,
            "point_count": 10000000,
            "result": direct_10m_result,
            "run_kind": "profile_only_diagnostic",
            "timeout_seconds": direct_10m_timeout_seconds,
        },
        {
            "command": direct_scale_command(
                30_000_000, direct_30m_timeout_seconds
            ),
            "external_wall_finished_epoch_nanoseconds": direct_30m_finished,
            "external_wall_nanoseconds": direct_30m_finished - direct_30m_started,
            "external_wall_started_epoch_nanoseconds": direct_30m_started,
            "exit_status": direct_30m_exit_status,
            "point_count": 30000000,
            "result": direct_30m_result,
            "run_kind": "profile_only_diagnostic",
            "timeout_seconds": direct_30m_timeout_seconds,
        },
    ],
    "schema": "morsehgp3d.phase15.resident_transactional_semantic_qualification.v2",
    "status": "worker_passed_pending_shutdown",
    "vm_lifecycle": {
        "guest_shutdown_guard_verified": True,
        "stop_responsibility": "external_orchestrator",
        "worker_mutates_gcp": False,
    },
}
encoded = json.dumps(
    artifact, ensure_ascii=False, sort_keys=True, separators=(",", ":")
) + "\n"
descriptor = os.open(
    output_path,
    os.O_WRONLY | os.O_CREAT | os.O_EXCL,
    0o600,
)
try:
    with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
        stream.write(encoded)
        stream.flush()
        os.fsync(stream.fileno())
except BaseException:
    try:
        output_path.unlink()
    except OSError:
        pass
    raise
PY
fi

python3 - "${PUBLISH_TEMP}" "${OUTPUT_PATH}" \
    "${PHASE4_PUBLISH_TEMP}" "${PHASE4_OUTPUT_PATH}" \
    "${PHASE5_PUBLISH_TEMP}" "${PHASE5_OUTPUT_PATH}" \
    "${PHASE5_WORK_PROFILE_PUBLISH_TEMP}" \
    "${PHASE5_WORK_PROFILE_OUTPUT_PATH}" \
    "${PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP}" \
    "${PHASE5_EXACT_SEARCH_WORK_PROFILE_OUTPUT_PATH}" \
    "${PHASE7_H_POLYTOPE_PUBLISH_TEMP}" \
    "${PHASE7_H_POLYTOPE_OUTPUT_PATH}" \
    "${PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP}" \
    "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" \
    "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}" \
    "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}" \
    "${PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP}" \
    "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}" \
    "${PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP}" \
    "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}" \
    "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP}" \
    "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}" \
    "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP}" \
    "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}" \
    "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP}" \
    "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}" <<'PY'
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
if sys.argv[13] or sys.argv[14]:
    if not sys.argv[13] or not sys.argv[14]:
        raise SystemExit("incomplete Phase 9 pair-support Phi publication pair")
    pairs.append(
        (Path(sys.argv[13]), Path(sys.argv[14]), "Phase 9 pair-support Phi")
    )
if sys.argv[15] or sys.argv[16]:
    if not sys.argv[15] or not sys.argv[16]:
        raise SystemExit("incomplete Phase 15 exact diametral-Phi publication pair")
    pairs.append(
        (Path(sys.argv[15]), Path(sys.argv[16]), "Phase 15 exact diametral-Phi")
    )
if sys.argv[17] or sys.argv[18]:
    if not sys.argv[17] or not sys.argv[18]:
        raise SystemExit("incomplete Phase 15 device-frontier 50k publication pair")
    pairs.append(
        (Path(sys.argv[17]), Path(sys.argv[18]), "Phase 15 device-frontier 50k")
    )
if sys.argv[19] or sys.argv[20]:
    if not sys.argv[19] or not sys.argv[20]:
        raise SystemExit("incomplete Phase 15 ranked-pair-classifier publication pair")
    pairs.append(
        (
            Path(sys.argv[19]),
            Path(sys.argv[20]),
            "Phase 15 ranked-pair-classifier",
        )
    )
if sys.argv[21] or sys.argv[22]:
    if not sys.argv[21] or not sys.argv[22]:
        raise SystemExit(
            "incomplete Phase 15 device-frontier strict-interior publication pair"
        )
    pairs.append(
        (
            Path(sys.argv[21]),
            Path(sys.argv[22]),
            "Phase 15 device-frontier strict-interior",
        )
    )
if sys.argv[23] or sys.argv[24]:
    if not sys.argv[23] or not sys.argv[24]:
        raise SystemExit(
            "incomplete Phase 15 exact pair-block witness CUDA publication pair"
        )
    pairs.append(
        (
            Path(sys.argv[23]),
            Path(sys.argv[24]),
            "Phase 15 exact pair-block witness CUDA",
        )
    )
if sys.argv[25] or sys.argv[26]:
    if not sys.argv[25] or not sys.argv[26]:
        raise SystemExit(
            "incomplete Phase 15 resident transactional semantic publication pair"
        )
    pairs.append(
        (
            Path(sys.argv[25]),
            Path(sys.argv[26]),
            "Phase 15 resident transactional semantic",
        )
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
if [[ -n "${PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP}"
fi
if [[ -n "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP}" ]]; then
    rm -f -- "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP}"
fi
PUBLISH_TEMP=""
PHASE4_PUBLISH_TEMP=""
PHASE5_PUBLISH_TEMP=""
PHASE5_WORK_PROFILE_PUBLISH_TEMP=""
PHASE5_EXACT_SEARCH_WORK_PROFILE_PUBLISH_TEMP=""
PHASE7_H_POLYTOPE_PUBLISH_TEMP=""
PHASE9_PAIR_SUPPORT_PHI_PUBLISH_TEMP=""
PHASE15_EXACT_DIAMETRAL_PHI_PUBLISH_TEMP=""
PHASE15_DEVICE_FRONTIER_50K_PUBLISH_TEMP=""
PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_PUBLISH_TEMP=""
PHASE15_RANKED_PAIR_CLASSIFIER_PUBLISH_TEMP=""
PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_PUBLISH_TEMP=""
PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_PUBLISH_TEMP=""

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
if [[ -n "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Compagnon pair-support Phi Phase 9 provisoire publié sans remplacement : %s\n' \
        "${PHASE9_PAIR_SUPPORT_PHI_OUTPUT_PATH}"
fi
if [[ -n "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Compagnon exact diametral-Phi Phase 15 provisoire publié sans remplacement : %s\n' \
        "${PHASE15_EXACT_DIAMETRAL_PHI_OUTPUT_PATH}"
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Compagnon device-frontier 50k Phase 15 provisoire publié sans remplacement : %s\n' \
        "${PHASE15_DEVICE_FRONTIER_50K_OUTPUT_PATH}"
fi
if [[ -n "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Compagnon device-frontier strict-interior Phase 15 provisoire publié sans remplacement : %s\n' \
        "${PHASE15_DEVICE_FRONTIER_STRICT_INTERIOR_OUTPUT_PATH}"
fi
if [[ -n "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Compagnon ranked-pair-classifier Phase 15 provisoire publié sans remplacement : %s\n' \
        "${PHASE15_RANKED_PAIR_CLASSIFIER_OUTPUT_PATH}"
fi
if [[ -n "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Compagnon exact pair-block witness CUDA Phase 15 provisoire publié sans remplacement : %s\n' \
        "${PHASE15_EXACT_PAIR_BLOCK_WITNESS_CUDA_OUTPUT_PATH}"
fi
if [[ -n "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}" ]]; then
    printf '[SUCCÈS WORKER] Compagnon resident-transactional-semantic Phase 15 provisoire publié sans remplacement : %s\n' \
        "${PHASE15_RESIDENT_TRANSACTIONAL_SEMANTIC_OUTPUT_PATH}"
fi
printf '[CYCLE DE VIE] Le worker invité ne ferme pas la VM; l’orchestrateur externe doit certifier TERMINATED.\n'
