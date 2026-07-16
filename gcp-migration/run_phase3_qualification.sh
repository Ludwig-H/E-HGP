#!/usr/bin/env bash
set -Eeuo pipefail

readonly DEFAULT_PROJECT_ID="devpod-gpu-exploration"
readonly DEFAULT_ZONE="europe-west4-a"
readonly DEFAULT_INSTANCE_NAME="ehgp-blackwell-spot"
readonly GUEST_SHUTDOWN_MINUTES=45
readonly STOP_SCRIPT_FAILURE=90
readonly STOP_READBACK_FAILURE=91
readonly STOP_NOT_TERMINATED=92
readonly GCLOUD_READ_TIMEOUT_SECONDS=30
readonly GCLOUD_TRANSFER_TIMEOUT_SECONDS=120
readonly GCLOUD_REMOTE_TIMEOUT_SECONDS=2880
readonly GCLOUD_KILL_AFTER_SECONDS=10

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly START_SCRIPT="${SCRIPT_DIR}/start_and_verify.sh"
readonly STOP_SCRIPT="${SCRIPT_DIR}/stop_and_verify.sh"

PROJECT_ID="${GCP_PROJECT_ID:-${DEFAULT_PROJECT_ID}}"
ZONE="${GCP_ZONE:-${DEFAULT_ZONE}}"
INSTANCE_NAME="${GCP_INSTANCE_NAME:-${DEFAULT_INSTANCE_NAME}}"
RESULT_DIR="${MORSEHGP3D_PHASE3_RESULT_DIR:-${TMPDIR:-/tmp}/morsehgp3d-phase3-results}"
ASSUME_YES=0

SESSION_CERTIFIED=0
TARGET_STOP_CERTIFIED=0
REMOTE_WORKDIR=""
LOCAL_TEMP_RESULT=""
LOCAL_RESULT=""
START_HANDOFF=""
SESSION_LAST_START_TIMESTAMP=""
FINAL_STATUS=""
FINAL_STOP_VERIFIED_AT_UTC=""

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/run_phase3_qualification.sh --yes [--result-dir RÉPERTOIRE]

Orchestre une qualification réelle de Phase 3, déjà explicitement autorisée,
sur l'unique cible G4 E-HGP. L'arrêt invité est armé pour 45 minutes après la
certification des gardes; le coupe-circuit GCE reste borné séparément. Le script
utilise exclusivement start_and_verify.sh et stop_and_verify.sh, et ne réussit
qu'après une relecture GCE indépendante de l'état TERMINATED.

Le commit local doit être propre et présent sur origin/main. Le script clone ce
SHA exact dans un mktemp distant, appelle phase3_remote_qualification.sh avec
--yes --output, puis récupère son artefact JSON dans le répertoire demandé.
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
    die "--yes est obligatoire et atteste qu'une session facturable avec arrêt invité armé à 45 minutes a été explicitement autorisée."

[[ "${PROJECT_ID}" == "${DEFAULT_PROJECT_ID}" ]] || \
    die "Projet refusé : cette qualification cible uniquement ${DEFAULT_PROJECT_ID}."
[[ "${ZONE}" == "${DEFAULT_ZONE}" ]] || \
    die "Zone refusée : cette qualification cible uniquement ${DEFAULT_ZONE}."
[[ "${INSTANCE_NAME}" == "${DEFAULT_INSTANCE_NAME}" ]] || \
    die "Instance refusée : cette qualification cible uniquement ${DEFAULT_INSTANCE_NAME}."
if [[ -n "${GCP_GUEST_SHUTDOWN_MINUTES+x}" ]]; then
    [[ "${GCP_GUEST_SHUTDOWN_MINUTES}" == "${GUEST_SHUTDOWN_MINUTES}" ]] || \
        die "GCP_GUEST_SHUTDOWN_MINUTES doit valoir exactement ${GUEST_SHUTDOWN_MINUTES}."
fi
[[ -n "${RESULT_DIR}" ]] || die "Le répertoire de résultat ne peut pas être vide."

