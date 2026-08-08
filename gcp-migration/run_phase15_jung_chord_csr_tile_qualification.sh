#!/usr/bin/env bash
set -Eeuo pipefail

readonly DEFAULT_PROJECT_ID="devpod-gpu-exploration"
readonly DEFAULT_ZONE="europe-west4-a"
readonly DEFAULT_INSTANCE_NAME="ehgp-blackwell-spot"
readonly AI_CAPACITY_ZONE="europe-west4-ai1a"
readonly AI_CAPACITY_INSTANCE_NAME="ehgp-blackwell-spot-ai1a"
readonly EXPECTED_MAX_RUN_SECONDS=3600
readonly GUEST_SHUTDOWN_MINUTES=45
readonly GCLOUD_REMOTE_TIMEOUT_SECONDS=2880
readonly GCLOUD_READ_TIMEOUT_SECONDS=30
readonly GCLOUD_TRANSFER_TIMEOUT_SECONDS=120
readonly GCLOUD_KILL_AFTER_SECONDS=10
readonly SSH_KEY_TTL="70m"
readonly SSH_KEY_TTL_SLACK_SECONDS=660
readonly DEFAULT_CHORD_CAPACITY=67108864
readonly MAX_CHORD_CAPACITY=4294967296
readonly TIMESTAMP_TOLERANCE_SECONDS=300
readonly WORK_RESERVE_SECONDS=120
readonly STOP_SCRIPT_FAILURE=90
readonly STOP_READBACK_FAILURE=91
readonly STOP_NOT_TERMINATED=92
readonly SSH_KEY_CLEANUP_FAILURE=93

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly START_SCRIPT="${SCRIPT_DIR}/start_and_verify.sh"
readonly STOP_SCRIPT="${SCRIPT_DIR}/stop_and_verify.sh"
readonly REMOTE_WORKER_RELATIVE="gcp-migration/phase15_jung_chord_csr_tile_remote_qualification.sh"
readonly ASSEMBLER_RELATIVE="morsehgp3d/tests/cuda/assemble_phase15_jung_chord_csr_tile_qualification.py"
readonly STATUS_RELATIVE="docs/implementation_status.toml"
readonly STATUS_CHECKER_RELATIVE="tools/check_implementation_status.py"

PROJECT_ID="${GCP_PROJECT_ID:-${DEFAULT_PROJECT_ID}}"
ZONE="${GCP_ZONE:-${DEFAULT_ZONE}}"
INSTANCE_NAME="${GCP_INSTANCE_NAME:-${DEFAULT_INSTANCE_NAME}}"
RESULT_DIR="${MORSEHGP3D_PHASE15_JUNG_RESULT_DIR:-${TMPDIR:-/tmp}/morsehgp3d-phase15-jung-results}"
CHORD_CAPACITY="${MORSEHGP3D_PHASE15_JUNG_CHORD_CAPACITY:-${DEFAULT_CHORD_CAPACITY}}"
ASSUME_YES=0
REPOSITORY_ROOT=""
HEAD_SHA=""
ORIGIN_URL=""
START_HANDOFF=""
SESSION_LAST_START_TIMESTAMP=""
SESSION_HANDOFF_STATUS=""
EFFECTIVE_GCE_DEADLINE_EPOCH=""
SESSION_CERTIFIED=0
TARGET_STOP_CERTIFIED=0
START_INVOCATION_ATTEMPTED=0
PRE_START_GENERATION_JSON=""
PRE_START_SNAPSHOT_CERTIFIED=0
FINAL_STOP_VERIFIED_AT_UTC=""
SSH_KEY_DIR=""
SSH_KEY_FILE=""
SSH_KEY_EXPIRATION_UTC=""
SSH_KEY_IMPORT_ATTEMPTED=0
REMOTE_WORKDIR=""
LOCAL_TEMP_ROOT=""
LOCAL_TEMP_ARTIFACT=""
LOCAL_TEMP_LOGS=""
LOCAL_FINAL_ARTIFACT=""
LOCAL_FINAL_LOGS=""
PUBLICATION_COMPLETE=0

