#!/usr/bin/env bash
set -Eeuo pipefail

readonly GUEST_GUARD_MIN_REMAINING_SECONDS=1800
readonly GUEST_GUARD_MAX_REMAINING_SECONDS=2820
readonly WORK_RESERVE_SECONDS=1800
readonly PROVISION_TOTAL_BUDGET_SECONDS=660
readonly APT_UPDATE_TIMEOUT_SECONDS=180
readonly APT_PLAN_TIMEOUT_SECONDS=30
readonly APT_INSTALL_TIMEOUT_SECONDS=300
readonly CONFIG_TIMEOUT_SECONDS=30
readonly SERVICE_TIMEOUT_SECONDS=60
readonly PROBE_TIMEOUT_SECONDS=5
readonly TIMEOUT_KILL_AFTER_SECONDS=15
readonly PROBE_KILL_AFTER_SECONDS=2
readonly DEADLINE_LAUNCH_SLACK_SECONDS=2
readonly FAILURE_LOG_MAX_LINES=240
readonly FAILURE_LOG_MAX_BYTES=65536
readonly EXPECTED_OS_ID="ubuntu"
readonly EXPECTED_OS_VERSION="22.04"
readonly EXPECTED_ARCHITECTURE="amd64"
readonly EXPECTED_NVIDIA_TOOLKIT_VERSION="1.17.8-1"
readonly DOCKER_CONFIG="/etc/docker/daemon.json"
readonly DOCKER_BIN="/usr/bin/docker"
readonly DOCKERD_BIN="/usr/bin/dockerd"
readonly CONTAINERD_BIN="/usr/bin/containerd"
readonly NVIDIA_CTK_BIN="/usr/bin/nvidia-ctk"
readonly NVIDIA_RUNTIME_BIN="/usr/bin/nvidia-container-runtime"
readonly BUILDX_PLUGIN="/usr/libexec/docker/cli-plugins/docker-buildx"
readonly DOCKER_UNIT_LEXICAL="/lib/systemd/system/docker.service"
readonly DOCKER_UNIT_CANONICAL="/usr/lib/systemd/system/docker.service"
readonly DOCKER_SOCKET_LEXICAL="/lib/systemd/system/docker.socket"
readonly DOCKER_SOCKET_CANONICAL="/usr/lib/systemd/system/docker.socket"
readonly CONTAINERD_UNIT_LEXICAL="/lib/systemd/system/containerd.service"
readonly CONTAINERD_UNIT_CANONICAL="/usr/lib/systemd/system/containerd.service"
readonly MODPROBE_LEXICAL="/sbin/modprobe"
readonly KMOD_LEXICAL="/bin/kmod"
readonly KMOD_CANONICAL="/usr/bin/kmod"
readonly SUDO_BIN="/usr/bin/sudo"
readonly TIMEOUT_BIN="/usr/bin/timeout"
readonly DATE_BIN="/usr/bin/date"
readonly MKTEMP_BIN="/usr/bin/mktemp"
readonly TAIL_BIN="/usr/bin/tail"
readonly RM_BIN="/usr/bin/rm"
readonly CHMOD_BIN="/usr/bin/chmod"
readonly STAT_BIN="/usr/bin/stat"
readonly TEST_BIN="/usr/bin/test"
readonly CAT_BIN="/usr/bin/cat"

ASSUME_YES=0
GCE_DEADLINE_RAW=""
GCE_DEADLINE_EPOCH=0
WORK_DEADLINE_EPOCH=0
PROVISION_DEADLINE_EPOCH=0
GUEST_SHUTDOWN_EPOCH=0
INITIAL_BOOT_ID=""
SESSION_DIR=""
PROVISION_LOG=""
SESSION_CREATED=0
GUARD_CERTIFIED=0
CONFIG_WAS_ABSENT=0
FAILURE_REPORTED=0
IN_DIE=0
DIAGNOSTIC_PATHS_CERTIFIED=0
DEADLINE_ENFORCED=0
FAILED_FIXED_PATH=""
ROOT_TEMP_DIR=""
ROOT_TEMP_ID=""
CONFIG_STAGING=""
CONFIG_STAGING_ID=""

usage() {
    printf '%s\n' \
        'Usage : ./gcp-migration/phase3_remote_docker_provision.sh --yes --gce-deadline-epoch EPOCH' \
        '' \
        "Prépare Docker pour la qualification Phase 3 sur l'invité déjà protégé." \
        "Le script ne pilote aucune ressource GCP, n'ajoute aucun dépôt et" \
        "n'exécute aucun conteneur. Il exige les deux coupe-circuits avant toute" \
        "mutation, installe uniquement docker.io et docker-buildx depuis Ubuntu," \
        "configure le runtime NVIDIA déjà épinglé, puis certifie le daemon."
}

safety_probe() {
    "${TIMEOUT_BIN}" --kill-after="${PROBE_KILL_AFTER_SECONDS}s" \
        "${PROBE_TIMEOUT_SECONDS}s" "$@"
}

report_failure_log() {
    ((FAILURE_REPORTED == 0)) || return 0
    FAILURE_REPORTED=1
    if [[ -z "${PROVISION_LOG}" || ! -f "${PROVISION_LOG}" || -L "${PROVISION_LOG}" ]]; then
        return 0
    fi
    printf '[DIAGNOSTIC docker-provision] %s dernières lignes et %s octets au plus; début.\n' \
        "${FAILURE_LOG_MAX_LINES}" "${FAILURE_LOG_MAX_BYTES}" >&2
    "${TAIL_BIN}" -c "${FAILURE_LOG_MAX_BYTES}" -- "${PROVISION_LOG}" | \
        "${TAIL_BIN}" -n "${FAILURE_LOG_MAX_LINES}" >&2 || true
    printf '%s\n' '[DIAGNOSTIC docker-provision] fin.' >&2
}

collect_failure_diagnostics() {
    ((GUARD_CERTIFIED == 1)) || return 0
    ((DIAGNOSTIC_PATHS_CERTIFIED == 1)) || return 0
    [[ -n "${PROVISION_LOG}" && -f "${PROVISION_LOG}" ]] || return 0
    {
        printf '%s\n' '[ÉTAT FINAL PAQUETS]'
        safety_probe "${SUDO_BIN}" -n -- /usr/bin/dpkg-query -W \
            '-f=${binary:Package}\t${Status}\t${Version}\n' \
            docker.io docker-buildx docker-ce docker-ce-cli containerd containerd.io \
            nvidia-container-toolkit nvidia-container-toolkit-base \
            libnvidia-container-tools libnvidia-container1 2>&1 || true
        printf '%s\n' '[ÉTAT FINAL SERVICES]'
        safety_probe "${SUDO_BIN}" -n -- /usr/bin/systemctl is-enabled docker.service \
            2>&1 || true
        safety_probe "${SUDO_BIN}" -n -- /usr/bin/systemctl is-active docker.service \
            2>&1 || true
        safety_probe "${SUDO_BIN}" -n -- /usr/bin/systemctl is-active containerd.service \
            2>&1 || true
        printf '%s\n' '[JOURNAL DOCKER DU BOOT, 80 LIGNES AU PLUS]'
        safety_probe "${SUDO_BIN}" -n -- /usr/bin/journalctl --unit=docker.service \
            --boot --no-pager --lines=80 2>&1 || true
    } >>"${PROVISION_LOG}" 2>&1
}

die() {
    local message="$*"
    if ((IN_DIE == 0)); then
        IN_DIE=1
        collect_failure_diagnostics || true
        report_failure_log || true
    fi
    printf '[ERREUR] %s\n' "${message}" >&2
    exit 1
}