export GCP_PROJECT_ID="${PROJECT_ID}"
export GCP_ZONE="${ZONE}"
export GCP_INSTANCE_NAME="${INSTANCE_NAME}"
export GCP_GUEST_SHUTDOWN_MINUTES="${GUEST_SHUTDOWN_MINUTES}"

command -v git >/dev/null 2>&1 || die "git est introuvable."
command -v gcloud >/dev/null 2>&1 || die "gcloud est introuvable."
command -v python3 >/dev/null 2>&1 || die "python3 est requis pour valider l'artefact JSON."
command -v timeout >/dev/null 2>&1 || die "GNU timeout est requis avant toute mutation GCP."
timeout_version="$(timeout --version 2>/dev/null | sed -n '1p')" || \
    die "Impossible d'identifier GNU timeout."
[[ "${timeout_version}" == timeout\ \(GNU\ coreutils\)* ]] || \
    die "timeout doit être l'implémentation GNU compatible avec --foreground et --kill-after."
timeout --foreground --kill-after=1s 1s true >/dev/null 2>&1 || \
    die "GNU timeout ne prend pas en charge --foreground et --kill-after."
[[ -x "${START_SCRIPT}" ]] || die "Point d'entrée de démarrage absent ou non exécutable : ${START_SCRIPT}."
[[ -x "${STOP_SCRIPT}" ]] || die "Point d'entrée d'arrêt absent ou non exécutable : ${STOP_SCRIPT}."

REPOSITORY_ROOT="$(git -C "${SCRIPT_DIR}/.." rev-parse --show-toplevel 2>/dev/null)" || \
    die "Impossible d'identifier la racine Git."
[[ -n "${REPOSITORY_ROOT}" ]] || die "Racine Git vide."

worktree_status="$(git -C "${REPOSITORY_ROOT}" status --porcelain --untracked-files=normal)" || \
    die "Impossible de vérifier la propreté du dépôt."
[[ -z "${worktree_status}" ]] || \
    die "Worktree sale : committez ou retirez toutes les modifications et fichiers non suivis avant toute session GCP."

HEAD_SHA="$(git -C "${REPOSITORY_ROOT}" rev-parse HEAD)" || die "HEAD illisible."
[[ "${HEAD_SHA}" =~ ^[0-9a-f]{40}$ ]] || die "SHA HEAD non canonique : ${HEAD_SHA}."

git -C "${REPOSITORY_ROOT}" fetch --quiet --no-tags \
    origin refs/heads/main:refs/remotes/origin/main || \
    die "Impossible de rafraîchir origin/main; démarrage refusé."
git -C "${REPOSITORY_ROOT}" merge-base --is-ancestor \
    "${HEAD_SHA}" refs/remotes/origin/main || \
    die "HEAD ${HEAD_SHA} n'est pas présent sur origin/main; poussez-le avant toute session GCP."

ORIGIN_URL="$(git -C "${REPOSITORY_ROOT}" remote get-url origin)" || \
    die "URL du remote origin illisible."
[[ -n "${ORIGIN_URL}" && "${ORIGIN_URL}" != *$'\n'* && "${ORIGIN_URL}" != *$'\r'* ]] || \
    die "URL du remote origin invalide."
case "${ORIGIN_URL}" in
    https://github.com/Ludwig-H/E-HGP|https://github.com/Ludwig-H/E-HGP.git)
        ;;
    *)
        die "Remote origin refusé : ${ORIGIN_URL}; le dépôt public E-HGP exact est obligatoire."
        ;;
esac

RESULT_DIR="$(python3 - "${RESULT_DIR}" <<'PY'
from pathlib import Path
import sys

print(Path(sys.argv[1]).expanduser().resolve(strict=False))
PY
)" || die "Impossible de résoudre le répertoire de résultat ${RESULT_DIR}."
case "${RESULT_DIR}/" in
    "${REPOSITORY_ROOT}/"*)
        die "Le répertoire de résultat doit rester hors du worktree ${REPOSITORY_ROOT}."
        ;;