die() {
    printf '[ERREUR ROUTE P15-HOCUDA-P0] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/run_phase15_jung_chord_csr_tile_qualification.sh \
  --yes [--result-dir RÉPERTOIRE] [--chord-capacity N]

Point d'entrée G4 unique et gardé pour P15-HOCUDA-P0. Le commit propre doit
déjà appartenir à origin/main. La route crée une clé OS Login expirante, appelle
exclusivement start_and_verify.sh, clone ce SHA sur l'invité, puis délègue au
worker non mutatif la séquence fixe suivante :

  1. n=32, all-pairs, oracle exact sous compute-sanitizer memcheck ;
  2. n=50000 uniform_latin, support 4, rang fermé 11 ;
  3. n=50000 eight_clusters, support 4, rang fermé 11.

Aucun palier intermédiaire, aucune répétition de tuning et aucun agrandissement
automatique ne sont permis. Une saturation de capacité est publiée telle quelle
et fait terminer la route avec le code 3 après certification de l'arrêt. La
capacité 50k par défaut est 67 108 864 PointIds (environ 576 Mio pour PointIds
u64 et flags, hors CSR et espace de travail); la variable d'environnement
MORSEHGP3D_PHASE15_JUNG_CHORD_CAPACITY ou --chord-capacity la fixe.

La route ne rend la main qu'après stop_and_verify.sh sur la génération exacte
et une relecture indépendante TERMINATED. Elle ne touche aucune autre VM.
EOF
}

while (($# > 0)); do
    case "$1" in
        --yes)
            ASSUME_YES=1
            shift
            ;;
        --result-dir)
            (($# >= 2)) || die "Valeur manquante après --result-dir."
            RESULT_DIR="$2"
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

((ASSUME_YES == 1)) || \
    die "--yes est obligatoire et atteste la session G4 Spot explicitement autorisée."
[[ "${PROJECT_ID}" == "${DEFAULT_PROJECT_ID}" ]] || die "Projet GCP refusé : ${PROJECT_ID}."
case "${ZONE}/${INSTANCE_NAME}" in
    "${DEFAULT_ZONE}/${DEFAULT_INSTANCE_NAME}"|\
    "${AI_CAPACITY_ZONE}/${AI_CAPACITY_INSTANCE_NAME}") ;;
    *) die "Cible refusée : ${ZONE}/${INSTANCE_NAME}." ;;
esac
[[ "${CHORD_CAPACITY}" =~ ^[1-9][0-9]*$ ]] || die "La capacité doit être strictement positive."
((10#${CHORD_CAPACITY} <= MAX_CHORD_CAPACITY)) || die "La capacité dépasse ${MAX_CHORD_CAPACITY}."
CHORD_CAPACITY="$((10#${CHORD_CAPACITY}))"
[[ -n "${RESULT_DIR}" ]] || die "Le répertoire de résultat ne peut pas être vide."

for command_name in git gcloud python3 ssh-keygen timeout stat awk; do
    command -v "${command_name}" >/dev/null 2>&1 || die "${command_name} est introuvable."
done
timeout_version="$(timeout --version 2>/dev/null | sed -n '1p')" || die "GNU timeout est illisible."
[[ "${timeout_version}" == timeout\ \(GNU\ coreutils\)* ]] || die "GNU timeout est obligatoire."
[[ -x "${START_SCRIPT}" && ! -L "${START_SCRIPT}" ]] || die "start_and_verify.sh est absent ou symbolique."
[[ -x "${STOP_SCRIPT}" && ! -L "${STOP_SCRIPT}" ]] || die "stop_and_verify.sh est absent ou symbolique."

REPOSITORY_ROOT="$(git -C "${SCRIPT_DIR}/.." rev-parse --show-toplevel 2>/dev/null)" || die "Racine Git introuvable."
REPOSITORY_ROOT="$(cd -- "${REPOSITORY_ROOT}" && pwd -P)" || die "Racine Git illisible."
readonly REMOTE_WORKER="${REPOSITORY_ROOT}/${REMOTE_WORKER_RELATIVE}"
readonly ASSEMBLER="${REPOSITORY_ROOT}/${ASSEMBLER_RELATIVE}"
readonly STATUS_FILE="${REPOSITORY_ROOT}/${STATUS_RELATIVE}"
readonly STATUS_CHECKER="${REPOSITORY_ROOT}/${STATUS_CHECKER_RELATIVE}"
[[ -x "${REMOTE_WORKER}" && ! -L "${REMOTE_WORKER}" ]] || die "Worker distant absent ou symbolique."
[[ -f "${ASSEMBLER}" && ! -L "${ASSEMBLER}" ]] || die "Validateur absent ou symbolique."
[[ -f "${STATUS_FILE}" && ! -L "${STATUS_FILE}" && \
    -f "${STATUS_CHECKER}" && ! -L "${STATUS_CHECKER}" ]] || \
    die "Le registre de phase ou son checker est absent ou symbolique."
[[ -z "$(GIT_OPTIONAL_LOCKS=0 git -C "${REPOSITORY_ROOT}" status --porcelain --untracked-files=normal)" ]] || \
    die "Le worktree doit être entièrement propre avant toute session GCP."
HEAD_SHA="$(git -C "${REPOSITORY_ROOT}" rev-parse HEAD)" || die "HEAD illisible."
[[ "${HEAD_SHA}" =~ ^[0-9a-f]{40}$ ]] || die "HEAD non canonique."
python3 -B "${STATUS_CHECKER}" || die "Le registre des portes d'implémentation est invalide."
python3 -B - "${STATUS_FILE}" <<'PY' || \
    die "La porte d'entrée P15-HOCUDA-P0 n'est pas ouverte; aucun démarrage GCP."
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
ORIGIN_URL="$(git -C "${REPOSITORY_ROOT}" remote get-url origin)" || die "origin illisible."
case "${ORIGIN_URL}" in
    https://github.com/Ludwig-H/E-HGP|https://github.com/Ludwig-H/E-HGP.git) ;;
    *) die "Remote origin refusé : ${ORIGIN_URL}." ;;
esac
git -C "${REPOSITORY_ROOT}" fetch --quiet --no-tags \
    origin refs/heads/main:refs/remotes/origin/main || die "Impossible de rafraîchir origin/main."
git -C "${REPOSITORY_ROOT}" merge-base --is-ancestor \
    "${HEAD_SHA}" refs/remotes/origin/main || die "HEAD doit déjà être poussé sur origin/main."

RESULT_DIR="$(python3 - "${RESULT_DIR}" <<'PY'
from pathlib import Path
import sys
print(Path(sys.argv[1]).expanduser().resolve(strict=False))
PY
)" || die "Impossible de résoudre le répertoire de résultat."
case "${RESULT_DIR}/" in
    "${REPOSITORY_ROOT}/"*) die "Le résultat doit rester hors du worktree." ;;
