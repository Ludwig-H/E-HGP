#!/usr/bin/env bash
set -Eeuo pipefail

readonly DEFAULT_PROJECT_ID="devpod-gpu-exploration"
readonly DEFAULT_ZONE="europe-west4-a"
readonly DEFAULT_INSTANCE_NAME="ehgp-blackwell-spot"
readonly GUEST_SHUTDOWN_MINUTES=45
readonly STOP_SCRIPT_FAILURE=90
readonly STOP_READBACK_FAILURE=91
readonly STOP_NOT_TERMINATED=92

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly START_SCRIPT="${SCRIPT_DIR}/start_and_verify.sh"
readonly STOP_SCRIPT="${SCRIPT_DIR}/stop_and_verify.sh"

PROJECT_ID="${GCP_PROJECT_ID:-${DEFAULT_PROJECT_ID}}"
ZONE="${GCP_ZONE:-${DEFAULT_ZONE}}"
INSTANCE_NAME="${GCP_INSTANCE_NAME:-${DEFAULT_INSTANCE_NAME}}"
RESULT_DIR="${MORSEHGP3D_PHASE3_RESULT_DIR:-${TMPDIR:-/tmp}/morsehgp3d-phase3-results}"
ASSUME_YES=0

SESSION_CERTIFIED=0
REMOTE_WORKDIR=""
LOCAL_TEMP_RESULT=""
LOCAL_RESULT=""

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

warn() {
    printf '[ATTENTION] %s\n' "$*" >&2
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/run_phase3_qualification.sh --yes [--result-dir RÉPERTOIRE]

Orchestre une qualification réelle de Phase 3, déjà explicitement autorisée,
sur l'unique cible G4 E-HGP. La session est bornée par un arrêt invité à
45 minutes, utilise exclusivement start_and_verify.sh et stop_and_verify.sh,
et ne réussit qu'après une relecture GCE indépendante de l'état TERMINATED.

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
    die "--yes est obligatoire et atteste qu'une session facturable de 45 minutes a été explicitement autorisée."

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
LOCAL_TEMP_RESULT="$(mktemp "${RESULT_DIR}/.phase3-${HEAD_SHA}.XXXXXXXX.partial")" || \
    die "Impossible de créer l'artefact temporaire local."

shell_quote() {
    printf '%q' "$1"
}

remote_exec() {
    local command="$1"
    gcloud compute ssh "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --quiet \
        --ssh-flag='-o ConnectTimeout=15' \
        --ssh-flag='-o BatchMode=yes' \
        --command="${command}"
}

cleanup_remote_best_effort() {
    local quoted_remote=""

    [[ -n "${REMOTE_WORKDIR}" ]] || return 0
    if [[ ! "${REMOTE_WORKDIR}" =~ ^/tmp/morsehgp3d-phase3\.[A-Za-z0-9]{8}$ ]]; then
        warn "Répertoire mktemp distant non canonique; aucun nettoyage distant tenté : ${REMOTE_WORKDIR}."
        return 0
    fi

    quoted_remote="$(shell_quote "${REMOTE_WORKDIR}")"
    if ! remote_exec "rm -rf -- ${quoted_remote}" >/dev/null 2>&1; then
        warn "Nettoyage distant au mieux impossible pour le seul mktemp ${REMOTE_WORKDIR}."
        return 0
    fi
    REMOTE_WORKDIR=""
}

print_control_command() {
    printf 'Commande de contrôle : gcloud compute instances describe %q --project=%q --zone=%q --format=value\\(status\\)\n' \
        "${INSTANCE_NAME}" "${PROJECT_ID}" "${ZONE}" >&2
}

certify_target_stopped() {
    local stop_status=0
    local final_status=""

    set +e
    "${STOP_SCRIPT}" --yes
    stop_status=$?
    set -e
    if ((stop_status != 0)); then
        printf '[ARRÊT NON CERTIFIÉ] Projet=%s zone=%s instance=%s; stop_and_verify.sh a échoué avec le code %s.\n' \
            "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" "${stop_status}" >&2
        print_control_command
        return "${STOP_SCRIPT_FAILURE}"
    fi

    if ! final_status="$(gcloud compute instances describe "${INSTANCE_NAME}" \
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

    SESSION_CERTIFIED=0
    printf '[TERMINATED] Projet=%s zone=%s instance=%s : relecture GCE indépendante certifiée.\n' \
        "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}"
    return 0
}

on_exit() {
    local original_status=$?
    local stop_status=0

    trap - EXIT HUP INT TERM
    if [[ -n "${LOCAL_TEMP_RESULT}" && -e "${LOCAL_TEMP_RESULT}" ]]; then
        rm -f -- "${LOCAL_TEMP_RESULT}" || true
    fi

    if ((SESSION_CERTIFIED == 1)); then
        cleanup_remote_best_effort
        set +e
        certify_target_stopped
        stop_status=$?
        set -e
        if ((stop_status != 0)); then
            exit "${stop_status}"
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

"${START_SCRIPT}" --yes --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}"
SESSION_CERTIFIED=1

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

gcloud compute scp \
    "${INSTANCE_NAME}:${remote_artifact}" \
    "${LOCAL_TEMP_RESULT}" \
    --project="${PROJECT_ID}" \
    --zone="${ZONE}" \
    --quiet \
    --scp-flag='-o ConnectTimeout=15' \
    --scp-flag='-o BatchMode=yes'

[[ -s "${LOCAL_TEMP_RESULT}" ]] || die "Artefact distant récupéré mais vide."
python3 - "${LOCAL_TEMP_RESULT}" <<'PY'
import json
from pathlib import Path
import sys

path = Path(sys.argv[1])
with path.open(encoding="utf-8") as stream:
    value = json.load(stream)
if not isinstance(value, dict):
    raise SystemExit("l'artefact Phase 3 doit être un objet JSON")
PY

mv -- "${LOCAL_TEMP_RESULT}" "${LOCAL_RESULT}"
LOCAL_TEMP_RESULT=""
printf '[ARTEFACT] Résultat Phase 3 publié atomiquement : %s\n' "${LOCAL_RESULT}"

cleanup_remote_best_effort
set +e
certify_target_stopped
stop_status=$?
set -e
if ((stop_status != 0)); then
    exit "${stop_status}"
fi

trap - EXIT HUP INT TERM
printf '[SUCCÈS] Qualification Phase 3 terminée; cible certifiée TERMINATED et artefact conservé : %s\n' \
    "${LOCAL_RESULT}"