esac
mkdir -p -- "${RESULT_DIR}" || die "Impossible de créer le répertoire de résultat ${RESULT_DIR}."
RESULT_DIR="$(cd -- "${RESULT_DIR}" && pwd -P)" || die "Répertoire de résultat illisible."
LOCAL_RESULT="${RESULT_DIR}/phase3-${HEAD_SHA}.json"
[[ ! -e "${LOCAL_RESULT}" ]] || \
    die "L'artefact ${LOCAL_RESULT} existe déjà; utilisez un répertoire de résultat distinct."
START_HANDOFF="${RESULT_DIR}/phase3-${HEAD_SHA}.start-handoff.json"
[[ ! -e "${START_HANDOFF}" && ! -L "${START_HANDOFF}" ]] || \
    die "Le témoin de handoff existe déjà : ${START_HANDOFF}; résolvez d'abord cette session ciblée."
LOCAL_TEMP_RESULT="$(mktemp "${RESULT_DIR}/.phase3-${HEAD_SHA}.XXXXXXXX.partial")" || \
    die "Impossible de créer l'artefact temporaire local."

shell_quote() {
    printf '%q' "$1"
}

remote_exec() {
    local command="$1"
    timeout --foreground --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_REMOTE_TIMEOUT_SECONDS}s" gcloud compute ssh "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --quiet \
        --ssh-flag='-o ConnectTimeout=15' \
        --ssh-flag='-o BatchMode=yes' \
        --command="${command}"
}

load_targeted_handoff() {
    local generation=""
    [[ -n "${START_HANDOFF}" && -f "${START_HANDOFF}" && ! -L "${START_HANDOFF}" ]] || return 1
    generation="$(python3 - "${START_HANDOFF}" "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" \
        "${GUEST_SHUTDOWN_MINUTES}" <<'PY'
import json
from pathlib import Path
import sys

with Path(sys.argv[1]).open(encoding="utf-8") as stream:
    value = json.load(stream)
expected = {
    "guest_shutdown_minutes": int(sys.argv[5]),
    "instance": sys.argv[4],
    "project": sys.argv[2],
    "schema": "e-hgp.start-handoff.v3",
    "status": "targeted_running",
    "zone": sys.argv[3],
}
if not isinstance(value, dict) or set(value) != set(expected) | {"last_start_timestamp"}:
    raise SystemExit("invalid targeted start handoff keys")
generation = value.pop("last_start_timestamp")
if value != expected:
    raise SystemExit("invalid targeted start handoff")
if not isinstance(generation, str) or not generation or "\n" in generation or "\r" in generation:
    raise SystemExit("invalid targeted start generation")
print(generation)
PY
)" || return 1
    [[ -n "${generation}" ]] || return 1
    SESSION_LAST_START_TIMESTAMP="${generation}"
}

print_control_command() {
    printf 'Commande de contrôle : gcloud compute instances describe %q --project=%q --zone=%q --format=%q\n' \
        "${INSTANCE_NAME}" "${PROJECT_ID}" "${ZONE}" \
        'value(status,lastStartTimestamp)' >&2
    if [[ -n "${SESSION_LAST_START_TIMESTAMP}" ]]; then
        printf 'Commande d’arrêt ciblé : GCP_PROJECT_ID=%q GCP_ZONE=%q GCP_INSTANCE_NAME=%q %q --yes --expected-last-start-timestamp %q\n' \
            "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" "${STOP_SCRIPT}" \
            "${SESSION_LAST_START_TIMESTAMP}" >&2
    fi
}