esac
mkdir -p -- "${RESULT_DIR}" || die "Impossible de créer le répertoire de résultat."
RESULT_DIR="$(cd -- "${RESULT_DIR}" && pwd -P)" || die "Répertoire de résultat illisible."
LOCAL_FINAL_ARTIFACT="${RESULT_DIR}/phase15-jung-chord-csr-tile-${HEAD_SHA}.json"
LOCAL_FINAL_LOGS="${RESULT_DIR}/phase15-jung-chord-csr-tile-${HEAD_SHA}.logs"
START_HANDOFF="${RESULT_DIR}/phase15-jung-chord-csr-tile-${HEAD_SHA}.start-handoff.json"
for target in "${LOCAL_FINAL_ARTIFACT}" "${LOCAL_FINAL_LOGS}" "${START_HANDOFF}"; do
    [[ ! -e "${target}" && ! -L "${target}" ]] || die "La cible existe déjà : ${target}."
done
LOCAL_TEMP_ROOT="$(mktemp -d "${RESULT_DIR}/.phase15-jung-${HEAD_SHA}.XXXXXXXX.partial")" || die "Temporaire local impossible."
chmod 700 -- "${LOCAL_TEMP_ROOT}"
LOCAL_TEMP_ARTIFACT="${LOCAL_TEMP_ROOT}/p15-result.json"
LOCAL_TEMP_LOGS="${LOCAL_TEMP_ROOT}/p15-result.logs"

export GCP_PROJECT_ID="${PROJECT_ID}"
export GCP_ZONE="${ZONE}"
export GCP_INSTANCE_NAME="${INSTANCE_NAME}"
export GCP_GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES}"

shell_quote() {
    printf '%q' "$1"
}

remote_exec() {
    local command="$1"
    timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_REMOTE_TIMEOUT_SECONDS}s" gcloud compute ssh "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" --zone="${ZONE}" --quiet \
        --ssh-key-file="${SSH_KEY_FILE}" \
        --ssh-key-expiration="${SSH_KEY_EXPIRATION_UTC}" \
        --ssh-flag='-o ConnectTimeout=15' --ssh-flag='-o BatchMode=yes' \
        --command="${command}"
}