cleanup() {
    local original_status=$?
    local current_id=""
    trap - EXIT ERR HUP INT TERM
    if [[ -n "${CONFIG_STAGING}" && -n "${CONFIG_STAGING_ID}" ]]; then
        current_id="$(safety_probe "${SUDO_BIN}" -n -- "${STAT_BIN}" -Lc '%d:%i' -- \
            "${CONFIG_STAGING}" 2>/dev/null || true)"
        if [[ "${current_id}" == "${CONFIG_STAGING_ID}" ]]; then
            safety_probe "${SUDO_BIN}" -n -- "${RM_BIN}" -f -- "${CONFIG_STAGING}" \
                >/dev/null 2>&1 || true
        elif [[ -n "${current_id}" ]]; then
            printf '[ATTENTION] Staging Docker remplacé; suppression refusée : %s.\n' \
                "${CONFIG_STAGING}" >&2
        fi
    fi
    if [[ -n "${ROOT_TEMP_DIR}" && -n "${ROOT_TEMP_ID}" ]]; then
        current_id="$(safety_probe "${SUDO_BIN}" -n -- "${STAT_BIN}" -Lc '%d:%i' -- \
            "${ROOT_TEMP_DIR}" 2>/dev/null || true)"
        if [[ "${current_id}" == "${ROOT_TEMP_ID}" ]]; then
            safety_probe "${SUDO_BIN}" -n -- "${RM_BIN}" -rf -- "${ROOT_TEMP_DIR}" \
                >/dev/null 2>&1 || true
        elif [[ -n "${current_id}" ]]; then
            printf '[ATTENTION] Temporaire root Docker remplacé; suppression refusée : %s.\n' \
                "${ROOT_TEMP_DIR}" >&2
        fi
    fi
    if ((SESSION_CREATED == 1)) && [[ -n "${SESSION_DIR}" && -d "${SESSION_DIR}" ]]; then
        case "${SESSION_DIR}" in
            "${TMPDIR:-/tmp}"/morsehgp3d-phase3-docker.*)
                "${RM_BIN}" -rf -- "${SESSION_DIR}" || true
                ;;
            *)
                printf '[ATTENTION] Nettoyage refusé pour un temporaire non canonique : %s\n' \
                    "${SESSION_DIR}" >&2
                ;;
        esac
    fi
    exit "${original_status}"
}

unexpected_error() {
    local status="$1"
    local line="$2"
    trap - ERR
    die "Échec inattendu ligne ${line} (code ${status})."
}

trap cleanup EXIT
trap 'unexpected_error $? $LINENO' ERR
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