certify_target_stopped() {
    local stop_status=0
    local final_status=""
    local final_generation=""

    if [[ -z "${SESSION_LAST_START_TIMESTAMP}" ]]; then
        printf '[ARRÊT NON CERTIFIÉ] Projet=%s zone=%s instance=%s; génération lastStartTimestamp absente, aucune mutation automatique autorisée.\n' \
            "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" >&2
        print_control_command
        return "${STOP_SCRIPT_FAILURE}"
    fi

    if "${STOP_SCRIPT}" --yes \
        --expected-last-start-timestamp "${SESSION_LAST_START_TIMESTAMP}"; then
        stop_status=0
    else
        stop_status=$?
    fi
    if ((stop_status != 0)); then
        printf '[ARRÊT NON CERTIFIÉ] Projet=%s zone=%s instance=%s; stop_and_verify.sh a échoué avec le code %s.\n' \
            "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" "${stop_status}" >&2
        print_control_command
        return "${STOP_SCRIPT_FAILURE}"
    fi

    if ! final_status="$(timeout --foreground --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --format='value(status)' 2>/dev/null)"; then
        printf '[ARRÊT ILLISIBLE] Projet=%s zone=%s instance=%s; la relecture GCE indépendante a échoué.\n' \
            "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" >&2
        print_control_command
        return "${STOP_READBACK_FAILURE}"
    fi
    if [[ "${final_status}" != "TERMINATED" ]]; then
        printf '[ARRÊT NON CERTIFIÉ] Projet=%s zone=%s instance=%s; dernier état connu=%s, TERMINATED attendu.\n' \
            "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" "${final_status:-vide}" >&2
        print_control_command
        return "${STOP_NOT_TERMINATED}"
    fi
    if ! final_generation="$(timeout --foreground --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --format='value(lastStartTimestamp)' 2>/dev/null)"; then
        printf '[ARRÊT ILLISIBLE] Projet=%s zone=%s instance=%s; la génération finale est illisible.\n' \
            "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" >&2
        print_control_command
        return "${STOP_READBACK_FAILURE}"
    fi
    if [[ "${final_generation}" != "${SESSION_LAST_START_TIMESTAMP}" ]]; then
        printf '[ARRÊT NON CERTIFIÉ] Projet=%s zone=%s instance=%s; lastStartTimestamp final=%s, attendu=%s.\n' \
            "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" \
            "${final_generation:-vide}" "${SESSION_LAST_START_TIMESTAMP}" >&2
        print_control_command
        return "${STOP_NOT_TERMINATED}"
    fi

    FINAL_STATUS="${final_status}"
    FINAL_STOP_VERIFIED_AT_UTC="$(date -u +'%Y-%m-%dT%H:%M:%SZ')"
    SESSION_CERTIFIED=0
    TARGET_STOP_CERTIFIED=1
    printf '[TERMINATED] Projet=%s zone=%s instance=%s : relecture GCE indépendante certifiée.\n' \
        "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}"
    return 0
}

on_exit() {
    local original_status=$?
    local stop_status=0

    trap - EXIT HUP INT TERM
    if ((TARGET_STOP_CERTIFIED == 0)) && [[ -z "${SESSION_LAST_START_TIMESTAMP}" ]] && \
        load_targeted_handoff 2>/dev/null; then
        SESSION_CERTIFIED=1
    fi

    if ((TARGET_STOP_CERTIFIED == 0 && SESSION_CERTIFIED == 1)); then
        if certify_target_stopped; then
            stop_status=0
        else
            stop_status=$?
        fi
        if ((stop_status != 0)); then
            if [[ -n "${LOCAL_TEMP_RESULT}" && -e "${LOCAL_TEMP_RESULT}" ]]; then
                rm -f -- "${LOCAL_TEMP_RESULT}" || true
            fi
            if [[ -n "${START_HANDOFF}" && -e "${START_HANDOFF}" ]]; then
                printf '[HANDOFF CONSERVÉ] Arrêt non certifié; preuve ciblée à conserver jusqu’à résolution : %s\n' \
                    "${START_HANDOFF}" >&2
            fi
            exit "${stop_status}"
        fi
    fi
    if [[ -n "${LOCAL_TEMP_RESULT}" && -e "${LOCAL_TEMP_RESULT}" ]]; then
        rm -f -- "${LOCAL_TEMP_RESULT}" || true
    fi
    if [[ -n "${START_HANDOFF}" && -e "${START_HANDOFF}" ]]; then
        if ((TARGET_STOP_CERTIFIED == 1)); then
            rm -f -- "${START_HANDOFF}" || true
        else
            printf '[HANDOFF CONSERVÉ] Session non certifiée arrêtée; preuve opérationnelle : %s\n' \
                "${START_HANDOFF}" >&2
        fi
    fi
    exit "${original_status}"
}

trap on_exit EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

worktree_status="$(git -C "${REPOSITORY_ROOT}" status --porcelain --untracked-files=normal)" || \
    die "Impossible de revérifier la propreté du dépôt juste avant le démarrage."