create_session_ssh_key() {
    local old_umask=""
    local created_dir=""
    local declared_public=""
    local derived_public=""
    old_umask="$(umask)" || return 1
    umask 077
    created_dir="$(mktemp -d "${TMPDIR:-/tmp}/morsehgp3d-phase15-jung-ssh.XXXXXXXX")" || {
        umask "${old_umask}"
        return 1
    }
    umask "${old_umask}"
    SSH_KEY_DIR="$(cd -- "${created_dir}" && pwd -P)" || return 1
    SSH_KEY_FILE="${SSH_KEY_DIR}/id_ed25519"
    [[ "$(stat -c '%a' -- "${SSH_KEY_DIR}")" == "700" ]] || return 1
    ssh-keygen -q -t ed25519 -N '' -C "morsehgp3d-phase15-jung-${HEAD_SHA}" \
        -f "${SSH_KEY_FILE}" || return 1
    chmod 600 -- "${SSH_KEY_FILE}" || return 1
    [[ -f "${SSH_KEY_FILE}" && ! -L "${SSH_KEY_FILE}" && \
        -f "${SSH_KEY_FILE}.pub" && ! -L "${SSH_KEY_FILE}.pub" ]] || return 1
    [[ "$(stat -c '%a' -- "${SSH_KEY_FILE}")" == "600" ]] || return 1
    declared_public="$(awk 'NF >= 2 {print $1 " " $2; exit}' "${SSH_KEY_FILE}.pub")" || return 1
    derived_public="$(ssh-keygen -y -P '' -f "${SSH_KEY_FILE}" 2>/dev/null)" || return 1
    derived_public="$(awk 'NF >= 2 {print $1 " " $2; exit}' <<<"${derived_public}")"
    [[ "${declared_public}" == ssh-ed25519\ * && "${derived_public}" == "${declared_public}" ]] || return 1
    export GCP_SSH_KEY_FILE="${SSH_KEY_FILE}"
}

capture_session_ssh_key_expiration() {
    local algorithm=""
    local blob=""
    local profile_json=""
    local fields=""
    read -r algorithm blob _ <"${SSH_KEY_FILE}.pub" || return 1
    profile_json="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" 30s \
        gcloud compute os-login describe-profile --project="${PROJECT_ID}" --format=json)" || return 1
    fields="$(python3 - "${algorithm}" "${blob}" "${EXPECTED_MAX_RUN_SECONDS}" \
        "${SSH_KEY_TTL_SLACK_SECONDS}" "${profile_json}" <<'PY'
from datetime import datetime, timezone
import json
import sys
import time
algorithm, blob = sys.argv[1], sys.argv[2]
minimum, maximum = int(sys.argv[3]), int(sys.argv[3]) + int(sys.argv[4])
value = json.loads(sys.argv[5])
records = value.get("sshPublicKeys") if isinstance(value, dict) else None
if not isinstance(records, dict):
    raise SystemExit("missing OS Login keys")
matches = []
for record in records.values():
    if not isinstance(record, dict):
        continue
    fields = str(record.get("key", "")).split()
    if len(fields) >= 2 and fields[:2] == [algorithm, blob]:
        try:
            matches.append(int(record["expirationTimeUsec"]))
        except (KeyError, TypeError, ValueError):
            pass
if len(matches) != 1:
    raise SystemExit("session key absent or ambiguous")
remaining = (matches[0] - time.time_ns() // 1000) // 1_000_000
if not minimum <= remaining <= maximum:
    raise SystemExit("session key TTL outside bounds")
seconds, micros = divmod(matches[0], 1_000_000)
stamp = datetime.fromtimestamp(seconds, timezone.utc).replace(microsecond=micros)
print(f"{remaining}\t{stamp.isoformat(timespec='microseconds').replace('+00:00', 'Z')}")
PY
)" || return 1
    local remaining=""
    IFS=$'\t' read -r remaining SSH_KEY_EXPIRATION_UTC <<<"${fields}"
    [[ "${remaining}" =~ ^[0-9]+$ && \
        "${SSH_KEY_EXPIRATION_UTC}" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}T.*Z$ ]]
}

import_session_ssh_key() {
    SSH_KEY_IMPORT_ATTEMPTED=1
    timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" 30s \
        gcloud compute os-login ssh-keys add --key-file="${SSH_KEY_FILE}.pub" \
        --ttl="${SSH_KEY_TTL}" --project="${PROJECT_ID}" --quiet >/dev/null || return 1
    capture_session_ssh_key_expiration
}

revoke_and_remove_session_ssh_key() {
    local cleanup_failed=0
    [[ -n "${SSH_KEY_DIR}" ]] || return 0
    [[ "${SSH_KEY_FILE}" == "${SSH_KEY_DIR}/id_ed25519" && \
        -d "${SSH_KEY_DIR}" && ! -L "${SSH_KEY_DIR}" ]] || return 1
    if ((SSH_KEY_IMPORT_ATTEMPTED == 1)) && [[ -f "${SSH_KEY_FILE}.pub" ]]; then
        if timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" 30s \
            gcloud compute os-login ssh-keys remove --key-file="${SSH_KEY_FILE}.pub" \
            --project="${PROJECT_ID}" --quiet >/dev/null 2>&1; then
            printf '[CLÉ SSH] Clé OS Login révoquée.\n'
        else
            printf '[AVERTISSEMENT] Révocation illisible; la clé reste bornée par TTL=%s.\n' "${SSH_KEY_TTL}" >&2
        fi
    fi
    rm -f -- "${SSH_KEY_FILE}" "${SSH_KEY_FILE}.pub" || cleanup_failed=1
    ((cleanup_failed == 0)) && rmdir -- "${SSH_KEY_DIR}" || cleanup_failed=1
    ((cleanup_failed == 0)) || return 1
    unset GCP_SSH_KEY_FILE
    SSH_KEY_DIR=""
    SSH_KEY_FILE=""
    SSH_KEY_EXPIRATION_UTC=""
    SSH_KEY_IMPORT_ATTEMPTED=0
}