while (($# > 0)); do
    case "$1" in
        --yes)
            ASSUME_YES=1
            shift
            ;;
        --gce-deadline-epoch)
            (($# >= 2)) || die "Valeur manquante après --gce-deadline-epoch."
            [[ -z "${GCE_DEADLINE_RAW}" ]] || \
                die "--gce-deadline-epoch ne peut être fourni qu'une fois."
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

((ASSUME_YES == 1)) || \
    die "--yes est obligatoire et atteste la préparation hôte explicitement autorisée dans la session gardée."
[[ "${GCE_DEADLINE_RAW}" =~ ^[0-9]{10}$ ]] || \
    die "--gce-deadline-epoch doit être un epoch UTC positif sur dix chiffres."
GCE_DEADLINE_EPOCH=$((10#${GCE_DEADLINE_RAW}))
WORK_DEADLINE_EPOCH=$((GCE_DEADLINE_EPOCH - WORK_RESERVE_SECONDS))

bootstrap_certify_fixed_executable() {
    local candidate="$1"
    local index=0
    local metadata=""
    local path=""
    local owner_uid=""
    local mode=""
    local -a expected_paths=(/ /usr /usr/bin "${candidate}")

    [[ "${candidate}" == /usr/bin/* && -f "${candidate}" && ! -L "${candidate}" && \
        -x "${candidate}" ]] || return 1
    [[ ! -L / && ! -L /usr && ! -L /usr/bin ]] || return 1
    metadata="$(/usr/bin/stat -Lc '%n|%u|%a' -- \
        / /usr /usr/bin "${candidate}" 2>/dev/null)" || return 1
    while IFS='|' read -r path owner_uid mode; do
        ((index < ${#expected_paths[@]})) || return 1
        [[ "${path}" == "${expected_paths[index]}" && "${owner_uid}" == "0" && \
            "${mode}" =~ ^[0-7]{3,4}$ ]] || return 1
        (((8#${mode} & 8#22) == 0)) || return 1
        index=$((index + 1))
    done <<<"${metadata}"
    ((index == ${#expected_paths[@]}))
}

for bootstrap_path in \
    "${STAT_BIN}" \
    "${TEST_BIN}" \
    "${CAT_BIN}" \
    "${SUDO_BIN}" \
    "${TIMEOUT_BIN}" \
    "${DATE_BIN}" \
    "${MKTEMP_BIN}" \
    "${TAIL_BIN}" \
    "${RM_BIN}" \
    "${CHMOD_BIN}"; do
    bootstrap_certify_fixed_executable "${bootstrap_path}" || \
        die "Chemin bootstrap absent ou non sûr : ${bootstrap_path}."
done

timeout_version="$(LC_ALL=C "${TIMEOUT_BIN}" --version 2>/dev/null)" || \
    die "Impossible d'identifier GNU timeout."
timeout_version="${timeout_version%%$'\n'*}"
[[ "${timeout_version}" == timeout\ \(GNU\ coreutils\)* ]] || \
    die "timeout doit être GNU coreutils avec gestion du groupe de processus et --kill-after."

bounded_probe() {
    local now=0
    local remaining=0
    if ((DEADLINE_ENFORCED == 1)); then
        now="$("${DATE_BIN}" +%s)" || return 125
        [[ "${now}" =~ ^[0-9]+$ ]] || return 125
        remaining=$((PROVISION_DEADLINE_EPOCH - now))
        ((remaining > PROBE_TIMEOUT_SECONDS + PROBE_KILL_AFTER_SECONDS + \
            DEADLINE_LAUNCH_SLACK_SECONDS)) || return 125
    fi
    "${TIMEOUT_BIN}" --kill-after="${PROBE_KILL_AFTER_SECONDS}s" \
        "${PROBE_TIMEOUT_SECONDS}s" "$@"
}

build_path_chain() {
    local candidate="$1"
    local output_name="$2"
    local current=""
    local component=""
    local relative="${candidate#/}"
    local -a components=()
    local -n output_ref="${output_name}"

    [[ "${candidate}" == /* && "${candidate}" != *$'\n'* && \
        "${candidate}" != *$'\r'* && "${candidate}" != *"//"* ]] || return 1
    output_ref=(/)
    IFS='/' read -r -a components <<<"${relative}"
    for component in "${components[@]}"; do
        [[ -n "${component}" && "${component}" != '.' && "${component}" != '..' ]] || return 1
        current="${current}/${component}"
        output_ref+=("${current}")
    done
}

certify_root_owned_path() {
    local candidate="$1"
    local final_kind="$2"
    local require_executable="${3:-0}"
    local index=0
    local last=0
    local metadata=""
    local path=""
    local owner_uid=""
    local mode=""
    local -a expected_paths=()

    build_path_chain "${candidate}" expected_paths || return 1
    last=$((${#expected_paths[@]} - 1))
    for ((index = 0; index <= last; index++)); do
        path="${expected_paths[index]}"
        bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" ! -L "${path}" \
            >/dev/null 2>&1 || return 1
        if ((index < last)) || [[ "${final_kind}" == "directory" ]]; then
            bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -d "${path}" \
                >/dev/null 2>&1 || return 1
        else
            bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -f "${path}" \
                >/dev/null 2>&1 || return 1
        fi
    done
    if ((require_executable == 1)); then
        bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -x "${candidate}" \
            >/dev/null 2>&1 || return 1
    fi
    metadata="$(bounded_probe "${SUDO_BIN}" -n -- "${STAT_BIN}" -Lc '%n|%u|%a' -- \
        "${expected_paths[@]}" 2>/dev/null)" || return 1
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

certify_fixed_executable() {
    certify_root_owned_path "$1" file 1
}

certify_fixed_executables() {
    (($# > 0)) || return 1
    local candidate=""
    local index=0
    local metadata=""
    local mode=""
    local owner_uid=""
    local path=""
    local -a expected_paths=(/ /usr /usr/bin)

    FAILED_FIXED_PATH="shared /usr/bin prefix"
    for path in / /usr /usr/bin; do
        bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" ! -L "${path}" \
            >/dev/null 2>&1 || return 1
        bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -d "${path}" \
            >/dev/null 2>&1 || return 1
    done

    for candidate in "$@"; do
        FAILED_FIXED_PATH="${candidate}"
        [[ "${candidate}" == /usr/bin/* && "${candidate}" != *$'\n'* && \
            "${candidate}" != *$'\r'* && "${candidate}" != *"//"* ]] || return 1
        bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" ! -L "${candidate}" \
            >/dev/null 2>&1 || return 1
        bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -f "${candidate}" \
            >/dev/null 2>&1 || return 1
        bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -x "${candidate}" \
            >/dev/null 2>&1 || return 1
        expected_paths+=("${candidate}")
    done

    FAILED_FIXED_PATH="batched /usr/bin metadata"
    metadata="$(bounded_probe "${SUDO_BIN}" -n -- "${STAT_BIN}" -Lc '%n|%u|%a' -- \
        "${expected_paths[@]}" 2>/dev/null)" || return 1
    while IFS='|' read -r path owner_uid mode; do
        ((index < ${#expected_paths[@]})) || return 1
        FAILED_FIXED_PATH="${path:-batched /usr/bin metadata}"
        [[ "${path}" == "${expected_paths[index]}" && "${owner_uid}" == "0" && \
            "${mode}" =~ ^[0-7]{3,4}$ ]] || return 1
        (((8#${mode} & 8#22) == 0)) || return 1
        index=$((index + 1))
    done <<<"${metadata}"
    ((index == ${#expected_paths[@]})) || return 1
    FAILED_FIXED_PATH=""
}

read_guest_guard() {
    local evidence=""
    local line=""
    local mode=""
    local mode_count=0
    local scheduled_usec=""
    local usec_count=0
    local scheduled_epoch=0
    local now_epoch=0
    local remaining_seconds=0

    evidence="$(bounded_probe "${SUDO_BIN}" -n -- "${CAT_BIN}" \
        /run/systemd/shutdown/scheduled 2>/dev/null)" || return 1
    while IFS= read -r line; do
        case "${line}" in
            MODE=*)
                mode="${line#MODE=}"
                mode_count=$((mode_count + 1))
                ;;
            USEC=*)
                scheduled_usec="${line#USEC=}"
                usec_count=$((usec_count + 1))
                ;;
        esac
    done <<<"${evidence}"
    ((mode_count == 1 && usec_count == 1)) || return 1
    [[ "${mode}" == "poweroff" && "${scheduled_usec}" =~ ^[0-9]{1,18}$ ]] || return 1
    scheduled_epoch=$((10#${scheduled_usec} / 1000000))
    now_epoch="$("${DATE_BIN}" +%s)" || return 1
    [[ "${now_epoch}" =~ ^[0-9]+$ ]] || return 1
    remaining_seconds=$((scheduled_epoch - now_epoch))
    ((remaining_seconds >= GUEST_GUARD_MIN_REMAINING_SECONDS)) || return 1
    ((remaining_seconds <= GUEST_GUARD_MAX_REMAINING_SECONDS)) || return 1
    ((scheduled_epoch <= GCE_DEADLINE_EPOCH)) || return 1
    GUEST_SHUTDOWN_EPOCH="${scheduled_epoch}"
}

read_guest_guard || \
    die "Arrêt invité poweroff absent, illisible ou hors borne; aucune préparation Docker n'a été lancée."
GUARD_CERTIFIED=1
PROVISION_DEADLINE_EPOCH="${WORK_DEADLINE_EPOCH}"
guest_reserve_deadline=$((GUEST_SHUTDOWN_EPOCH - WORK_RESERVE_SECONDS))
if ((guest_reserve_deadline < PROVISION_DEADLINE_EPOCH)); then
    PROVISION_DEADLINE_EPOCH="${guest_reserve_deadline}"
fi
now_epoch="$("${DATE_BIN}" +%s)" || die "Horloge invitée illisible après certification de la garde."
[[ "${now_epoch}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique."
((PROVISION_DEADLINE_EPOCH > now_epoch)) || \
    die "La deadline de préparation GCE/invitée est déjà atteinte."
DEADLINE_ENFORCED=1
printf '[GARDE DOCKER] arrêt invité=%s, échéance GCE sûre=%s, deadline préparation=%s.\n' \
    "${GUEST_SHUTDOWN_EPOCH}" "${GCE_DEADLINE_EPOCH}" "${PROVISION_DEADLINE_EPOCH}"

SESSION_DIR="$("${MKTEMP_BIN}" -d \
    "${TMPDIR:-/tmp}/morsehgp3d-phase3-docker.XXXXXXXX")" || \
    die "Impossible de créer le temporaire borné de préparation Docker."
SESSION_CREATED=1
PROVISION_LOG="${SESSION_DIR}/provision.log"
: >"${PROVISION_LOG}"
"${CHMOD_BIN}" 600 "${PROVISION_LOG}"

run_bounded() {
    local label="$1"
    local maximum_seconds="$2"
    shift 2
    local now=0
    local remaining=0
    local required=0
    now="$("${DATE_BIN}" +%s)" || die "Horloge invitée illisible avant ${label}."
    [[ "${now}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique avant ${label}."
    remaining=$((PROVISION_DEADLINE_EPOCH - now))
    required=$((maximum_seconds + TIMEOUT_KILL_AFTER_SECONDS + \
        DEADLINE_LAUNCH_SLACK_SECONDS))
    ((remaining > required)) || \
        die "Deadline atteinte : ${label} réserve ${required}s, reste ${remaining}s; commande non lancée."
    printf '[UNITÉ] %s, timeout=%ss, kill-after=%ss, marge=%ss.\n' \
        "${label}" "${maximum_seconds}" "${TIMEOUT_KILL_AFTER_SECONDS}" "${remaining}"
    printf '[UNITÉ] %s, timeout=%ss, kill-after=%ss, marge=%ss.\n' \
        "${label}" "${maximum_seconds}" "${TIMEOUT_KILL_AFTER_SECONDS}" "${remaining}" \
        >>"${PROVISION_LOG}"
    "${TIMEOUT_BIN}" --kill-after="${TIMEOUT_KILL_AFTER_SECONDS}s" \
        "${maximum_seconds}s" "$@" >>"${PROVISION_LOG}" 2>&1
}

run_bounded_to_file() {
    local label="$1"
    local maximum_seconds="$2"
    local output_file="$3"
    shift 3
    local now=0
    local remaining=0
    local required=0
    now="$("${DATE_BIN}" +%s)" || die "Horloge invitée illisible avant ${label}."
    [[ "${now}" =~ ^[0-9]+$ ]] || die "Horloge invitée non numérique avant ${label}."
    remaining=$((PROVISION_DEADLINE_EPOCH - now))
    required=$((maximum_seconds + TIMEOUT_KILL_AFTER_SECONDS + \
        DEADLINE_LAUNCH_SLACK_SECONDS))
    ((remaining > required)) || \
        die "Deadline atteinte : ${label} réserve ${required}s, reste ${remaining}s; commande non lancée."
    printf '[UNITÉ] %s, timeout=%ss, kill-after=%ss, marge=%ss.\n' \
        "${label}" "${maximum_seconds}" "${TIMEOUT_KILL_AFTER_SECONDS}" "${remaining}"
    printf '[UNITÉ] %s, timeout=%ss, kill-after=%ss, marge=%ss.\n' \
        "${label}" "${maximum_seconds}" "${TIMEOUT_KILL_AFTER_SECONDS}" "${remaining}" \
        >>"${PROVISION_LOG}"
    : >"${output_file}"
    "${CHMOD_BIN}" 600 "${output_file}"
    "${TIMEOUT_BIN}" --kill-after="${TIMEOUT_KILL_AFTER_SECONDS}s" \
        "${maximum_seconds}s" "$@" >"${output_file}" 2>&1
}

read_fixed_file() {
    bounded_probe "${SUDO_BIN}" -n -- "${CAT_BIN}" "$1"
}

declare -a required_paths=(
    /usr/bin/apt-cache
    /usr/bin/apt-get
    /usr/bin/dpkg
    /usr/bin/dpkg-query
    /usr/bin/env
    /usr/bin/install
    /usr/bin/journalctl
    /usr/bin/ln
    /usr/bin/python3
    /usr/bin/readlink
    /usr/bin/systemctl
    "${NVIDIA_CTK_BIN}"
    "${NVIDIA_RUNTIME_BIN}"
)
certify_fixed_executables "${required_paths[@]}" || \
    die "Chemin système absent ou non sûr : ${FAILED_FIXED_PATH:-groupe /usr/bin}."
DIAGNOSTIC_PATHS_CERTIFIED=1

INITIAL_BOOT_ID="$(read_fixed_file /proc/sys/kernel/random/boot_id 2>/dev/null)" || \
    die "Boot ID initial illisible."
[[ "${INITIAL_BOOT_ID}" =~ ^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$ ]] || \
    die "Boot ID initial non canonique."

os_release="$(read_fixed_file /etc/os-release 2>/dev/null)" || die "/etc/os-release illisible."
os_id=""
os_version=""
while IFS= read -r line; do
    case "${line}" in
        ID=*)
            os_id="${line#ID=}"
            os_id="${os_id%\"}"
            os_id="${os_id#\"}"
            ;;
        VERSION_ID=*)
            os_version="${line#VERSION_ID=}"
            os_version="${os_version%\"}"
            os_version="${os_version#\"}"
            ;;
    esac
done <<<"${os_release}"
[[ "${os_id}" == "${EXPECTED_OS_ID}" && "${os_version}" == "${EXPECTED_OS_VERSION}" ]] || \
    die "OS refusé : ${os_id:-inconnu} ${os_version:-inconnue}; Ubuntu 22.04 requis."
architecture="$(bounded_probe "${SUDO_BIN}" -n -- /usr/bin/dpkg --print-architecture \
    2>/dev/null)" || die "Architecture dpkg illisible."
[[ "${architecture}" == "${EXPECTED_ARCHITECTURE}" ]] || \
    die "Architecture refusée : ${architecture:-inconnue}; amd64 requis."

package_record() {
    local package="$1"
    bounded_probe "${SUDO_BIN}" -n -- /usr/bin/dpkg-query -W \
        '-f=${Status}\t${Version}\n' "${package}" 2>/dev/null
}

installed_package_version() {
    local package="$1"
    local record=""
    local status=""
    local version=""
    record="$(package_record "${package}")" || return 1
    IFS=$'\t' read -r status version <<<"${record}"
    [[ "${status}" =~ ^(install|hold)\ ok\ installed$ && -n "${version}" ]] || return 1
    printf '%s\n' "${version}"
}

reject_partial_package() {
    local package="$1"
    local record=""
    local status=""
    local version=""
    if record="$(package_record "${package}")"; then
        IFS=$'\t' read -r status version <<<"${record}"
        [[ "${status}" =~ ^(install|hold)\ ok\ installed$ && -n "${version}" ]] || \
            die "État dpkg partiel ou ambigu pour ${package}: ${record}."
    fi
}

verify_nvidia_package_stack() {
    local package=""
    local actual=""
    for package in \
        nvidia-container-toolkit \
        nvidia-container-toolkit-base \
        libnvidia-container-tools \
        libnvidia-container1; do
        actual="$(installed_package_version "${package}" 2>/dev/null || true)"
        [[ "${actual}" == "${EXPECTED_NVIDIA_TOOLKIT_VERSION}" ]] || \
            die "${package} doit valoir exactement ${EXPECTED_NVIDIA_TOOLKIT_VERSION} (reçu=${actual:-absent})."
    done
}

verify_nvidia_package_stack

for competing_package in docker-ce docker-ce-cli containerd.io podman-docker moby-engine; do
    reject_partial_package "${competing_package}"
    if installed_package_version "${competing_package}" >/dev/null 2>&1; then
        die "Famille Docker concurrente installée : ${competing_package}; aucune fusion automatique."
    fi
done
for expected_package in docker.io docker-buildx containerd; do
    reject_partial_package "${expected_package}"
done

dpkg_audit="$(bounded_probe "${SUDO_BIN}" -n -- /usr/bin/dpkg --audit 2>&1)" || \
    die "dpkg --audit a échoué avant préparation."
[[ -z "${dpkg_audit}" ]] || die "État dpkg incohérent avant préparation : ${dpkg_audit}."

certify_docker_directory_state() {
    certify_root_owned_path /etc directory || return 2
    if bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -L /etc/docker \
        >/dev/null 2>&1; then
        return 2
    fi
    if bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -e /etc/docker \
        >/dev/null 2>&1; then
        certify_root_owned_path /etc/docker directory || return 2
        return 0
    fi
    return 1
}

docker_directory_state=0
if certify_docker_directory_state; then
    docker_directory_state=1
else
    docker_directory_status=$?
    ((docker_directory_status == 1)) || \
        die "/etc/docker existe mais n'est pas un répertoire root sûr et non symbolique."
fi

strict_validate_daemon_json() {
    local path="$1"
    bounded_probe "${SUDO_BIN}" -n -- /usr/bin/python3 -I -S - "${path}" <<'PY'
import json
import sys

class DuplicateKey(ValueError):
    pass

def strict_object(pairs):
    value = {}
    for key, item in pairs:
        if key in value:
            raise DuplicateKey(f"duplicate JSON key: {key}")
        value[key] = item
    return value

with open(sys.argv[1], "r", encoding="utf-8") as stream:
    value = json.load(stream, object_pairs_hook=strict_object)
if not isinstance(value, dict) or set(value) != {"runtimes"}:
    raise SystemExit("daemon.json contains unsupported top-level settings")
runtimes = value.get("runtimes")
if not isinstance(runtimes, dict) or set(runtimes) != {"nvidia"}:
    raise SystemExit("daemon.json must contain only the NVIDIA runtime")
nvidia = runtimes.get("nvidia")
if not isinstance(nvidia, dict) or set(nvidia) != {"args", "path"}:
    raise SystemExit("invalid NVIDIA runtime object")
if nvidia.get("args") != []:
    raise SystemExit("the NVIDIA runtime args must be empty")
if nvidia.get("path") != "/usr/bin/nvidia-container-runtime":
    raise SystemExit("the NVIDIA runtime path must be absolute and fixed")
PY
}

daemon_json_file_is_approved() {
    local path="$1"
    local config_size=""

    certify_root_owned_path "${path}" file || return 1
    config_size="$(bounded_probe "${SUDO_BIN}" -n -- "${STAT_BIN}" -Lc '%s' -- \
        "${path}" 2>/dev/null)" || return 1
    [[ "${config_size}" =~ ^[0-9]+$ ]] || return 1
    ((config_size > 0 && config_size <= FAILURE_LOG_MAX_BYTES)) || return 1
    strict_validate_daemon_json "${path}" >/dev/null 2>&1
}

daemon_config_is_approved() {
    if bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -L "${DOCKER_CONFIG}" \
        >/dev/null 2>&1; then
        return 2
    fi
    if ! bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -e "${DOCKER_CONFIG}" \
        >/dev/null 2>&1; then
        return 1
    fi
    daemon_json_file_is_approved "${DOCKER_CONFIG}" || return 2
}

config_state=0
if daemon_config_is_approved; then
    config_state=1
else
    config_status=$?
    if ((config_status == 1)); then
        CONFIG_WAS_ABSENT=1
    else
        die "${DOCKER_CONFIG} existe mais sa chaîne de chemins ou son JSON strict est ambigu."
    fi
fi

refuse_systemd_shadow_paths() {
    local path=""
    certify_root_owned_path /etc/systemd/system directory || return 1
    certify_root_owned_path /run/systemd/system directory || return 1
    for path in \
        /etc/systemd/system/docker.service \
        /etc/systemd/system/docker.service.d \
        /etc/systemd/system/docker.socket \
        /etc/systemd/system/docker.socket.d \
        /etc/systemd/system/containerd.service \
        /etc/systemd/system/containerd.service.d \
        /run/systemd/system/docker.service \
        /run/systemd/system/docker.service.d \
        /run/systemd/system/docker.socket \
        /run/systemd/system/docker.socket.d \
        /run/systemd/system/containerd.service \
        /run/systemd/system/containerd.service.d; do
        if bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -L "${path}" \
            >/dev/null 2>&1 || \
            bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -e "${path}" \
                >/dev/null 2>&1; then
            return 1
        fi
    done
}

refuse_systemd_shadow_paths || \
    die "Override, drop-in ou symlink systemd Docker/containerd refusé avant APT."

package_owns_path() {
    local package="$1"
    local path="$2"
    local ownership=""
    ownership="$(bounded_probe "${SUDO_BIN}" -n -- /usr/bin/dpkg-query -S \
        "${path}" 2>/dev/null)" || return 1
    [[ "${ownership}" == "${package}: ${path}" ]]
}

verify_systemd_unit() {
    local unit="$1"
    local role="$2"
    local lexical_path="$3"
    local canonical_path="$4"
    local package="$5"
    local canonical_readback=""
    local properties=""

    certify_root_owned_path "${canonical_path}" file || return 1
    canonical_readback="$(bounded_probe "${SUDO_BIN}" -n -- /usr/bin/readlink -f -- \
        "${lexical_path}" 2>/dev/null)" || return 1
    [[ "${canonical_readback}" == "${canonical_path}" ]] || return 1
    package_owns_path "${package}" "${lexical_path}" || return 1

    if [[ "${role}" == "socket" ]]; then
        properties="$(bounded_probe "${SUDO_BIN}" -n -- /usr/bin/systemctl show \
            "${unit}" --no-pager \
            --property=LoadState \
            --property=FragmentPath \
            --property=DropInPaths \
            --property=Listen \
            --property=SocketUser \
            --property=SocketGroup \
            --property=SocketMode 2>/dev/null)" || return 1
    else
        properties="$(bounded_probe "${SUDO_BIN}" -n -- /usr/bin/systemctl show \
            "${unit}" --no-pager \
            --property=LoadState \
            --property=FragmentPath \
            --property=DropInPaths \
            --property=ExecStart \
            --property=ExecStartPre \
            --property=ExecStartPost \
            --property=ExecCondition \
            --property=ExecStop \
            --property=ExecStopPost \
            --property=Environment \
            --property=EnvironmentFiles 2>/dev/null)" || return 1
    fi

    bounded_probe /usr/bin/python3 -I -S - "${role}" "${lexical_path}" \
        "${canonical_path}" "${properties}" <<'PY'
import re
import sys

role, lexical, canonical, raw = sys.argv[1:]
values = {}
for line in raw.splitlines():
    if "=" not in line:
        raise SystemExit("malformed systemctl show record")
    key, value = line.split("=", 1)
    if key in values:
        raise SystemExit("duplicate systemctl property")
    values[key] = value

common = {"LoadState", "FragmentPath", "DropInPaths"}
if role == "socket":
    expected = common | {"Listen", "SocketUser", "SocketGroup", "SocketMode"}
else:
    expected = common | {
        "ExecStart", "ExecStartPre", "ExecStartPost", "ExecCondition", "ExecStop",
        "ExecStopPost", "Environment", "EnvironmentFiles"
    }
if set(values) != expected:
    raise SystemExit("missing or unexpected systemctl property")
if values["LoadState"] != "loaded":
    raise SystemExit("unit is not loaded")
if values["FragmentPath"] not in {lexical, canonical}:
    raise SystemExit("unexpected unit fragment")
if values["DropInPaths"]:
    raise SystemExit("systemd drop-ins are forbidden")

if role == "socket":
    if values["Listen"] != "/run/docker.sock (Stream)":
        raise SystemExit("unexpected Docker socket listener")
    if values["SocketUser"] != "root" or values["SocketGroup"] != "docker":
        raise SystemExit("unexpected Docker socket identity")
    if values["SocketMode"] not in {"0660", "660"}:
        raise SystemExit("unexpected Docker socket mode")
    raise SystemExit(0)

if (
    values["ExecStartPost"]
    or values["ExecCondition"]
    or values["ExecStop"]
    or values["ExecStopPost"]
    or values["Environment"]
    or values["EnvironmentFiles"]
):
    raise SystemExit("unexpected systemd auxiliary execution or environment")

def execution(value):
    records = re.findall(r"\{\s*(.*?)\s*\}", value)
    if len(records) != 1:
        raise SystemExit("expected one systemd execution record")
    fields = {}
    for item in records[0].split(";"):
        item = item.strip()
        if not item:
            continue
        if "=" not in item:
            raise SystemExit("malformed execution field")
        key, field_value = item.split("=", 1)
        key = key.strip()
        if key in fields:
            raise SystemExit("duplicate execution field")
        fields[key] = field_value.strip()
    return fields

start = execution(values["ExecStart"])
if role == "docker":
    if values["ExecStartPre"]:
        raise SystemExit("unexpected Docker ExecStartPre")
    if start.get("path") != "/usr/bin/dockerd":
        raise SystemExit("unexpected dockerd path")
    if start.get("argv[]") != "/usr/bin/dockerd -H fd:// --containerd=/run/containerd/containerd.sock":
        raise SystemExit("unexpected dockerd argv")
elif role == "containerd":
    if start.get("path") != "/usr/bin/containerd" or start.get("argv[]") != "/usr/bin/containerd":
        raise SystemExit("unexpected containerd command")
    if values["ExecStartPre"]:
        pre = execution(values["ExecStartPre"])
        if pre.get("path") != "/sbin/modprobe":
            raise SystemExit("unexpected containerd ExecStartPre path")
        if pre.get("argv[]") != "/sbin/modprobe overlay":
            raise SystemExit("unexpected containerd ExecStartPre argv")
        if pre.get("ignore_errors") != "yes":
            raise SystemExit("containerd modprobe must be optional")
else:
    raise SystemExit("unknown unit role")
PY
}

certify_systemd_stack() {
    local kmod_readback=""
    local modprobe_readback=""

    certify_fixed_executable "${CONTAINERD_BIN}" || return 1
    certify_fixed_executable "${KMOD_CANONICAL}" || return 1
    kmod_readback="$(bounded_probe "${SUDO_BIN}" -n -- /usr/bin/readlink -f -- \
        "${KMOD_LEXICAL}" 2>/dev/null)" || return 1
    modprobe_readback="$(bounded_probe "${SUDO_BIN}" -n -- /usr/bin/readlink -f -- \
        "${MODPROBE_LEXICAL}" 2>/dev/null)" || return 1
    [[ "${kmod_readback}" == "${KMOD_CANONICAL}" && \
        "${modprobe_readback}" == "${KMOD_CANONICAL}" ]] || return 1
    package_owns_path kmod "${KMOD_LEXICAL}" || return 1
    package_owns_path kmod "${MODPROBE_LEXICAL}" || return 1
    verify_systemd_unit docker.service docker "${DOCKER_UNIT_LEXICAL}" \
        "${DOCKER_UNIT_CANONICAL}" docker.io || return 1
    verify_systemd_unit docker.socket socket "${DOCKER_SOCKET_LEXICAL}" \
        "${DOCKER_SOCKET_CANONICAL}" docker.io || return 1
    verify_systemd_unit containerd.service containerd "${CONTAINERD_UNIT_LEXICAL}" \
        "${CONTAINERD_UNIT_CANONICAL}" containerd || return 1
}

certify_buildx_plugin() {
    certify_fixed_executable "${BUILDX_PLUGIN}" || return 1
    package_owns_path docker-buildx "${BUILDX_PLUGIN}" || return 1
    bounded_probe "${BUILDX_PLUGIN}" version >/dev/null 2>&1
}

docker_version="$(installed_package_version docker.io 2>/dev/null || true)"
buildx_version="$(installed_package_version docker-buildx 2>/dev/null || true)"
containerd_version="$(installed_package_version containerd 2>/dev/null || true)"
if [[ -z "${docker_version}" ]] && \
    bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -e "${DOCKER_BIN}" \
        >/dev/null 2>&1; then
    die "${DOCKER_BIN} existe sans paquet docker.io certifié."
fi
if [[ -n "${docker_version}" ]]; then
    certify_fixed_executable "${DOCKER_BIN}" || \
        die "Le paquet docker.io est installé mais ${DOCKER_BIN} n'est pas sûr."
    certify_fixed_executable "${DOCKERD_BIN}" || \
        die "Le paquet docker.io est installé mais ${DOCKERD_BIN} n'est pas sûr."
fi

docker_runtime_is_ready() {
    local runtime_json=""
    [[ -n "${docker_version}" && -n "${buildx_version}" && "${config_state}" == "1" ]] || \
        return 1
    certify_buildx_plugin || return 1
    certify_systemd_stack || return 1
    bounded_probe "${SUDO_BIN}" -n -- /usr/bin/systemctl is-enabled --quiet docker.service \
        >/dev/null 2>&1 || return 1
    bounded_probe "${SUDO_BIN}" -n -- /usr/bin/systemctl is-active --quiet docker.service \
        >/dev/null 2>&1 || return 1
    bounded_probe "${SUDO_BIN}" -n -- /usr/bin/systemctl is-active --quiet containerd.service \
        >/dev/null 2>&1 || return 1
    bounded_probe "${SUDO_BIN}" -n -- "${DOCKER_BIN}" --version >/dev/null 2>&1 || return 1
    runtime_json="$(bounded_probe "${SUDO_BIN}" -n -- "${DOCKER_BIN}" info \
        --format '{{json .Runtimes}}' 2>/dev/null)" || return 1
    bounded_probe /usr/bin/python3 -I -S - "${runtime_json}" <<'PY'
import json
import sys

value = json.loads(sys.argv[1])
if not isinstance(value, dict):
    raise SystemExit("Docker runtimes are not an object")
nvidia = value.get("nvidia")
if not isinstance(nvidia, dict):
    raise SystemExit("Docker does not expose the NVIDIA runtime")
if nvidia.get("path") != "/usr/bin/nvidia-container-runtime":
    raise SystemExit("Docker exposes an unexpected NVIDIA runtime path")
if nvidia.get("runtimeArgs") != []:
    raise SystemExit("Docker exposes unexpected NVIDIA runtime arguments")
PY
}

verify_postconditions() {
    local final_boot_id=""
    local runtime_json=""

    verify_nvidia_package_stack
    read_guest_guard || die "La garde invitée n'est plus certifiable après la préparation Docker."
    final_boot_id="$(read_fixed_file /proc/sys/kernel/random/boot_id 2>/dev/null)" || \
        die "Boot ID final illisible."
    [[ "${final_boot_id}" == "${INITIAL_BOOT_ID}" ]] || \
        die "Le boot ID a changé pendant la préparation Docker; benchmark refusé."
    refuse_systemd_shadow_paths || \
        die "Un override systemd est apparu pendant la préparation Docker."
    daemon_config_is_approved || die "Le daemon.json final n'est plus strictement approuvé."
    docker_runtime_is_ready || \
        die "Docker, buildx, containerd, leurs unités ou le runtime NVIDIA ne sont pas certifiés."
    runtime_json="$(bounded_probe "${SUDO_BIN}" -n -- "${DOCKER_BIN}" info \
        --format '{{json .Runtimes}}' 2>/dev/null)" || die "Runtimes Docker illisibles."
    printf '[DOCKER PRÊT] docker.io=%s, docker-buildx=%s, toolkit=%s, runtimes=%s.\n' \
        "${docker_version}" "${buildx_version}" "${EXPECTED_NVIDIA_TOOLKIT_VERSION}" \
        "${runtime_json}"
}

if docker_runtime_is_ready; then
    printf '%s\n' '[DOCKER PRÊT] Installation, configuration et services déjà certifiés; aucune mutation hôte.'
    verify_postconditions
    exit 0
fi

now_epoch="$("${DATE_BIN}" +%s)" || die "Horloge invitée illisible avant l'enveloppe de préparation."
remaining_seconds=$((PROVISION_DEADLINE_EPOCH - now_epoch))
((remaining_seconds > PROVISION_TOTAL_BUDGET_SECONDS)) || \
    die "Fenêtre insuffisante pour une préparation Docker bornée : reste=${remaining_seconds}s, requis>${PROVISION_TOTAL_BUDGET_SECONDS}s."

declare -a missing_packages=()
declare -a package_specs=()
declare -A expected_versions=()
[[ -n "${docker_version}" ]] || missing_packages+=(docker.io)
[[ -n "${buildx_version}" ]] || missing_packages+=(docker-buildx)

verify_ubuntu_madison_origin() {
    local package="$1"
    local candidate="$2"
    local source_file="$3"
    bounded_probe /usr/bin/python3 -I -S - "${package}" "${candidate}" "${source_file}" <<'PY'
from pathlib import Path
import sys
from urllib.parse import urlparse

package, candidate, source_path = sys.argv[1:]
matched = 0
allowed_hosts = {"archive.ubuntu.com", "security.ubuntu.com", "snapshot.ubuntu.com"}
allowed_suites = {"jammy", "jammy-updates", "jammy-security"}
for raw in Path(source_path).read_text(encoding="utf-8").splitlines():
    fields = [field.strip() for field in raw.split("|")]
    if len(fields) != 3 or fields[0] != package or fields[1] != candidate:
        continue
    matched += 1
    tokens = fields[2].split()
    if len(tokens) != 4 or tokens[3] != "Packages":
        raise SystemExit("ambiguous apt-cache madison source")
    parsed = urlparse(tokens[0])
    if parsed.scheme not in {"http", "https"} or not parsed.hostname:
        raise SystemExit("APT source URL is not HTTP(S)")
    host = parsed.hostname.lower()
    if host not in allowed_hosts and not host.endswith(".archive.ubuntu.com"):
        raise SystemExit("APT candidate is not from an allowed Ubuntu host")
    if parsed.path.rstrip("/") != "/ubuntu":
        raise SystemExit("APT candidate has an unexpected archive path")
    suite_component = tokens[1].split("/", 1)
    if len(suite_component) != 2 or suite_component[0] not in allowed_suites:
        raise SystemExit("APT candidate has an unexpected Ubuntu suite")
    if suite_component[1] != "universe" or tokens[2] != "amd64":
        raise SystemExit("APT candidate has an unexpected component or architecture")
if matched == 0:
    raise SystemExit("APT candidate has no exact madison provenance record")
PY
}

if ((${#missing_packages[@]} > 0)); then
    if [[ -z "${docker_version}" ]]; then
        for unit_path in \
            "${DOCKER_UNIT_LEXICAL}" \
            "${DOCKER_SOCKET_LEXICAL}"; do
            if bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -L "${unit_path}" \
                >/dev/null 2>&1 || \
                bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -e "${unit_path}" \
                    >/dev/null 2>&1; then
                die "Un fragment systemd Docker existe sans paquet docker.io certifié : ${unit_path}."
            fi
        done
    fi
    if [[ -z "${containerd_version}" ]]; then
        for unit_path in \
            "${CONTAINERD_UNIT_LEXICAL}" \
            "${CONTAINERD_UNIT_CANONICAL}"; do
            if bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -L "${unit_path}" \
                >/dev/null 2>&1 || \
                bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -e "${unit_path}" \
                    >/dev/null 2>&1; then
                die "Un fragment systemd containerd existe sans paquet containerd certifié : ${unit_path}."
            fi
        done
    fi

    run_bounded "apt-update" "${APT_UPDATE_TIMEOUT_SECONDS}" \
        "${SUDO_BIN}" -n -- /usr/bin/env LC_ALL=C DEBIAN_FRONTEND=noninteractive \
        /usr/bin/apt-get \
        -o DPkg::Lock::Timeout=30 \
        -o Acquire::Retries=3 \
        -o Acquire::http::Timeout=30 \
        -o Acquire::https::Timeout=30 \
        update || die "apt-get update borné a échoué."

    for package in "${missing_packages[@]}"; do
        policy_log="${SESSION_DIR}/apt-policy-${package}.log"
        madison_log="${SESSION_DIR}/apt-madison-${package}.log"
        run_bounded_to_file "apt-policy-${package}" "${APT_PLAN_TIMEOUT_SECONDS}" \
            "${policy_log}" "${SUDO_BIN}" -n -- /usr/bin/env LC_ALL=C \
            /usr/bin/apt-cache policy "${package}" || \
            die "Candidat APT illisible pour ${package}."
        policy="$(<"${policy_log}")"
        "${TAIL_BIN}" -n "${FAILURE_LOG_MAX_LINES}" "${policy_log}" >>"${PROVISION_LOG}"
        candidate=""
        candidate_count=0
        while IFS= read -r line; do
            case "${line}" in
                *Candidate:*)
                    candidate="${line#*Candidate:}"
                    candidate="${candidate#${candidate%%[![:space:]]*}}"
                    candidate="${candidate%${candidate##*[![:space:]]}}"
                    candidate_count=$((candidate_count + 1))
                    ;;
            esac
        done <<<"${policy}"
        ((candidate_count == 1)) || die "Candidat APT ambigu pour ${package}."
        [[ "${candidate}" != "(none)" && \
            "${candidate}" =~ ^[0-9A-Za-z][0-9A-Za-z.+:~_-]{0,126}$ ]] || \
            die "Aucun candidat APT canonique pour ${package}."

        run_bounded_to_file "apt-madison-${package}" "${APT_PLAN_TIMEOUT_SECONDS}" \
            "${madison_log}" "${SUDO_BIN}" -n -- /usr/bin/env LC_ALL=C \
            /usr/bin/apt-cache madison "${package}" || \
            die "Provenance APT illisible pour ${package}."
        verify_ubuntu_madison_origin "${package}" "${candidate}" "${madison_log}" || \
            die "Le candidat ${package}=${candidate} n'a pas une provenance Ubuntu Jammy autorisée."
        "${TAIL_BIN}" -n "${FAILURE_LOG_MAX_LINES}" "${madison_log}" >>"${PROVISION_LOG}"
        expected_versions["${package}"]="${candidate}"
        package_specs+=("${package}=${candidate}")
        printf '[APT CANDIDAT] %s=%s, origine Ubuntu Jammy certifiée.\n' \
            "${package}" "${candidate}"
        printf '[APT CANDIDAT] %s=%s, origine Ubuntu Jammy certifiée.\n' \
            "${package}" "${candidate}" >>"${PROVISION_LOG}"
    done

    APT_PLAN_LOG="${SESSION_DIR}/apt-plan.log"
    run_bounded_to_file "apt-simulation" "${APT_PLAN_TIMEOUT_SECONDS}" \
        "${APT_PLAN_LOG}" "${SUDO_BIN}" -n -- /usr/bin/env LC_ALL=C \
        DEBIAN_FRONTEND=noninteractive /usr/bin/apt-get \
        -o DPkg::Lock::Timeout=30 \
        --simulate --yes --no-remove --no-upgrade --no-install-recommends \
        install "${package_specs[@]}" || die "La simulation APT Docker a échoué."
    while IFS= read -r line; do
        if [[ "${line}" == Remv\ * || \
            "${line}" == *"essential packages will be removed"* ]]; then
            die "La simulation APT propose une suppression : ${line}."
        fi
        if [[ "${line}" =~ ^Inst[[:space:]]+[^[:space:]]+[[:space:]]+\[[^]]+\] ]]; then
            die "La simulation APT propose une mise à niveau ou rétrogradation : ${line}."
        fi
    done <"${APT_PLAN_LOG}"
    "${TAIL_BIN}" -n "${FAILURE_LOG_MAX_LINES}" "${APT_PLAN_LOG}" >>"${PROVISION_LOG}"

    run_bounded "apt-install" "${APT_INSTALL_TIMEOUT_SECONDS}" \
        "${SUDO_BIN}" -n -- /usr/bin/env LC_ALL=C DEBIAN_FRONTEND=noninteractive \
        /usr/bin/apt-get \
        -o DPkg::Lock::Timeout=30 \
        --yes --no-remove --no-upgrade --no-install-recommends \
        install "${package_specs[@]}" || die "Installation APT Docker bornée échouée."

    dpkg_audit="$(bounded_probe "${SUDO_BIN}" -n -- /usr/bin/dpkg --audit 2>&1)" || \
        die "dpkg --audit a échoué après installation."
    [[ -z "${dpkg_audit}" ]] || die "État dpkg incohérent après installation : ${dpkg_audit}."
    for package in "${missing_packages[@]}"; do
        actual_version="$(installed_package_version "${package}" 2>/dev/null || true)"
        [[ "${actual_version}" == "${expected_versions[${package}]}" ]] || \
            die "Version installée inattendue pour ${package}: ${actual_version:-absente}, attendu=${expected_versions[${package}]}."
    done
    docker_version="$(installed_package_version docker.io)"
    buildx_version="$(installed_package_version docker-buildx)"
fi

verify_nvidia_package_stack
read_guest_guard || die "La garde invitée a disparu ou dérivé après APT."
current_boot_id="$(read_fixed_file /proc/sys/kernel/random/boot_id 2>/dev/null)" || \
    die "Boot ID illisible après APT."
[[ "${current_boot_id}" == "${INITIAL_BOOT_ID}" ]] || \
    die "Le boot ID a changé pendant APT; aucune configuration ni relance de service."

certify_fixed_executable "${DOCKER_BIN}" || die "${DOCKER_BIN} installé mais non sûr."
certify_fixed_executable "${DOCKERD_BIN}" || die "${DOCKERD_BIN} installé mais non sûr."
certify_buildx_plugin || die "Le plugin docker-buildx fixe ou son propriétaire dpkg n'est pas sûr."
refuse_systemd_shadow_paths || die "Un override systemd est apparu pendant APT."
certify_systemd_stack || \
    die "Fragments, drop-ins, commandes ou socket systemd Docker/containerd non certifiés."

if ((docker_directory_state == 0)); then
    if bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -L /etc/docker \
        >/dev/null 2>&1 || \
        bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -e /etc/docker \
            >/dev/null 2>&1; then
        certify_docker_directory_state || \
            die "/etc/docker apparu pendant APT mais n'est pas un répertoire sûr."
    else
        run_bounded "create-etc-docker" "${CONFIG_TIMEOUT_SECONDS}" \
            "${SUDO_BIN}" -n -- /usr/bin/install -d -o root -g root -m 0755 \
            /etc/docker || die "Création sûre de /etc/docker échouée."
        certify_docker_directory_state || die "/etc/docker créé mais non certifiable."
    fi
    docker_directory_state=1
fi

if ((CONFIG_WAS_ABSENT == 1)); then
    if bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -L "${DOCKER_CONFIG}" \
        >/dev/null 2>&1 || \
        bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" -e "${DOCKER_CONFIG}" \
            >/dev/null 2>&1; then
        die "${DOCKER_CONFIG} est apparu concurremment; configuration refusée."
    fi

    ROOT_TEMP_DIR="$(bounded_probe "${SUDO_BIN}" -n -- "${MKTEMP_BIN}" -d \
        /run/morsehgp3d-docker.XXXXXXXX)" || \
        die "Création du temporaire root de configuration Docker échouée."
    [[ "${ROOT_TEMP_DIR}" == /run/morsehgp3d-docker.* && \
        "${ROOT_TEMP_DIR}" != *$'\n'* && "${ROOT_TEMP_DIR}" != *$'\r'* ]] || \
        die "Chemin temporaire root Docker non canonique."
    certify_root_owned_path "${ROOT_TEMP_DIR}" directory || \
        die "Temporaire root Docker non certifiable."
    root_temp_metadata="$(bounded_probe "${SUDO_BIN}" -n -- "${STAT_BIN}" -Lc '%u|%a|%d:%i' -- \
        "${ROOT_TEMP_DIR}" 2>/dev/null)" || die "Métadonnées du temporaire root illisibles."
    IFS='|' read -r root_temp_owner root_temp_mode ROOT_TEMP_ID <<<"${root_temp_metadata}"
    [[ "${root_temp_owner}" == "0" && "${root_temp_mode}" == "700" && \
        "${ROOT_TEMP_ID}" =~ ^[0-9]+:[0-9]+$ ]] || \
        die "Le temporaire root Docker n'est pas root:0700 avec inode canonique."
    config_candidate="${ROOT_TEMP_DIR}/daemon.json"
    bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" ! -e "${config_candidate}" \
        >/dev/null 2>&1 || die "Le candidat daemon.json existe déjà."
    bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" ! -L "${config_candidate}" \
        >/dev/null 2>&1 || die "Le candidat daemon.json est un symlink."

    run_bounded "nvidia-runtime-config" "${CONFIG_TIMEOUT_SECONDS}" \
        "${SUDO_BIN}" -n -- "${NVIDIA_CTK_BIN}" runtime configure \
        --runtime=docker \
        --nvidia-runtime-path="${NVIDIA_RUNTIME_BIN}" \
        --config="${config_candidate}" || die "Configuration NVIDIA de Docker échouée."
    daemon_json_file_is_approved "${config_candidate}" || \
        die "Le candidat daemon.json NVIDIA n'est pas un JSON root strict et borné."

    run_bounded "dockerd-candidate-validation" "${CONFIG_TIMEOUT_SECONDS}" \
        "${SUDO_BIN}" -n -- "${DOCKERD_BIN}" --validate \
        --config-file="${config_candidate}" || die "dockerd refuse le candidat NVIDIA."

    CONFIG_STAGING="$(bounded_probe "${SUDO_BIN}" -n -- "${MKTEMP_BIN}" \
        /etc/docker/.morsehgp3d-daemon.XXXXXXXX)" || \
        die "Création du staging daemon.json échouée."
    [[ "${CONFIG_STAGING}" == /etc/docker/.morsehgp3d-daemon.* && \
        "${CONFIG_STAGING}" != *$'\n'* && "${CONFIG_STAGING}" != *$'\r'* ]] || \
        die "Chemin de staging daemon.json non canonique."
    run_bounded "stage-daemon-config" "${CONFIG_TIMEOUT_SECONDS}" \
        "${SUDO_BIN}" -n -- /usr/bin/install -o root -g root -m 0644 -- \
        "${config_candidate}" "${CONFIG_STAGING}" || die "Staging daemon.json échoué."
    daemon_json_file_is_approved "${CONFIG_STAGING}" || \
        die "Le staging daemon.json n'est pas un JSON root strict et borné."
    CONFIG_STAGING_ID="$(bounded_probe "${SUDO_BIN}" -n -- "${STAT_BIN}" -Lc '%d:%i' -- \
        "${CONFIG_STAGING}" 2>/dev/null)" || die "Inode du staging daemon.json illisible."
    [[ "${CONFIG_STAGING_ID}" =~ ^[0-9]+:[0-9]+$ ]] || \
        die "Inode du staging daemon.json non canonique."
    run_bounded "dockerd-staging-validation" "${CONFIG_TIMEOUT_SECONDS}" \
        "${SUDO_BIN}" -n -- "${DOCKERD_BIN}" --validate \
        --config-file="${CONFIG_STAGING}" || die "dockerd refuse le staging NVIDIA."

    certify_docker_directory_state || die "/etc/docker a changé avant publication."
    bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" ! -e "${DOCKER_CONFIG}" \
        >/dev/null 2>&1 || die "${DOCKER_CONFIG} est apparu avant publication."
    bounded_probe "${SUDO_BIN}" -n -- "${TEST_BIN}" ! -L "${DOCKER_CONFIG}" \
        >/dev/null 2>&1 || die "${DOCKER_CONFIG} est devenu un symlink avant publication."
    run_bounded "publish-daemon-config" "${CONFIG_TIMEOUT_SECONDS}" \
        "${SUDO_BIN}" -n -- /usr/bin/ln -T -- "${CONFIG_STAGING}" "${DOCKER_CONFIG}" || \
        die "Publication atomique sans écrasement de daemon.json échouée."
    published_id="$(bounded_probe "${SUDO_BIN}" -n -- "${STAT_BIN}" -Lc '%d:%i' -- \
        "${DOCKER_CONFIG}" 2>/dev/null)" || die "Inode daemon.json publié illisible."
    [[ "${published_id}" == "${CONFIG_STAGING_ID}" ]] || \
        die "Le daemon.json publié ne correspond pas au staging certifié."
    safety_probe "${SUDO_BIN}" -n -- "${RM_BIN}" -f -- "${CONFIG_STAGING}" || \
        die "Suppression du lien staging daemon.json échouée."
    CONFIG_STAGING=""
    CONFIG_STAGING_ID=""
    daemon_config_is_approved || die "Le daemon.json publié n'est pas strictement approuvé."
    config_state=1
fi

run_bounded "dockerd-config-validation" "${CONFIG_TIMEOUT_SECONDS}" \
    "${SUDO_BIN}" -n -- "${DOCKERD_BIN}" --validate --config-file="${DOCKER_CONFIG}" || \
    die "dockerd refuse la configuration NVIDIA publiée."

certify_systemd_stack || die "Unités systemd non certifiées avant activation."
run_bounded "docker-service-enable" "${SERVICE_TIMEOUT_SECONDS}" \
    "${SUDO_BIN}" -n -- /usr/bin/systemctl enable docker.service || \
    die "Activation persistante de docker.service échouée."
refuse_systemd_shadow_paths || die "Un override systemd est apparu après enable."
certify_systemd_stack || die "Unités systemd non certifiées après enable."
run_bounded "docker-service-restart" "${SERVICE_TIMEOUT_SECONDS}" \
    "${SUDO_BIN}" -n -- /usr/bin/systemctl restart docker.service || \
    die "Redémarrage borné de docker.service échoué."

verify_postconditions
printf '%s\n' '[SUCCÈS] Préparation Docker Phase 3 achevée sans conteneur ni travail GPU.'