[[ -z "${worktree_status}" ]] || \
    die "Le worktree a changé pendant la préparation; démarrage refusé."

printf '%s\n' \
    "[SESSION PHASE 3] Autorisation non interactive confirmée :" \
    "  projet          : ${PROJECT_ID}" \
    "  zone            : ${ZONE}" \
    "  instance        : ${INSTANCE_NAME}" \
    "  SHA             : ${HEAD_SHA}" \
    "  arrêt invité    : ${GUEST_SHUTDOWN_MINUTES} min" \
    "  résultat local  : ${LOCAL_RESULT}"

"${START_SCRIPT}" --yes \
    --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}" \
    --handoff-file "${START_HANDOFF}"
SESSION_CERTIFIED=1
load_targeted_handoff || die "Le démarrage n'a pas publié son témoin ciblé certifié."

mktemp_output="$(remote_exec \
    'remote_dir=$(mktemp -d /tmp/morsehgp3d-phase3.XXXXXXXX) && printf "__EHGP_REMOTE_DIR__%s\n" "${remote_dir}"')"
REMOTE_WORKDIR="$(printf '%s\n' "${mktemp_output}" | sed -n 's/^__EHGP_REMOTE_DIR__//p' | tail -n 1)"
[[ "${REMOTE_WORKDIR}" =~ ^/tmp/morsehgp3d-phase3\.[A-Za-z0-9]{8}$ ]] || \
    die "mktemp distant absent ou ambigu; valeur reçue : ${REMOTE_WORKDIR:-vide}."

remote_repository="${REMOTE_WORKDIR}/repository"
remote_artifact="${REMOTE_WORKDIR}/phase3-result.json"
quoted_origin="$(shell_quote "${ORIGIN_URL}")"
quoted_repository="$(shell_quote "${remote_repository}")"
quoted_head="$(shell_quote "${HEAD_SHA}")"
quoted_artifact="$(shell_quote "${remote_artifact}")"

clone_output="$(remote_exec \
    "git clone --quiet --single-branch --branch main ${quoted_origin} ${quoted_repository} && git -C ${quoted_repository} checkout --quiet --detach ${quoted_head} && remote_head=\$(git -C ${quoted_repository} rev-parse HEAD) && printf '__EHGP_REMOTE_HEAD__%s\\n' \"\${remote_head}\"")"
remote_head="$(printf '%s\n' "${clone_output}" | sed -n 's/^__EHGP_REMOTE_HEAD__//p' | tail -n 1)"
[[ "${remote_head}" == "${HEAD_SHA}" ]] || \
    die "HEAD distant ${remote_head:-illisible} différent du SHA local ${HEAD_SHA}."

remote_exec \
    "test -x ${quoted_repository}/gcp-migration/phase3_remote_qualification.sh && cd ${quoted_repository} && ./gcp-migration/phase3_remote_qualification.sh --yes --output ${quoted_artifact}"

timeout --foreground --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
    "${GCLOUD_TRANSFER_TIMEOUT_SECONDS}s" gcloud compute scp \
    "${INSTANCE_NAME}:${remote_artifact}" \
    "${LOCAL_TEMP_RESULT}" \
    --project="${PROJECT_ID}" \
    --zone="${ZONE}" \
    --quiet \
    --scp-flag='-o ConnectTimeout=15' \
    --scp-flag='-o BatchMode=yes'

[[ -s "${LOCAL_TEMP_RESULT}" ]] || die "Artefact distant récupéré mais vide."
python3 - "${LOCAL_TEMP_RESULT}" "${HEAD_SHA}" <<'PY'
import json
from pathlib import Path
import re
import sys

path = Path(sys.argv[1])
with path.open(encoding="utf-8") as stream:
    value = json.load(stream)
if not isinstance(value, dict) or value.get("schema") != "morsehgp3d.phase3.qualification.v1":
    raise SystemExit("l'artefact Phase 3 doit être un objet du schéma de qualification v1")
if value.get("status") != "worker_passed_pending_shutdown":
    raise SystemExit("l'artefact Phase 3 distant n'attend pas la certification d'arrêt")