capture_pre_start_snapshot() {
    local lifecycle_json=""
    lifecycle_json="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" --zone="${ZONE}" --format=json)" || return 1
    PRE_START_GENERATION_JSON="$(python3 - "${lifecycle_json}" <<'PY'
import json
import sys
value = json.loads(sys.argv[1])
if not isinstance(value, dict) or value.get("status") != "TERMINATED":
    raise SystemExit("target not initially terminated")
generation = value.get("lastStartTimestamp")
if generation in (None, ""):
    generation = None
elif not isinstance(generation, str) or "\n" in generation or "\r" in generation:
    raise SystemExit("ambiguous initial generation")
print(json.dumps(generation, separators=(",", ":")))
PY
)" || return 1
    PRE_START_SNAPSHOT_CERTIFIED=1
}

target_has_unchanged_terminated_generation() {
    local lifecycle_json=""
    ((PRE_START_SNAPSHOT_CERTIFIED == 1)) || return 1
    lifecycle_json="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" --zone="${ZONE}" --format=json 2>/dev/null)" || return 1
    python3 - "${PRE_START_GENERATION_JSON}" "${lifecycle_json}" <<'PY'
import json
import sys
expected = json.loads(sys.argv[1])
value = json.loads(sys.argv[2])
if not isinstance(value, dict) or value.get("status") != "TERMINATED":
    raise SystemExit("target not terminated")
generation = value.get("lastStartTimestamp")
if generation in (None, ""):
    generation = None
if generation != expected:
    raise SystemExit("target generation changed")
PY
}

load_targeted_handoff() {
    local fields=""
    [[ -f "${START_HANDOFF}" && ! -L "${START_HANDOFF}" ]] || return 1
    fields="$(python3 - "${START_HANDOFF}" "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" \
        "${GUEST_SHUTDOWN_MINUTES}" <<'PY'
import json
from pathlib import Path
import sys
value = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
expected = {
    "guest_shutdown_minutes": int(sys.argv[5]),
    "instance": sys.argv[4],
    "project": sys.argv[2],
    "schema": "e-hgp.start-handoff.v3",
    "zone": sys.argv[3],
}
if not isinstance(value, dict) or set(value) != set(expected) | {"last_start_timestamp", "status"}:
    raise SystemExit("invalid handoff keys")
generation = value.pop("last_start_timestamp")
status = value.pop("status")
if value != expected or status not in {"targeted_running", "targeted_stopping"}:
    raise SystemExit("invalid handoff")
if not isinstance(generation, str) or not generation or "\n" in generation or "\r" in generation:
    raise SystemExit("invalid generation")
print(f"{status}\t{generation}")
PY
)" || return 1
    IFS=$'\t' read -r SESSION_HANDOFF_STATUS SESSION_LAST_START_TIMESTAMP <<<"${fields}"
    [[ -n "${SESSION_LAST_START_TIMESTAMP}" ]]
}

certify_session_deadline() {
    local lifecycle_json=""
    lifecycle_json="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" --zone="${ZONE}" --format=json)" || return 1
    EFFECTIVE_GCE_DEADLINE_EPOCH="$(python3 - "${SESSION_LAST_START_TIMESTAMP}" "${ZONE}" \
        "${EXPECTED_MAX_RUN_SECONDS}" "${TIMESTAMP_TOLERANCE_SECONDS}" \
        "${WORK_RESERVE_SECONDS}" "${lifecycle_json}" <<'PY'
from datetime import datetime, timezone
import json
import sys
def parse(value):
    if not isinstance(value, str) or not value:
        raise SystemExit("timestamp absent")
    return int(datetime.fromisoformat(value[:-1] + "+00:00" if value.endswith("Z") else value).timestamp())
generation, zone = sys.argv[1], sys.argv[2]
duration, tolerance, reserve = map(int, sys.argv[3:6])
value = json.loads(sys.argv[6])
if value.get("status") != "RUNNING" or value.get("lastStartTimestamp") != generation:
    raise SystemExit("target generation not running")
scheduling = value.get("scheduling")
if not isinstance(scheduling, dict) or str(scheduling.get("maxRunDuration", {}).get("seconds")) != str(duration):
    raise SystemExit("unexpected maxRunDuration")