for key, expected in {
    "backend": "cuda_g4",
    "mode": "certified",
    "phase": "3",
    "profile": "hgp_reduced",
    "scientific_scope": "environment_reproducibility_only",
}.items():
    if value.get(key) != expected:
        raise SystemExit(f"champ Phase 3 distant invalide: {key}")
if "public_status" in value:
    raise SystemExit("une qualification d'environnement ne doit pas publier public_status")
if value.get("scientific_result_claimed") is not False:
    raise SystemExit("l'artefact distant revendique à tort un résultat scientifique")
if value.get("scientific_public_status") is not None:
    raise SystemExit("l'artefact distant expose à tort un statut public scientifique")
git = value.get("git")
if git != {"clean": True, "sha": sys.argv[2]}:
    raise SystemExit("l'artefact distant ne correspond pas au commit propre qualifié")
image = value.get("image")
if not isinstance(image, dict):
    raise SystemExit("identité d'image absente de l'artefact distant")
if image.get("ref") != f"morsehgp3d-phase3:{sys.argv[2]}":
    raise SystemExit("tag d'image distant incohérent")
if re.fullmatch(r"sha256:[0-9a-f]{64}", str(image.get("id"))) is None:
    raise SystemExit("identifiant d'image distant non canonique")
lifecycle = value.get("vm_lifecycle")
if not isinstance(lifecycle, dict) or lifecycle.get("worker_mutates_gcp") is not False:
    raise SystemExit("contrat de cycle de vie du worker absent")
PY

if certify_target_stopped; then
    stop_status=0
else
    stop_status=$?
fi
if ((stop_status != 0)); then
    exit "${stop_status}"
fi

python3 - "${LOCAL_TEMP_RESULT}" "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" \
    "${GUEST_SHUTDOWN_MINUTES}" "${FINAL_STATUS}" \
    "${FINAL_STOP_VERIFIED_AT_UTC}" "${SESSION_LAST_START_TIMESTAMP}" <<'PY'
import json
import os
from pathlib import Path
import sys

path = Path(sys.argv[1])
with path.open(encoding="utf-8") as stream:
    value = json.load(stream)
lifecycle = value["vm_lifecycle"]
lifecycle["worker_status_before_targeted_stop"] = value["status"]
lifecycle.update(
    {
        "final_status": sys.argv[6],
        "final_status_readback": "gcloud_compute_instances_describe",
        "final_status_verified_at_utc": sys.argv[7],
        "guest_shutdown_minutes": int(sys.argv[5]),
        "initial_status": "TERMINATED",
        "initial_status_basis": "start_and_verify_precondition",
        "instance": sys.argv[4],
        "last_start_timestamp": sys.argv[8],
        "project": sys.argv[2],
        "start_handoff_schema": "e-hgp.start-handoff.v3",
        "targeted_stop_verified": True,
        "zone": sys.argv[3],
    }
)
value["status"] = "passed"
encoded = json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
with path.open("w", encoding="utf-8", newline="\n") as stream:
    stream.write(encoded)
    stream.write("\n")
    stream.flush()
    os.fsync(stream.fileno())
PY

python3 - "${LOCAL_TEMP_RESULT}" "${LOCAL_RESULT}" <<'PY'
import os
from pathlib import Path
import sys

temporary = Path(sys.argv[1])
target = Path(sys.argv[2])
try:
    os.link(temporary, target, follow_symlinks=False)
except FileExistsError:
    raise SystemExit(f"refus de remplacer un artefact créé concurremment : {target}")
os.unlink(temporary)
descriptor = os.open(target.parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
try:
    os.fsync(descriptor)
finally:
    os.close(descriptor)
PY
LOCAL_TEMP_RESULT=""
rm -f -- "${START_HANDOFF}"
START_HANDOFF=""
printf '[ARTEFACT] Résultat Phase 3 publié après certification TERMINATED : %s\n' "${LOCAL_RESULT}"

trap - EXIT HUP INT TERM
printf '[SUCCÈS] Qualification Phase 3 terminée; cible certifiée TERMINATED et artefact conservé : %s\n' \
    "${LOCAL_RESULT}"