computed = parse(generation) + duration
termination = value.get("terminationTimestamp")
if termination not in (None, ""):
    if abs(parse(termination) - computed) > tolerance:
        raise SystemExit("termination timestamp mismatch")
elif "-ai" not in zone:
    raise SystemExit("termination timestamp absent outside AI zone")
safe = computed - tolerance
if safe - reserve <= int(datetime.now(timezone.utc).timestamp()):
    raise SystemExit("safe deadline already reached")
print(safe)
PY
)" || return 1
    [[ "${EFFECTIVE_GCE_DEADLINE_EPOCH}" =~ ^[0-9]+$ ]]
}

print_control_command() {
    printf 'Commande de contrôle : gcloud compute instances describe %q --project=%q --zone=%q --format=%q\n' \
        "${INSTANCE_NAME}" "${PROJECT_ID}" "${ZONE}" 'value(status,lastStartTimestamp)' >&2
    if [[ -n "${SESSION_LAST_START_TIMESTAMP}" ]]; then
        printf 'Commande d’arrêt ciblé : GCP_PROJECT_ID=%q GCP_ZONE=%q GCP_INSTANCE_NAME=%q %q --yes --expected-last-start-timestamp %q\n' \
            "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" "${STOP_SCRIPT}" \
            "${SESSION_LAST_START_TIMESTAMP}" >&2
    fi
}

certify_target_stopped() {
    local final_status=""
    local final_generation=""
    [[ -n "${SESSION_LAST_START_TIMESTAMP}" ]] || return "${STOP_SCRIPT_FAILURE}"
    "${STOP_SCRIPT}" --yes --expected-last-start-timestamp \
        "${SESSION_LAST_START_TIMESTAMP}" || {
            print_control_command
            return "${STOP_SCRIPT_FAILURE}"
        }
    final_status="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" --zone="${ZONE}" --format='value(status)' 2>/dev/null)" || \
        return "${STOP_READBACK_FAILURE}"
    [[ "${final_status}" == "TERMINATED" ]] || return "${STOP_NOT_TERMINATED}"
    final_generation="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" --zone="${ZONE}" \
        --format='value(lastStartTimestamp)' 2>/dev/null)" || return "${STOP_READBACK_FAILURE}"
    [[ "${final_generation}" == "${SESSION_LAST_START_TIMESTAMP}" ]] || return "${STOP_NOT_TERMINATED}"
    FINAL_STOP_VERIFIED_AT_UTC="$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
    TARGET_STOP_CERTIFIED=1
    SESSION_CERTIFIED=0
    printf '[TERMINATED P15-HOCUDA-P0] %s/%s/%s génération exacte certifiée.\n' \
        "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}"
}

cleanup_local_temp() {
    [[ -n "${LOCAL_TEMP_ROOT}" && -d "${LOCAL_TEMP_ROOT}" ]] || return 0
    case "${LOCAL_TEMP_ROOT}" in
        "${RESULT_DIR}"/.phase15-jung-*.partial)
            rm -rf -- "${LOCAL_TEMP_ROOT}"
            ;;
        *)
            printf '[ERREUR] Nettoyage local refusé : %s\n' "${LOCAL_TEMP_ROOT}" >&2
            return 1
            ;;
    esac
}

on_exit() {
    local original_status=$?
    local stop_status=0
    local cleanup_status=0
    trap - EXIT HUP INT TERM
    if ((TARGET_STOP_CERTIFIED == 0)) && [[ -z "${SESSION_LAST_START_TIMESTAMP}" ]]; then
        if load_targeted_handoff 2>/dev/null; then
            SESSION_CERTIFIED=1
        fi
    fi
    if ((TARGET_STOP_CERTIFIED == 0 && SESSION_CERTIFIED == 1)); then
        if certify_target_stopped; then
            stop_status=0
        else
            stop_status=$?
        fi
        if ((stop_status != 0)); then
            printf '[BLOCAGE] Arrêt ciblé non certifié : projet=%s zone=%s instance=%s dernier état=illisible.\n' \
                "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" >&2
            print_control_command
            printf '[HANDOFF CONSERVÉ] %s\n' "${START_HANDOFF}" >&2
            printf '[CLÉ SSH CONSERVÉE] %s expiration=%s\n' "${SSH_KEY_FILE}" "${SSH_KEY_EXPIRATION_UTC}" >&2
            exit "${stop_status}"
        fi
    elif ((START_INVOCATION_ATTEMPTED == 1 && TARGET_STOP_CERTIFIED == 0)); then
        if ! target_has_unchanged_terminated_generation; then
            printf '[BLOCAGE] L’état ciblé après échec du démarrage est illisible ou a changé.\n' >&2
            print_control_command
            exit "${STOP_SCRIPT_FAILURE}"
        fi
    fi
    if ((PUBLICATION_COMPLETE == 0)); then
        cleanup_local_temp || cleanup_status=1
    fi
    if ((TARGET_STOP_CERTIFIED == 1)) && [[ -e "${START_HANDOFF}" ]]; then
        rm -f -- "${START_HANDOFF}" || cleanup_status=1
    fi
    if ((START_INVOCATION_ATTEMPTED == 0 || TARGET_STOP_CERTIFIED == 1)) || \
        target_has_unchanged_terminated_generation; then
        revoke_and_remove_session_ssh_key || cleanup_status=1
    fi
    ((cleanup_status == 0)) || exit "${SSH_KEY_CLEANUP_FAILURE}"
    exit "${original_status}"
}

trap on_exit EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

[[ -z "$(git -C "${REPOSITORY_ROOT}" status --porcelain --untracked-files=normal)" ]] || \
    die "Le worktree a changé pendant la préparation."
create_session_ssh_key || die "Création de la clé ED25519 de session impossible."
import_session_ssh_key || die "Inscription bornée de la clé OS Login impossible; aucun démarrage."
capture_pre_start_snapshot || die "La cible exacte n'est pas certifiée TERMINATED avant démarrage."

printf '%s\n' \
    '[SESSION P15-HOCUDA-P0]' \
    '  phase                 : P15-HOCUDA-P0' \
    '  backend               : cuda_g4_plus_reference_cpu_oracle' \
    '  profile               : hgp_reduced' \
    '  mode                  : proposal_only_shallow_higher_support_work_falsifier_v1' \
    "  projet/zone/instance : ${PROJECT_ID}/${ZONE}/${INSTANCE_NAME}" \
    "  SHA                  : ${HEAD_SHA}" \
    "  capacité cordes      : ${CHORD_CAPACITY}" \
    '  séquence             : oracle32+memcheck -> uniform50k -> eight_clusters50k' \
    '  claims               : component_only=true, slo_eligible=false'

START_INVOCATION_ATTEMPTED=1
"${START_SCRIPT}" --yes --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}" \
    --handoff-file "${START_HANDOFF}"
SESSION_CERTIFIED=1
load_targeted_handoff || die "Le démarrage n'a pas publié son handoff ciblé."
[[ "${SESSION_HANDOFF_STATUS}" == "targeted_running" ]] || die "Statut handoff inattendu."
certify_session_deadline || die "La garde GCE exacte de 3600 s n'a pas été recertifiée."

mktemp_output="$(remote_exec \
    'remote_dir=$(mktemp -d /tmp/morsehgp3d-phase15-jung.XXXXXXXX) && printf "__EHGP_REMOTE_DIR__%s\n" "${remote_dir}"')"
REMOTE_WORKDIR="$(printf '%s\n' "${mktemp_output}" | sed -n 's/^__EHGP_REMOTE_DIR__//p' | tail -n 1)"
[[ "${REMOTE_WORKDIR}" =~ ^/tmp/morsehgp3d-phase15-jung\.[A-Za-z0-9]{8}$ ]] || \
    die "Temporaire distant absent ou ambigu : ${REMOTE_WORKDIR:-vide}."
remote_repository="${REMOTE_WORKDIR}/repository"
remote_artifact="${REMOTE_WORKDIR}/p15-result.json"
remote_logs="${REMOTE_WORKDIR}/p15-result.logs"
quoted_origin="$(shell_quote "${ORIGIN_URL}")"
quoted_repository="$(shell_quote "${remote_repository}")"
quoted_head="$(shell_quote "${HEAD_SHA}")"
quoted_deadline="$(shell_quote "${EFFECTIVE_GCE_DEADLINE_EPOCH}")"
quoted_artifact="$(shell_quote "${remote_artifact}")"
quoted_capacity="$(shell_quote "${CHORD_CAPACITY}")"

clone_output="$(remote_exec \
    "git clone --quiet --single-branch --branch main ${quoted_origin} ${quoted_repository} && git -C ${quoted_repository} checkout --quiet --detach ${quoted_head} && printf '__EHGP_REMOTE_HEAD__%s\\n' \"\$(git -C ${quoted_repository} rev-parse HEAD)\"")"
remote_head="$(printf '%s\n' "${clone_output}" | sed -n 's/^__EHGP_REMOTE_HEAD__//p' | tail -n 1)"
[[ "${remote_head}" == "${HEAD_SHA}" ]] || die "Le checkout distant ne correspond pas au SHA local."

remote_exec \
    "test -x ${quoted_repository}/${REMOTE_WORKER_RELATIVE} && cd ${quoted_repository} && ./${REMOTE_WORKER_RELATIVE} --yes --expected-sha ${quoted_head} --gce-deadline-epoch ${quoted_deadline} --chord-capacity ${quoted_capacity} --output ${quoted_artifact}"

timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
    "${GCLOUD_TRANSFER_TIMEOUT_SECONDS}s" gcloud compute scp \
    "${INSTANCE_NAME}:${remote_artifact}" "${LOCAL_TEMP_ARTIFACT}" \
    --project="${PROJECT_ID}" --zone="${ZONE}" --quiet \
    --ssh-key-file="${SSH_KEY_FILE}" --ssh-key-expiration="${SSH_KEY_EXPIRATION_UTC}" \
    --scp-flag='-o ConnectTimeout=15' --scp-flag='-o BatchMode=yes'
timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
    "${GCLOUD_TRANSFER_TIMEOUT_SECONDS}s" gcloud compute scp --recurse \
    "${INSTANCE_NAME}:${remote_logs}" "${LOCAL_TEMP_ROOT}" \
    --project="${PROJECT_ID}" --zone="${ZONE}" --quiet \
    --ssh-key-file="${SSH_KEY_FILE}" --ssh-key-expiration="${SSH_KEY_EXPIRATION_UTC}" \
    --scp-flag='-o ConnectTimeout=15' --scp-flag='-o BatchMode=yes'
[[ -s "${LOCAL_TEMP_ARTIFACT}" && -d "${LOCAL_TEMP_LOGS}" ]] || die "Artefacts distants incomplets."

python3 -B - "${LOCAL_TEMP_ARTIFACT}" "${HEAD_SHA}" "${LOCAL_TEMP_LOGS}" \
    "$(dirname -- "${ASSEMBLER}")" <<'PY'
from pathlib import Path
import sys
sys.path.insert(0, sys.argv[4])
import assemble_phase15_jung_chord_csr_tile_qualification as assembler
assembler.validate_artifact_file(
    Path(sys.argv[1]), expected_sha=sys.argv[2], log_dir=Path(sys.argv[3])
)
PY

quoted_remote_workdir="$(shell_quote "${REMOTE_WORKDIR}")"
remote_exec "test -d ${quoted_remote_workdir} && rm -rf -- ${quoted_remote_workdir}"
REMOTE_WORKDIR=""

certify_target_stopped || {
    stop_status=$?
    print_control_command
    exit "${stop_status}"
}

final_temp="${LOCAL_TEMP_ROOT}/final.json"
python3 -B - "${LOCAL_TEMP_ARTIFACT}" "${HEAD_SHA}" "${LOCAL_TEMP_LOGS}" \
    "${SESSION_LAST_START_TIMESTAMP}" "${FINAL_STOP_VERIFIED_AT_UTC}" \
    "${final_temp}" "$(dirname -- "${ASSEMBLER}")" <<'PY'
from pathlib import Path
import sys
sys.path.insert(0, sys.argv[7])
import assemble_phase15_jung_chord_csr_tile_qualification as assembler
assembler.finalize_shutdown(
    Path(sys.argv[1]),
    expected_sha=sys.argv[2],
    log_dir=Path(sys.argv[3]),
    last_start_timestamp=sys.argv[4],
    verified_at_utc=sys.argv[5],
    output_path=Path(sys.argv[6]),
)
PY
capacity_exhausted="$(python3 - "${final_temp}" <<'PY'
import json
from pathlib import Path
import sys
value = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
print("true" if value["checks"]["capacity_exhausted_observed"] else "false")
PY
)"
mv -- "${LOCAL_TEMP_LOGS}" "${LOCAL_FINAL_LOGS}"
mv -- "${final_temp}" "${LOCAL_FINAL_ARTIFACT}"
rm -f -- "${LOCAL_TEMP_ARTIFACT}"
rmdir -- "${LOCAL_TEMP_ROOT}"
LOCAL_TEMP_ROOT=""
PUBLICATION_COMPLETE=1
rm -f -- "${START_HANDOFF}"
START_HANDOFF=""
revoke_and_remove_session_ssh_key || exit "${SSH_KEY_CLEANUP_FAILURE}"

printf '[ARTEFACT FINAL P15-HOCUDA-P0] %s\n' "${LOCAL_FINAL_ARTIFACT}"
printf '[LOGS JSON/STDOUT/STDERR] %s\n' "${LOCAL_FINAL_LOGS}"
printf '[GCP] Cible exacte certifiée TERMINATED; aucune autre VM mutée.\n'
if [[ "${capacity_exhausted}" == "true" ]]; then
    printf '[CAPACITÉ] capacity_exhausted observé; aucun retry ni tuning exécuté.\n' >&2
    exit 3
fi
