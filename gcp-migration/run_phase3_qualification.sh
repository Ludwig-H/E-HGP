#!/usr/bin/env bash
set -Eeuo pipefail

readonly DEFAULT_PROJECT_ID="devpod-gpu-exploration"
readonly DEFAULT_ZONE="europe-west4-a"
readonly DEFAULT_INSTANCE_NAME="ehgp-blackwell-spot"
readonly AI_CAPACITY_ZONE="europe-west4-ai1a"
readonly AI_CAPACITY_INSTANCE_NAME="ehgp-blackwell-spot-ai1a"
readonly GUEST_SHUTDOWN_MINUTES=45
readonly EXPECTED_MAX_RUN_SECONDS=3600
readonly WORK_RESERVE_SECONDS=1800
readonly TIMESTAMP_TOLERANCE_SECONDS=300
readonly STOP_SCRIPT_FAILURE=90
readonly STOP_READBACK_FAILURE=91
readonly STOP_NOT_TERMINATED=92
readonly SSH_KEY_CLEANUP_FAILURE=93
readonly GCLOUD_READ_TIMEOUT_SECONDS=30
readonly GCLOUD_IDENTITY_TIMEOUT_SECONDS=30
readonly GCLOUD_TRANSFER_TIMEOUT_SECONDS=120
readonly GCLOUD_REMOTE_TIMEOUT_SECONDS=2880
readonly GCLOUD_KILL_AFTER_SECONDS=10
readonly SSH_KEY_TTL="70m"
readonly SSH_KEY_TTL_SLACK_SECONDS=660

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
readonly START_SCRIPT="${SCRIPT_DIR}/start_and_verify.sh"
readonly STOP_SCRIPT="${SCRIPT_DIR}/stop_and_verify.sh"
readonly DOCKER_PROVISION_SCRIPT="${SCRIPT_DIR}/phase3_remote_docker_provision.sh"

PROJECT_ID="${GCP_PROJECT_ID:-${DEFAULT_PROJECT_ID}}"
ZONE="${GCP_ZONE:-${DEFAULT_ZONE}}"
INSTANCE_NAME="${GCP_INSTANCE_NAME:-${DEFAULT_INSTANCE_NAME}}"
RESULT_DIR="${MORSEHGP3D_PHASE3_RESULT_DIR:-${TMPDIR:-/tmp}/morsehgp3d-phase3-results}"
ASSUME_YES=0
PROVISION_DOCKER=0
PHASE4_SPATIAL_REFERENCE=0
PHASE5_K1_BORUVKA=0

SESSION_CERTIFIED=0
TARGET_STOP_CERTIFIED=0
REMOTE_WORKDIR=""
LOCAL_TEMP_RESULT=""
LOCAL_RESULT=""
LOCAL_PHASE4_TEMP_RESULT=""
LOCAL_PHASE4_RESULT=""
LOCAL_PHASE5_TEMP_RESULT=""
LOCAL_PHASE5_RESULT=""
START_HANDOFF=""
SESSION_LAST_START_TIMESTAMP=""
SESSION_HANDOFF_STATUS=""
FINAL_STATUS=""
FINAL_STOP_VERIFIED_AT_UTC=""
EFFECTIVE_GCE_DEADLINE_EPOCH=""
SSH_KEY_DIR=""
SSH_KEY_FILE=""
SSH_KEY_EXPIRATION_UTC=""
SSH_KEY_IMPORT_ATTEMPTED=0
START_INVOCATION_ATTEMPTED=0
PRE_START_GENERATION_JSON=""
PRE_START_SNAPSHOT_CERTIFIED=0

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/run_phase3_qualification.sh --yes [--provision-docker] [--phase4-spatial-reference] [--phase5-k1-boruvka] [--result-dir RÉPERTOIRE]

Orchestre une qualification réelle de Phase 3, déjà explicitement autorisée,
sur l'un des deux couples G4 E-HGP explicitement admis. L'arrêt invité est armé pour 45 minutes après la
certification des gardes; le coupe-circuit GCE reste borné séparément. Le script
utilise exclusivement start_and_verify.sh et stop_and_verify.sh, et ne réussit
qu'après une relecture GCE indépendante de l'état TERMINATED.

Le commit local doit être propre et présent sur origin/main. Le script clone ce
SHA exact dans un mktemp distant, appelle phase3_remote_qualification.sh avec
--yes --output, puis récupère son artefact JSON dans le répertoire demandé.
Il génère hors du dépôt une clé ED25519 de session non chiffrée, l'inscrit dans
OS Login pour 70 minutes seulement, transmet explicitement à SSH/SCP la clé et
son expiration UTC fixe, puis la révoque et supprime la copie locale après la
certification TERMINATED de la génération ciblée.

--phase4-spatial-reference ajoute dans la même session gardée la qualification
de la référence spatiale exhaustive et du LBVH résident Phase 4. Leur artefact
compagnon commun reste provisoire jusqu'à la même certification ciblée
TERMINATED et ne publie aucun statut scientifique.

--phase5-k1-boruvka ajoute dans la même session gardée la qualification de la
boucle GPU Boruvka complète de Phase 5 : chaîne multi-ronde proposée sur GPU,
décisions et contractions exactes sur CPU, rejeu GPU indépendant, témoin EMST
local, audit AOT sm_120 sans PTX, memcheck et racecheck. Son artefact compagnon
reste provisoire jusqu'à la même certification ciblée TERMINATED et ne publie
aucun statut scientifique.

--provision-docker autorise, après certification des deux coupe-circuits, le
provisionneur invité séparé à installer docker.io et docker-buildx depuis les
dépôts Ubuntu déjà configurés, puis à configurer le runtime NVIDIA. Le worker
de qualification reste non mutatif. Après la préparation, la garde GCE est
recertifiée avant le worker.
EOF
}

while (($# > 0)); do
    case "$1" in
        --yes)
            ASSUME_YES=1
            shift
            ;;
        --provision-docker)
            PROVISION_DOCKER=1
            shift
            ;;
        --phase4-spatial-reference)
            PHASE4_SPATIAL_REFERENCE=1
            shift
            ;;
        --phase5-k1-boruvka)
            PHASE5_K1_BORUVKA=1
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
case "${ZONE}/${INSTANCE_NAME}" in
    "${DEFAULT_ZONE}/${DEFAULT_INSTANCE_NAME}"|\
    "${AI_CAPACITY_ZONE}/${AI_CAPACITY_INSTANCE_NAME}")
        ;;
    *)
        die "Cible refusée : couples autorisés ${DEFAULT_ZONE}/${DEFAULT_INSTANCE_NAME} ou ${AI_CAPACITY_ZONE}/${AI_CAPACITY_INSTANCE_NAME}."
        ;;
esac
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
command -v ssh-keygen >/dev/null 2>&1 || die "ssh-keygen est requis pour la clé de session expirante."
command -v timeout >/dev/null 2>&1 || die "GNU timeout est requis avant toute mutation GCP."
timeout_version="$(timeout --version 2>/dev/null | sed -n '1p')" || \
    die "Impossible d'identifier GNU timeout."
[[ "${timeout_version}" == timeout\ \(GNU\ coreutils\)* ]] || \
    die "timeout doit être l'implémentation GNU compatible avec la gestion du groupe de processus et --kill-after."
timeout --kill-after=1s 1s true >/dev/null 2>&1 || \
    die "GNU timeout ne prend pas en charge la gestion du groupe de processus et --kill-after."
[[ -x "${START_SCRIPT}" ]] || die "Point d'entrée de démarrage absent ou non exécutable : ${START_SCRIPT}."
[[ -x "${STOP_SCRIPT}" ]] || die "Point d'entrée d'arrêt absent ou non exécutable : ${STOP_SCRIPT}."
if ((PROVISION_DOCKER == 1)); then
    [[ -x "${DOCKER_PROVISION_SCRIPT}" ]] || \
        die "Provisionneur Docker absent ou non exécutable : ${DOCKER_PROVISION_SCRIPT}."
fi

REPOSITORY_ROOT="$(git -C "${SCRIPT_DIR}/.." rev-parse --show-toplevel 2>/dev/null)" || \
    die "Impossible d'identifier la racine Git."
[[ -n "${REPOSITORY_ROOT}" ]] || die "Racine Git vide."
REPOSITORY_ROOT="$(cd -- "${REPOSITORY_ROOT}" && pwd -P)" || \
    die "Impossible de canoniser la racine Git."

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
[[ ! -e "${LOCAL_RESULT}" && ! -L "${LOCAL_RESULT}" ]] || \
    die "L'artefact ${LOCAL_RESULT} existe déjà; utilisez un répertoire de résultat distinct."
START_HANDOFF="${RESULT_DIR}/phase3-${HEAD_SHA}.start-handoff.json"
[[ ! -e "${START_HANDOFF}" && ! -L "${START_HANDOFF}" ]] || \
    die "Le témoin de handoff existe déjà : ${START_HANDOFF}; résolvez d'abord cette session ciblée."
if ((PHASE4_SPATIAL_REFERENCE == 1)); then
    LOCAL_PHASE4_RESULT="${RESULT_DIR}/phase4-spatial-${HEAD_SHA}.json"
    [[ ! -e "${LOCAL_PHASE4_RESULT}" && ! -L "${LOCAL_PHASE4_RESULT}" ]] || \
        die "L'artefact ${LOCAL_PHASE4_RESULT} existe déjà; utilisez un répertoire distinct."
fi
if ((PHASE5_K1_BORUVKA == 1)); then
    LOCAL_PHASE5_RESULT="${RESULT_DIR}/phase5-k1-boruvka-${HEAD_SHA}.json"
    [[ ! -e "${LOCAL_PHASE5_RESULT}" && ! -L "${LOCAL_PHASE5_RESULT}" ]] || \
        die "L'artefact ${LOCAL_PHASE5_RESULT} existe déjà; utilisez un répertoire distinct."
fi
LOCAL_TEMP_RESULT="$(mktemp "${RESULT_DIR}/.phase3-${HEAD_SHA}.XXXXXXXX.partial")" || \
    die "Impossible de créer l'artefact temporaire local."
if ((PHASE4_SPATIAL_REFERENCE == 1)); then
    if ! LOCAL_PHASE4_TEMP_RESULT="$(mktemp \
        "${RESULT_DIR}/.phase4-spatial-${HEAD_SHA}.XXXXXXXX.partial")"; then
        rm -f -- "${LOCAL_TEMP_RESULT}"
        LOCAL_TEMP_RESULT=""
        die "Impossible de créer l'artefact Phase 4 temporaire local."
    fi
fi
if ((PHASE5_K1_BORUVKA == 1)); then
    if ! LOCAL_PHASE5_TEMP_RESULT="$(mktemp \
        "${RESULT_DIR}/.phase5-k1-boruvka-${HEAD_SHA}.XXXXXXXX.partial")"; then
        rm -f -- "${LOCAL_TEMP_RESULT}"
        LOCAL_TEMP_RESULT=""
        if [[ -n "${LOCAL_PHASE4_TEMP_RESULT}" ]]; then
            rm -f -- "${LOCAL_PHASE4_TEMP_RESULT}"
            LOCAL_PHASE4_TEMP_RESULT=""
        fi
        die "Impossible de créer l'artefact Phase 5 temporaire local."
    fi
fi

shell_quote() {
    printf '%q' "$1"
}

remote_exec() {
    local command="$1"
    timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_REMOTE_TIMEOUT_SECONDS}s" gcloud compute ssh "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --quiet \
        --ssh-key-file="${SSH_KEY_FILE}" \
        --ssh-key-expiration="${SSH_KEY_EXPIRATION_UTC}" \
        --ssh-flag='-o ConnectTimeout=15' \
        --ssh-flag='-o BatchMode=yes' \
        --command="${command}"
}

create_session_ssh_key() {
    local declared_public=""
    local derived_public=""
    local created_dir=""
    local key_mode=""
    local old_umask=""

    old_umask="$(umask)" || return 1
    umask 077
    created_dir="$(mktemp -d "${TMPDIR:-/tmp}/morsehgp3d-phase3-ssh.XXXXXXXX")" || {
        umask "${old_umask}"
        return 1
    }
    umask "${old_umask}"
    SSH_KEY_DIR="${created_dir}"
    SSH_KEY_FILE="${SSH_KEY_DIR}/id_ed25519"
    [[ -d "${SSH_KEY_DIR}" && ! -L "${SSH_KEY_DIR}" ]] || return 1
    SSH_KEY_DIR="$(cd -- "${SSH_KEY_DIR}" && pwd -P)" || return 1
    [[ -n "${SSH_KEY_DIR}" && "${SSH_KEY_DIR}" != *$'\n'* && \
        "${SSH_KEY_DIR}" != *$'\r'* ]] || return 1
    SSH_KEY_FILE="${SSH_KEY_DIR}/id_ed25519"
    case "${SSH_KEY_DIR}/" in
        "${REPOSITORY_ROOT}/"*)
            printf '[ERREUR] Le répertoire de clé de session doit rester hors du worktree : %s.\n' \
                "${SSH_KEY_DIR}" >&2
            return 1
            ;;
    esac
    key_mode="$(stat -c '%a' -- "${SSH_KEY_DIR}" 2>/dev/null)" || return 1
    [[ "${key_mode}" == "700" ]] || {
        printf '[ERREUR] Le répertoire de clé de session doit être en mode 700 (reçu=%s).\n' \
            "${key_mode:-illisible}" >&2
        return 1
    }
    ssh-keygen -q -t ed25519 -N '' \
        -C "morsehgp3d-phase3-${HEAD_SHA}" \
        -f "${SSH_KEY_FILE}" || return 1
    [[ -f "${SSH_KEY_FILE}" && ! -L "${SSH_KEY_FILE}" && \
        -f "${SSH_KEY_FILE}.pub" && ! -L "${SSH_KEY_FILE}.pub" ]] || return 1
    chmod 600 -- "${SSH_KEY_FILE}" || return 1
    key_mode="$(stat -c '%a' -- "${SSH_KEY_FILE}" 2>/dev/null)" || return 1
    [[ "${key_mode}" == "600" ]] || {
        printf '[ERREUR] La clé privée de session doit être en mode 600 (reçu=%s).\n' \
            "${key_mode:-illisible}" >&2
        return 1
    }
    declared_public="$(awk 'NF >= 2 {print $1 " " $2; exit}' "${SSH_KEY_FILE}.pub")" || return 1
    [[ "${declared_public}" == "ssh-ed25519 "* ]] || return 1
    derived_public="$(ssh-keygen -y -P '' -f "${SSH_KEY_FILE}" 2>/dev/null)" || return 1
    derived_public="$(awk 'NF >= 2 {print $1 " " $2; exit}' <<<"${derived_public}")"
    [[ "${derived_public}" == "${declared_public}" ]] || return 1
    export GCP_SSH_KEY_FILE="${SSH_KEY_FILE}"
    printf '[CLÉ SSH] Clé ED25519 de session locale créée dans un répertoire privé hors dépôt.\n'
}

capture_session_ssh_key_expiration() {
    local declared_algorithm=""
    local declared_blob=""
    local expiration_fields=""
    local profile_json=""
    local remaining_seconds=""

    read -r declared_algorithm declared_blob _ <"${SSH_KEY_FILE}.pub" || return 1
    [[ "${declared_algorithm}" == "ssh-ed25519" && -n "${declared_blob}" ]] || return 1
    profile_json="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_IDENTITY_TIMEOUT_SECONDS}s" \
        gcloud compute os-login describe-profile \
        --project="${PROJECT_ID}" \
        --format=json)" || return 1
    expiration_fields="$(python3 - \
        "${declared_algorithm}" "${declared_blob}" \
        "${EXPECTED_MAX_RUN_SECONDS}" "${SSH_KEY_TTL_SLACK_SECONDS}" \
        "${profile_json}" <<'PY'
from datetime import datetime, timezone
import json
import sys
import time

algorithm = sys.argv[1]
blob = sys.argv[2]
minimum = int(sys.argv[3])
maximum = minimum + int(sys.argv[4])
value = json.loads(sys.argv[5])
keys = value.get("sshPublicKeys") if isinstance(value, dict) else None
if not isinstance(keys, dict):
    raise SystemExit("profil OS Login sans clés")
matches = []
for record in keys.values():
    if not isinstance(record, dict):
        continue
    fields = str(record.get("key", "")).split()
    if len(fields) < 2 or fields[0] != algorithm or fields[1] != blob:
        continue
    expiration = record.get("expirationTimeUsec")
    if isinstance(expiration, bool):
        continue
    try:
        expiration_usec = int(expiration)
    except (TypeError, ValueError):
        continue
    matches.append(expiration_usec)
if len(matches) != 1:
    raise SystemExit("clé de session absente, dupliquée ou sans expiration OS Login")
expiration_usec = matches[0]
remaining = (expiration_usec - time.time_ns() // 1_000) // 1_000_000
if remaining < minimum or remaining > maximum:
    raise SystemExit(
        f"durée OS Login restante hors borne: {remaining}s, attendu {minimum}..{maximum}s"
    )
seconds, microseconds = divmod(expiration_usec, 1_000_000)
expiration = datetime.fromtimestamp(seconds, timezone.utc).replace(
    microsecond=microseconds
)
expiration_utc = expiration.isoformat(timespec="microseconds").replace("+00:00", "Z")
print(f"{remaining}\t{expiration_utc}")
PY
)" || return 1
    IFS=$'\t' read -r remaining_seconds SSH_KEY_EXPIRATION_UTC <<<"${expiration_fields}"
    [[ "${remaining_seconds}" =~ ^[0-9]+$ && \
        "${SSH_KEY_EXPIRATION_UTC}" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{6}Z$ ]] || return 1
    printf '[CLÉ SSH] Expiration OS Login fixe certifiée : %s (reste=%ss).\n' \
        "${SSH_KEY_EXPIRATION_UTC}" "${remaining_seconds}"
}

import_session_ssh_key() {
    SSH_KEY_IMPORT_ATTEMPTED=1
    timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_IDENTITY_TIMEOUT_SECONDS}s" \
        gcloud compute os-login ssh-keys add \
        --key-file="${SSH_KEY_FILE}.pub" \
        --ttl="${SSH_KEY_TTL}" \
        --project="${PROJECT_ID}" \
        --quiet >/dev/null || return 1
    capture_session_ssh_key_expiration || return 1
    printf '[CLÉ SSH] Clé de session inscrite dans OS Login avec TTL=%s.\n' \
        "${SSH_KEY_TTL}"
}

revoke_and_remove_session_ssh_key() {
    local cleanup_failed=0

    [[ -n "${SSH_KEY_DIR}" || -n "${SSH_KEY_FILE}" ]] || return 0
    [[ -n "${SSH_KEY_DIR}" && "${SSH_KEY_FILE}" == "${SSH_KEY_DIR}/id_ed25519" ]] || {
        printf '[ERREUR] Chemins de clé de session incohérents; suppression locale refusée : dir=%s key=%s.\n' \
            "${SSH_KEY_DIR:-vide}" "${SSH_KEY_FILE:-vide}" >&2
        return 1
    }
    [[ -d "${SSH_KEY_DIR}" && ! -L "${SSH_KEY_DIR}" ]] || {
        printf '[ERREUR] Répertoire de clé de session absent ou remplacé : %s.\n' \
            "${SSH_KEY_DIR}" >&2
        return 1
    }

    if ((SSH_KEY_IMPORT_ATTEMPTED == 1)) && \
        [[ -f "${SSH_KEY_FILE}.pub" && ! -L "${SSH_KEY_FILE}.pub" ]]; then
        if timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
            "${GCLOUD_IDENTITY_TIMEOUT_SECONDS}s" \
            gcloud compute os-login ssh-keys remove \
            --key-file="${SSH_KEY_FILE}.pub" \
            --project="${PROJECT_ID}" \
            --quiet >/dev/null 2>&1; then
            printf '[CLÉ SSH] Clé de session révoquée dans OS Login.\n'
        else
            printf '[AVERTISSEMENT] Révocation OS Login illisible; la clé reste bornée par son TTL initial de %s et sa copie privée locale va être détruite.\n' \
                "${SSH_KEY_TTL}" >&2
        fi
    fi

    if [[ -e "${SSH_KEY_FILE}" || -L "${SSH_KEY_FILE}" ]]; then
        rm -f -- "${SSH_KEY_FILE}" || cleanup_failed=1
    fi
    if [[ -e "${SSH_KEY_FILE}.pub" || -L "${SSH_KEY_FILE}.pub" ]]; then
        rm -f -- "${SSH_KEY_FILE}.pub" || cleanup_failed=1
    fi
    if ((cleanup_failed == 0)); then
        rmdir -- "${SSH_KEY_DIR}" || cleanup_failed=1
    fi
    if ((cleanup_failed != 0)); then
        printf '[ERREUR] Suppression locale de la clé de session non certifiée : %s.\n' \
            "${SSH_KEY_DIR}" >&2
        return 1
    fi

    unset GCP_SSH_KEY_FILE
    SSH_KEY_DIR=""
    SSH_KEY_FILE=""
    SSH_KEY_EXPIRATION_UTC=""
    SSH_KEY_IMPORT_ATTEMPTED=0
    printf '[CLÉ SSH] Copie privée locale de session supprimée.\n'
}

capture_pre_start_snapshot() {
    local lifecycle_json=""

    lifecycle_json="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --format=json)" || return 1
    PRE_START_GENERATION_JSON="$(python3 - "${lifecycle_json}" <<'PY'
import json
import sys

value = json.loads(sys.argv[1])
if not isinstance(value, dict) or value.get("status") != "TERMINATED":
    raise SystemExit("la cible n'est pas TERMINATED avant l'invocation")
generation = value.get("lastStartTimestamp")
if generation in (None, ""):
    generation = None
elif not isinstance(generation, str) or "\n" in generation or "\r" in generation:
    raise SystemExit("lastStartTimestamp initial ambigu")
print(json.dumps(generation, ensure_ascii=True, separators=(",", ":")))
PY
)" || return 1
    [[ -n "${PRE_START_GENERATION_JSON}" && \
        "${PRE_START_GENERATION_JSON}" != *$'\n'* && \
        "${PRE_START_GENERATION_JSON}" != *$'\r'* ]] || return 1
    PRE_START_SNAPSHOT_CERTIFIED=1
    printf '[GARDE GÉNÉRATION] État initial TERMINATED et génération pré-démarrage capturés.\n'
}

target_has_unchanged_terminated_generation() {
    local lifecycle_json=""

    ((PRE_START_SNAPSHOT_CERTIFIED == 1)) || return 1
    lifecycle_json="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --format=json 2>/dev/null)" || return 1
    python3 - "${PRE_START_GENERATION_JSON}" "${lifecycle_json}" <<'PY'
import json
import sys

expected = json.loads(sys.argv[1])
value = json.loads(sys.argv[2])
if not isinstance(value, dict) or value.get("status") != "TERMINATED":
    raise SystemExit("la cible n'est pas TERMINATED")
generation = value.get("lastStartTimestamp")
if generation in (None, ""):
    generation = None
elif not isinstance(generation, str) or "\n" in generation or "\r" in generation:
    raise SystemExit("lastStartTimestamp final ambigu")
if generation != expected:
    raise SystemExit("la génération a changé")
PY
}

preserve_session_ssh_key() {
    [[ -n "${SSH_KEY_FILE}" ]] || return 0
    printf '[CLÉ SSH CONSERVÉE] État ciblé non certifié avec sa génération; clé locale=%s, expiration OS Login fixe=%s.\n' \
        "${SSH_KEY_FILE}" "${SSH_KEY_EXPIRATION_UTC:-TTL ${SSH_KEY_TTL}}" >&2
}

load_targeted_handoff() {
    local handoff_fields=""
    local generation=""
    local handoff_status=""
    [[ -n "${START_HANDOFF}" && -f "${START_HANDOFF}" && ! -L "${START_HANDOFF}" ]] || return 1
    handoff_fields="$(python3 - "${START_HANDOFF}" "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" \
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
    "zone": sys.argv[3],
}
if not isinstance(value, dict) or set(value) != set(expected) | {"last_start_timestamp", "status"}:
    raise SystemExit("invalid targeted start handoff keys")
generation = value.pop("last_start_timestamp")
status = value.pop("status")
if value != expected:
    raise SystemExit("invalid targeted start handoff")
if not isinstance(generation, str) or not generation or "\n" in generation or "\r" in generation:
    raise SystemExit("invalid targeted start generation")
if status not in {"targeted_running", "targeted_stopping"}:
    raise SystemExit("invalid targeted start status")
print(f"{status}\t{generation}")
PY
)" || return 1
    IFS=$'\t' read -r handoff_status generation <<<"${handoff_fields}"
    [[ "${handoff_status}" == "targeted_running" || \
        "${handoff_status}" == "targeted_stopping" ]] || return 1
    [[ -n "${generation}" ]] || return 1
    SESSION_HANDOFF_STATUS="${handoff_status}"
    SESSION_LAST_START_TIMESTAMP="${generation}"
}

certify_session_deadline() {
    local lifecycle_json=""
    lifecycle_json="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_READ_TIMEOUT_SECONDS}s" gcloud compute instances describe "${INSTANCE_NAME}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --format=json)" || return 1
    EFFECTIVE_GCE_DEADLINE_EPOCH="$(python3 - \
        "${SESSION_LAST_START_TIMESTAMP}" "${ZONE}" \
        "${EXPECTED_MAX_RUN_SECONDS}" "${TIMESTAMP_TOLERANCE_SECONDS}" \
        "${WORK_RESERVE_SECONDS}" "${lifecycle_json}" <<'PY'
from datetime import datetime, timezone
import json
import sys


def parse_timestamp(value: object, label: str) -> int:
    if not isinstance(value, str) or not value or "\n" in value or "\r" in value:
        raise SystemExit(f"{label} absent ou ambigu")
    candidate = value[:-1] + "+00:00" if value.endswith("Z") else value
    parsed = datetime.fromisoformat(candidate)
    if parsed.tzinfo is None:
        raise SystemExit(f"{label} sans fuseau")
    return int(parsed.timestamp())


expected_generation = sys.argv[1]
zone = sys.argv[2]
expected_duration = int(sys.argv[3])
tolerance = int(sys.argv[4])
reserve = int(sys.argv[5])
value = json.loads(sys.argv[6])
if not isinstance(value, dict) or value.get("status") != "RUNNING":
    raise SystemExit("la cible n'est pas RUNNING après les deux gardes")
if value.get("lastStartTimestamp") != expected_generation:
    raise SystemExit("la génération GCE a changé après les deux gardes")
scheduling = value.get("scheduling")
if not isinstance(scheduling, dict):
    raise SystemExit("scheduling absent de la relecture GCE")
max_run = scheduling.get("maxRunDuration")
if not isinstance(max_run, dict) or str(max_run.get("seconds")) != str(expected_duration):
    raise SystemExit(f"maxRunDuration doit rester exactement {expected_duration} s")
start_epoch = parse_timestamp(expected_generation, "lastStartTimestamp")
computed_deadline = start_epoch + expected_duration
termination = value.get("terminationTimestamp")
if termination not in (None, ""):
    termination_epoch = parse_timestamp(termination, "terminationTimestamp")
    if not computed_deadline - tolerance <= termination_epoch <= computed_deadline + tolerance:
        raise SystemExit("terminationTimestamp incohérent avec la durée GCE")
elif "-ai" not in zone:
    raise SystemExit("terminationTimestamp absent hors zone IA")
safe_deadline = computed_deadline - tolerance
now = int(datetime.now(timezone.utc).timestamp())
if safe_deadline - reserve <= now:
    raise SystemExit("la deadline de travail GCE-30 min est déjà atteinte")
print(safe_deadline)
PY
)" || return 1
    [[ "${EFFECTIVE_GCE_DEADLINE_EPOCH}" =~ ^[0-9]+$ ]] || return 1
    printf '[ÉCHÉANCE] GCE sûre=%s; aucune nouvelle unité après %s (réserve=%ss).\n' \
        "${EFFECTIVE_GCE_DEADLINE_EPOCH}" \
        "$((EFFECTIVE_GCE_DEADLINE_EPOCH - WORK_RESERVE_SECONDS))" \
        "${WORK_RESERVE_SECONDS}"
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

    if ! final_status="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
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
    if ! final_generation="$(timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
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

cleanup_local_publication() {
    if [[ -n "${LOCAL_TEMP_RESULT}" && -e "${LOCAL_TEMP_RESULT}" ]]; then
        rm -f -- "${LOCAL_TEMP_RESULT}" || true
    fi
    if [[ -n "${LOCAL_PHASE4_TEMP_RESULT}" && \
        -e "${LOCAL_PHASE4_TEMP_RESULT}" ]]; then
        rm -f -- "${LOCAL_PHASE4_TEMP_RESULT}" || true
    fi
    if [[ -n "${LOCAL_PHASE5_TEMP_RESULT}" && \
        -e "${LOCAL_PHASE5_TEMP_RESULT}" ]]; then
        rm -f -- "${LOCAL_PHASE5_TEMP_RESULT}" || true
    fi
}

on_exit() {
    local original_status=$?
    local cleanup_status=0
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
            cleanup_local_publication
            if [[ -n "${START_HANDOFF}" && -e "${START_HANDOFF}" ]]; then
                printf '[HANDOFF CONSERVÉ] Arrêt non certifié; preuve ciblée à conserver jusqu’à résolution : %s\n' \
                    "${START_HANDOFF}" >&2
            fi
            preserve_session_ssh_key
            exit "${stop_status}"
        fi
    fi
    cleanup_local_publication
    if [[ -n "${START_HANDOFF}" && -e "${START_HANDOFF}" ]]; then
        if ((TARGET_STOP_CERTIFIED == 1)); then
            rm -f -- "${START_HANDOFF}" || true
        else
            printf '[HANDOFF CONSERVÉ] Session non certifiée arrêtée; preuve opérationnelle : %s\n' \
                "${START_HANDOFF}" >&2
        fi
    fi
    if ((START_INVOCATION_ATTEMPTED == 0 || TARGET_STOP_CERTIFIED == 1)) || \
        target_has_unchanged_terminated_generation; then
        if revoke_and_remove_session_ssh_key; then
            cleanup_status=0
        else
            cleanup_status=$?
        fi
        if ((cleanup_status != 0)); then
            exit "${SSH_KEY_CLEANUP_FAILURE}"
        fi
    else
        preserve_session_ssh_key
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
create_session_ssh_key || \
    die "Impossible de générer et valider la clé ED25519 de session hors dépôt."
import_session_ssh_key || \
    die "Impossible d'inscrire la clé de session dans OS Login avec un TTL de ${SSH_KEY_TTL}; aucun démarrage ne sera tenté."
capture_pre_start_snapshot || \
    die "Impossible de certifier l'état TERMINATED et la génération juste avant l'invocation de démarrage."

printf '%s\n' \
    "[SESSION PHASE 3] Autorisation non interactive confirmée :" \
    "  projet          : ${PROJECT_ID}" \
    "  zone            : ${ZONE}" \
    "  instance        : ${INSTANCE_NAME}" \
    "  SHA             : ${HEAD_SHA}" \
    "  arrêt invité    : ${GUEST_SHUTDOWN_MINUTES} min" \
    "  Docker hôte     : $([[ ${PROVISION_DOCKER} == 1 ]] && printf 'provisioning Ubuntu autorisé' || printf 'aucune mutation')" \
    "  replay Phase 4  : $([[ ${PHASE4_SPATIAL_REFERENCE} == 1 ]] && printf 'référence + LBVH résident activés' || printf 'désactivé')" \
    "  replay Phase 5  : $([[ ${PHASE5_K1_BORUVKA} == 1 ]] && printf 'boucle K1 Boruvka complète activée' || printf 'désactivé')" \
    "  clé SSH         : ED25519 OS Login, TTL ${SSH_KEY_TTL}" \
    "  résultat local  : ${LOCAL_RESULT}"

START_INVOCATION_ATTEMPTED=1
"${START_SCRIPT}" --yes \
    --guest-shutdown-minutes "${GUEST_SHUTDOWN_MINUTES}" \
    --handoff-file "${START_HANDOFF}"
SESSION_CERTIFIED=1
load_targeted_handoff || die "Le démarrage n'a pas publié son témoin ciblé certifié."
[[ "${SESSION_HANDOFF_STATUS}" == "targeted_running" ]] || \
    die "Le démarrage n'a pas publié un témoin targeted_running; statut reçu=${SESSION_HANDOFF_STATUS:-vide}."
certify_session_deadline || \
    die "La durée exacte de 3600 s et l'échéance de travail GCE-30 min n'ont pas pu être certifiées après démarrage."

mktemp_output="$(remote_exec \
    'remote_dir=$(mktemp -d /tmp/morsehgp3d-phase3.XXXXXXXX) && printf "__EHGP_REMOTE_DIR__%s\n" "${remote_dir}"')"
REMOTE_WORKDIR="$(printf '%s\n' "${mktemp_output}" | sed -n 's/^__EHGP_REMOTE_DIR__//p' | tail -n 1)"
[[ "${REMOTE_WORKDIR}" =~ ^/tmp/morsehgp3d-phase3\.[A-Za-z0-9]{8}$ ]] || \
    die "mktemp distant absent ou ambigu; valeur reçue : ${REMOTE_WORKDIR:-vide}."

remote_repository="${REMOTE_WORKDIR}/repository"
remote_artifact="${REMOTE_WORKDIR}/phase3-result.json"
remote_phase4_artifact=""
remote_phase5_artifact=""
quoted_origin="$(shell_quote "${ORIGIN_URL}")"
quoted_repository="$(shell_quote "${remote_repository}")"
quoted_head="$(shell_quote "${HEAD_SHA}")"
quoted_artifact="$(shell_quote "${remote_artifact}")"
quoted_phase4_artifact=""
quoted_phase5_artifact=""
phase4_worker_option=""
phase5_worker_option=""
if ((PHASE4_SPATIAL_REFERENCE == 1)); then
    remote_phase4_artifact="${REMOTE_WORKDIR}/phase4-spatial-result.json"
    quoted_phase4_artifact="$(shell_quote "${remote_phase4_artifact}")"
    phase4_worker_option=" --phase4-spatial-output ${quoted_phase4_artifact}"
fi
if ((PHASE5_K1_BORUVKA == 1)); then
    remote_phase5_artifact="${REMOTE_WORKDIR}/phase5-k1-boruvka-result.json"
    quoted_phase5_artifact="$(shell_quote "${remote_phase5_artifact}")"
    phase5_worker_option=" --phase5-k1-boruvka-output ${quoted_phase5_artifact}"
fi
quoted_gce_deadline="$(shell_quote "${EFFECTIVE_GCE_DEADLINE_EPOCH}")"

clone_output="$(remote_exec \
    "git clone --quiet --single-branch --branch main ${quoted_origin} ${quoted_repository} && git -C ${quoted_repository} checkout --quiet --detach ${quoted_head} && remote_head=\$(git -C ${quoted_repository} rev-parse HEAD) && printf '__EHGP_REMOTE_HEAD__%s\\n' \"\${remote_head}\"")"
remote_head="$(printf '%s\n' "${clone_output}" | sed -n 's/^__EHGP_REMOTE_HEAD__//p' | tail -n 1)"
[[ "${remote_head}" == "${HEAD_SHA}" ]] || \
    die "HEAD distant ${remote_head:-illisible} différent du SHA local ${HEAD_SHA}."

if ((PROVISION_DOCKER == 1)); then
    remote_exec \
        "test -x ${quoted_repository}/gcp-migration/phase3_remote_docker_provision.sh && cd ${quoted_repository} && ./gcp-migration/phase3_remote_docker_provision.sh --yes --gce-deadline-epoch ${quoted_gce_deadline}"
    certify_session_deadline || \
        die "La garde GCE n'a pas pu être recertifiée après la préparation Docker; worker refusé."
fi

remote_exec \
    "test -x ${quoted_repository}/gcp-migration/phase3_remote_qualification.sh && cd ${quoted_repository} && ./gcp-migration/phase3_remote_qualification.sh --yes --gce-deadline-epoch ${quoted_gce_deadline} --output ${quoted_artifact}${phase4_worker_option}${phase5_worker_option}"

timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
    "${GCLOUD_TRANSFER_TIMEOUT_SECONDS}s" gcloud compute scp \
    "${INSTANCE_NAME}:${remote_artifact}" \
    "${LOCAL_TEMP_RESULT}" \
    --project="${PROJECT_ID}" \
    --zone="${ZONE}" \
    --quiet \
    --ssh-key-file="${SSH_KEY_FILE}" \
    --ssh-key-expiration="${SSH_KEY_EXPIRATION_UTC}" \
    --scp-flag='-o ConnectTimeout=15' \
    --scp-flag='-o BatchMode=yes'

if ((PHASE4_SPATIAL_REFERENCE == 1)); then
    timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_TRANSFER_TIMEOUT_SECONDS}s" gcloud compute scp \
        "${INSTANCE_NAME}:${remote_phase4_artifact}" \
        "${LOCAL_PHASE4_TEMP_RESULT}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --quiet \
        --ssh-key-file="${SSH_KEY_FILE}" \
        --ssh-key-expiration="${SSH_KEY_EXPIRATION_UTC}" \
        --scp-flag='-o ConnectTimeout=15' \
        --scp-flag='-o BatchMode=yes'
fi
if ((PHASE5_K1_BORUVKA == 1)); then
    timeout --kill-after="${GCLOUD_KILL_AFTER_SECONDS}s" \
        "${GCLOUD_TRANSFER_TIMEOUT_SECONDS}s" gcloud compute scp \
        "${INSTANCE_NAME}:${remote_phase5_artifact}" \
        "${LOCAL_PHASE5_TEMP_RESULT}" \
        --project="${PROJECT_ID}" \
        --zone="${ZONE}" \
        --quiet \
        --ssh-key-file="${SSH_KEY_FILE}" \
        --ssh-key-expiration="${SSH_KEY_EXPIRATION_UTC}" \
        --scp-flag='-o ConnectTimeout=15' \
        --scp-flag='-o BatchMode=yes'
fi

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
if image.get("base_ref") != (
    "nvidia/cuda:12.9.2-devel-ubuntu24.04@"
    "sha256:420850a3fd665171b3f1fd08946c51d50468d732a46d6c42345ea04444755048"
):
    raise SystemExit("image CUDA de base distante incohérente")
if image.get("ref") != f"morsehgp3d-phase3:{sys.argv[2]}":
    raise SystemExit("tag d'image distant incohérent")
if re.fullmatch(r"sha256:[0-9a-f]{64}", str(image.get("id"))) is None:
    raise SystemExit("identifiant d'image distant non canonique")
lifecycle = value.get("vm_lifecycle")
if not isinstance(lifecycle, dict) or lifecycle.get("worker_mutates_gcp") is not False:
    raise SystemExit("contrat de cycle de vie du worker absent")
PY

if ((PHASE4_SPATIAL_REFERENCE == 1)); then
    [[ -s "${LOCAL_PHASE4_TEMP_RESULT}" ]] || \
        die "Artefact Phase 4 distant récupéré mais vide."
    python3 - "${LOCAL_PHASE4_TEMP_RESULT}" "${HEAD_SHA}" \
        "${LOCAL_TEMP_RESULT}" <<'PY'
import json
from pathlib import Path
import re
import sys

path = Path(sys.argv[1])
with path.open(encoding="utf-8") as stream:
    value = json.load(stream)
with Path(sys.argv[3]).open(encoding="utf-8") as stream:
    phase3 = json.load(stream)
if not isinstance(value, dict) or value.get("schema") != (
    "morsehgp3d.phase4.spatial_gpu_reference_and_lbvh_qualification.v3"
):
    raise SystemExit("l'artefact spatial Phase 4 possède un schéma invalide")
for key, expected in {
    "backend": "cuda_g4",
    "mode": "certified",
    "phase": "4",
    "profile": "hgp_reduced",
    "scientific_scope": (
        "non_certifying_gpu_proposals_with_cpu_exact_reference_and_parallel_frontier_lbvh_recertification"
    ),
    "status": "worker_passed_pending_shutdown",
}.items():
    if value.get(key) != expected:
        raise SystemExit(f"champ Phase 4 distant invalide: {key}")
if "public_status" in value:
    raise SystemExit("la qualification spatiale ne doit pas publier public_status")
if value.get("scientific_result_claimed") is not False:
    raise SystemExit("la qualification spatiale revendique à tort un résultat scientifique")
if value.get("scientific_public_status") is not None:
    raise SystemExit("la qualification spatiale expose un statut public scientifique")
if value.get("git") != {"clean": True, "sha": sys.argv[2]}:
    raise SystemExit("l'artefact spatial ne correspond pas au SHA propre qualifié")
image = value.get("image")
phase3_image = phase3.get("image")
if not isinstance(image, dict) or not isinstance(phase3_image, dict):
    raise SystemExit("identité d'image spatiale absente")
if image != {
    field: phase3_image.get(field) for field in ("base_ref", "id", "ref")
}:
    raise SystemExit("identité d'image spatiale incohérente")
if re.fullmatch(r"sha256:[0-9a-f]{64}", str(image.get("id"))) is None:
    raise SystemExit("digest d'image spatial non canonique")
binary = value.get("binary")
if not isinstance(binary, dict) or any(
    re.fullmatch(r"[0-9a-f]{64}", str(binary.get(field))) is None
    for field in (
        "checker_sha256",
        "lbvh_checker_sha256",
        "lbvh_replay_sha256",
        "replay_sha256",
    )
):
    raise SystemExit("empreintes binaires spatiales absentes ou invalides")
checks = value.get("checks")
expected_check_keys = {
    "aot_elf_architectures",
    "aot_ptx_entry_count",
    "compute_sanitizer",
    "cpu_exact_recertification_complete",
    "cuda_audit_workflow",
    "cuda_release_workflow",
    "differential",
    "lbvh_aot_elf_architectures",
    "lbvh_aot_ptx_entry_count",
    "lbvh_compute_sanitizer",
    "lbvh_cpu_exact_recertification_complete",
    "lbvh_differential",
    "lbvh_memcheck_differential",
    "lbvh_racecheck",
    "quick_memcheck_differential",
}
if not isinstance(checks, dict) or set(checks) != expected_check_keys:
    raise SystemExit("contrôles spatiaux absents")
for field, expected in {
    "aot_elf_architectures": ["sm_120"],
    "aot_ptx_entry_count": 0,
    "compute_sanitizer": "passed",
    "cpu_exact_recertification_complete": True,
    "cuda_audit_workflow": "passed",
    "cuda_release_workflow": "passed",
    "lbvh_aot_elf_architectures": ["sm_120"],
    "lbvh_aot_ptx_entry_count": 0,
    "lbvh_compute_sanitizer": "passed",
    "lbvh_cpu_exact_recertification_complete": True,
    "lbvh_racecheck": "passed",
}.items():
    if checks.get(field) != expected:
        raise SystemExit(f"contrôle spatial {field} invalide")
required_summary_keys = {
    "all_cases_passed",
    "campaign_complete",
    "campaign_sizes_checked",
    "case_count",
    "closed_ball_query_count",
    "cpu_exact_recertification_complete",
    "decision_semantics",
    "gpu_launch_count",
    "ieee_coverage",
    "projection_coverage",
    "proposal_semantics",
    "schema",
    "scope",
    "scientific_public_status",
    "scientific_result_claimed",
    "top_k_query_count",
}
for field, scope, sizes, count in (
    ("differential", "full", list(range(1, 1001)), 1013),
    ("quick_memcheck_differential", "quick", [1, 2, 3, 4, 17, 257, 1000], 20),
):
    summary = checks.get(field)
    if not isinstance(summary, dict) or set(summary) != required_summary_keys:
        raise SystemExit(f"résumé spatial {field} absent ou mal typé")
    expected_summary = {
        "all_cases_passed": True,
        "campaign_complete": scope == "full",
        "campaign_sizes_checked": sizes,
        "case_count": count,
        "closed_ball_query_count": count,
        "cpu_exact_recertification_complete": True,
        "decision_semantics": "cpu_exact_all_points",
        "gpu_launch_count": 2 * count,
        "ieee_coverage": [
            "addition_only_overflow",
            "finite_subnormal_distance",
            "max_finite_query",
            "normal_subnormal_tie",
            "overflow_clamped_query",
            "subnormal_tie",
        ],
        "projection_coverage": [
            "exact",
            "overflow_clamped",
            "rounded",
            "underflow",
        ],
        "proposal_semantics": "non_certifying_fp64",
        "schema": "morsehgp3d.phase4.spatial_gpu_differential.v1",
        "scope": scope,
        "scientific_public_status": None,
        "scientific_result_claimed": False,
        "top_k_query_count": count,
    }
    if summary != expected_summary:
        raise SystemExit(f"résumé spatial {field} incomplet")
required_lbvh_summary_keys = {
    "all_cases_passed",
    "bounded_protocol",
    "campaign_complete",
    "campaign_input_sha256",
    "campaign_result_sha256",
    "campaign_sizes_checked",
    "case_count",
    "certified_pruned_subtree_count",
    "chunk_case_limit",
    "chunk_count",
    "closed_ball_query_count",
    "cover_antichain_complete",
    "cpu_exact_recertification_complete",
    "decision_semantics",
    "directed_enclosure_coverage",
    "exact_partition_complete",
    "gpu_kernel_launch_count",
    "gpu_launch_count",
    "gpu_parallel_round_count",
    "gpu_peak_frontier_count",
    "gpu_processed_node_count",
    "gpu_traversal_round_count",
    "input_point_count",
    "maximum_point_count",
    "point_partition_complete",
    "proposal_semantics",
    "schema",
    "scope",
    "scientific_public_status",
    "scientific_result_claimed",
    "targeted_case_count",
    "targeted_coverage",
    "top_k_query_count",
}
required_lbvh_coverage = [
    "addition_only_overflow",
    "cutoff_non_binary64",
    "cutoff_outside_binary64",
    "exact_tie",
    "exclusions",
    "maximum_finite",
    "negative_query_outside_binary64",
    "permuted_input",
    "query_non_binary64",
    "query_outside_binary64",
    "signed_subnormal",
    "singleton",
    "six_way_shell",
    "tri_partition",
]
lbvh_summaries = {}
for (
    field,
    scope,
    campaign_complete,
    campaign_sizes,
    case_count,
    gpu_launch_count,
    input_point_count,
    chunk_count,
) in (
    (
        "lbvh_differential",
        "full",
        True,
        list(range(1, 1001)),
        1013,
        2019,
        500550,
        8,
    ),
    (
        "lbvh_memcheck_differential",
        "quick",
        False,
        [1, 2, 3, 4, 17, 257, 1000],
        20,
        33,
        1334,
        1,
    ),
):
    summary = checks.get(field)
    if not isinstance(summary, dict) or set(summary) != required_lbvh_summary_keys:
        raise SystemExit(f"résumé LBVH résident {field} absent ou mal typé")
    if summary.get("schema") != (
        "morsehgp3d.phase4.spatial_gpu_lbvh_differential.v2"
    ):
        raise SystemExit(f"résumé LBVH résident {field}.schema invalide")
    if summary.get("scope") != scope:
        raise SystemExit(
            f"résumé LBVH résident {field}.scope doit valoir {scope}"
        )
    for key in (
        "all_cases_passed",
        "bounded_protocol",
        "cover_antichain_complete",
        "cpu_exact_recertification_complete",
        "exact_partition_complete",
        "point_partition_complete",
    ):
        if summary.get(key) is not True:
            raise SystemExit(f"résumé LBVH résident {field}.{key} invalide")
    if summary.get("campaign_complete") is not campaign_complete:
        raise SystemExit(
            f"résumé LBVH résident {field}.campaign_complete invalide"
        )
    if summary.get("scientific_result_claimed") is not False:
        raise SystemExit(
            f"résumé LBVH résident {field}.scientific_result_claimed invalide"
        )
    if summary.get("scientific_public_status") is not None:
        raise SystemExit(
            f"résumé LBVH résident {field}.scientific_public_status invalide"
        )
    if summary.get("decision_semantics") != (
        "cpu_exact_cover_and_leaf_recertification"
    ):
        raise SystemExit(
            f"résumé LBVH résident {field}.decision_semantics invalide"
        )
    if summary.get("proposal_semantics") != (
        "gpu_resident_parallel_frontier_lbvh_strict_exterior_cover"
    ):
        raise SystemExit(
            f"résumé LBVH résident {field}.proposal_semantics invalide"
        )
    if summary.get("directed_enclosure_coverage") != [
        "enclosed",
        "exact",
        "unsupported_range",
    ]:
        raise SystemExit(
            f"résumé LBVH résident {field}.directed_enclosure_coverage invalide"
        )
    if summary.get("targeted_coverage") != required_lbvh_coverage:
        raise SystemExit(
            f"résumé LBVH résident {field}.targeted_coverage invalide"
        )
    if summary.get("campaign_sizes_checked") != campaign_sizes:
        raise SystemExit(
            f"résumé LBVH résident {field}.campaign_sizes_checked invalide"
        )
    expected_integer_counts = {
        "case_count": case_count,
        "chunk_case_limit": 128,
        "chunk_count": chunk_count,
        "closed_ball_query_count": case_count,
        "gpu_launch_count": gpu_launch_count,
        "input_point_count": input_point_count,
        "maximum_point_count": 1000,
        "targeted_case_count": 13,
        "top_k_query_count": case_count,
    }
    for key, expected in expected_integer_counts.items():
        if type(summary.get(key)) is not int or summary[key] != expected:
            raise SystemExit(
                f"résumé LBVH résident {field}.{key} doit valoir {expected}"
            )
    for key in ("campaign_input_sha256", "campaign_result_sha256"):
        if re.fullmatch(r"[0-9a-f]{64}", str(summary.get(key))) is None:
            raise SystemExit(
                f"résumé LBVH résident {field}.{key} non canonique"
            )
    prunes = summary.get("certified_pruned_subtree_count")
    if type(prunes) is not int or prunes <= 0:
        raise SystemExit(f"résumé LBVH résident {field} sans prune certifié")
    parallel_counts = {}
    for key in (
        "gpu_kernel_launch_count",
        "gpu_parallel_round_count",
        "gpu_peak_frontier_count",
        "gpu_processed_node_count",
        "gpu_traversal_round_count",
    ):
        count = summary.get(key)
        if type(count) is not int or count <= 0:
            raise SystemExit(
                f"résumé LBVH résident {field}.{key} doit être strictement positif"
            )
        parallel_counts[key] = count
    if parallel_counts["gpu_kernel_launch_count"] != parallel_counts[
        "gpu_traversal_round_count"
    ]:
        raise SystemExit(
            f"résumé LBVH résident {field}: kernels et rondes divergent"
        )
    if parallel_counts["gpu_parallel_round_count"] > parallel_counts[
        "gpu_traversal_round_count"
    ]:
        raise SystemExit(
            f"résumé LBVH résident {field}: trop de rondes parallèles"
        )
    if parallel_counts["gpu_traversal_round_count"] > parallel_counts[
        "gpu_processed_node_count"
    ]:
        raise SystemExit(
            f"résumé LBVH résident {field}: rondes supérieures aux nœuds"
        )
    if not 1 < parallel_counts["gpu_peak_frontier_count"] <= 1999:
        raise SystemExit(
            f"résumé LBVH résident {field}: pic de frontière hors bornes"
        )
    maximum_processed_nodes = 2 * (2 * input_point_count - case_count)
    if not (
        gpu_launch_count
        <= parallel_counts["gpu_processed_node_count"]
        <= maximum_processed_nodes
    ):
        raise SystemExit(
            f"résumé LBVH résident {field}: nœuds traités hors bornes"
        )
    lbvh_summaries[scope] = summary
if lbvh_summaries["full"]["campaign_input_sha256"] == lbvh_summaries[
    "quick"
]["campaign_input_sha256"]:
    raise SystemExit("les racines d'entrée LBVH full et quick doivent différer")
if lbvh_summaries["full"]["campaign_result_sha256"] == lbvh_summaries[
    "quick"
]["campaign_result_sha256"]:
    raise SystemExit("les racines de résultat LBVH full et quick doivent différer")
environment = value.get("environment")
if not isinstance(environment, dict) or environment.get("compute_capability") != "12.0":
    raise SystemExit("environnement spatial absent ou incompatible")
logs = value.get("logs")
if not isinstance(logs, dict) or set(logs) != {
    "compute_sanitizer",
    "cuobjdump_elf",
    "cuobjdump_ptx",
    "cuobjdump_ptx_stderr",
    "differential",
    "lbvh_compute_sanitizer",
    "lbvh_cuobjdump_elf",
    "lbvh_cuobjdump_ptx",
    "lbvh_cuobjdump_ptx_stderr",
    "lbvh_differential",
    "lbvh_racecheck",
}:
    raise SystemExit("journaux spatiaux absents ou incomplets")
lifecycle = value.get("vm_lifecycle")
if lifecycle != {
    "guest_shutdown_guard_verified": True,
    "stop_responsibility": "external_orchestrator",
    "worker_mutates_gcp": False,
}:
    raise SystemExit("contrat de cycle de vie spatial absent")
PY
fi

if ((PHASE5_K1_BORUVKA == 1)); then
    [[ -s "${LOCAL_PHASE5_TEMP_RESULT}" ]] || \
        die "Artefact Phase 5 K1 Boruvka distant récupéré mais vide."
    python3 - "${LOCAL_PHASE5_TEMP_RESULT}" "${HEAD_SHA}" \
        "${LOCAL_TEMP_RESULT}" <<'PY'
import hashlib
import json
from pathlib import Path
import re
import sys


def reject_duplicates(pairs):
    value = {}
    for key, item in pairs:
        if key in value:
            raise SystemExit(f"clé JSON Phase 5 dupliquée: {key}")
        value[key] = item
    return value


path = Path(sys.argv[1])
with path.open(encoding="utf-8") as stream:
    value = json.load(stream, object_pairs_hook=reject_duplicates)
phase3_path = Path(sys.argv[3])
with phase3_path.open(encoding="utf-8") as stream:
    phase3 = json.load(stream, object_pairs_hook=reject_duplicates)
expected_top_keys = {
    "backend",
    "binary",
    "checks",
    "environment",
    "git",
    "image",
    "logs",
    "mode",
    "phase",
    "profile",
    "provenance",
    "schema",
    "scientific_public_status",
    "scientific_result_claimed",
    "scientific_scope",
    "status",
    "vm_lifecycle",
}
if not isinstance(value, dict) or set(value) != expected_top_keys:
    raise SystemExit("l'artefact Phase 5 K1 Boruvka ne respecte pas son schéma fermé")
for key, expected in {
    "backend": "cuda_g4",
    "mode": "certified",
    "phase": "5",
    "profile": "hgp_reduced",
    "schema": "morsehgp3d.phase5.k1_boruvka_gpu_qualification.v2",
    "scientific_scope": "gpu_proposed_cpu_exact_full_boruvka_local_emst_witness_only",
    "status": "worker_passed_pending_shutdown",
}.items():
    if value.get(key) != expected:
        raise SystemExit(f"champ Phase 5 K1 Boruvka invalide: {key}")
if value.get("scientific_result_claimed") is not False:
    raise SystemExit("la qualification Phase 5 revendique à tort un résultat scientifique")
if value.get("scientific_public_status") is not None or "public_status" in value:
    raise SystemExit("la qualification Phase 5 expose à tort un statut public scientifique")
if value.get("git") != {"clean": True, "sha": sys.argv[2]}:
    raise SystemExit("l'artefact Phase 5 ne correspond pas au SHA propre qualifié")
image = value.get("image")
phase3_image = phase3.get("image")
if not isinstance(image, dict) or not isinstance(phase3_image, dict):
    raise SystemExit("identité d'image Phase 5 absente")
if image != {field: phase3_image.get(field) for field in ("base_ref", "id", "ref")}:
    raise SystemExit("identité d'image Phase 5 incohérente")
if re.fullmatch(r"sha256:[0-9a-f]{64}", str(image.get("id"))) is None:
    raise SystemExit("digest d'image Phase 5 non canonique")
binary = value.get("binary")
if not isinstance(binary, dict) or set(binary) != {"full_replay_sha256"} or re.fullmatch(
    r"[0-9a-f]{64}", str(binary.get("full_replay_sha256"))
) is None:
    raise SystemExit("empreinte du replay complet Phase 5 absente ou invalide")
checks = value.get("checks")
expected_check_keys = {
    "aot_elf_architectures",
    "aot_ptx_entry_count",
    "canonical_contractions_certified",
    "cpu_exact_decision_chain_certified",
    "gpu_multi_round_proposal_chain_certified",
    "independent_gpu_replay_certified",
    "local_emst_witness_certified",
    "memcheck",
    "racecheck",
    "replay",
}
if not isinstance(checks, dict) or set(checks) != expected_check_keys:
    raise SystemExit("contrôles Phase 5 absents ou ouverts")
replay = checks.get("replay")
if not isinstance(replay, dict):
    raise SystemExit("replay complet Phase 5 absent")
replay_canonical = json.dumps(
    replay, ensure_ascii=True, sort_keys=True, separators=(",", ":")
) + "\n"
if hashlib.sha256(replay_canonical.encode("utf-8")).hexdigest() != (
    "b4bd717d175c84be0c89e8602245e28b8059e350f0ce7ff33edafa2a2df30d98"
):
    raise SystemExit("replay complet Phase 5 différent de la fixture canonique")
if replay.get("schema") != (
    "morsehgp3d.phase5.k1_boruvka_full_loop_gpu_replay.v1"
):
    raise SystemExit("schéma du replay complet Phase 5 invalide")
if checks != {
    "aot_elf_architectures": ["sm_120"],
    "aot_ptx_entry_count": 0,
    "canonical_contractions_certified": True,
    "cpu_exact_decision_chain_certified": True,
    "gpu_multi_round_proposal_chain_certified": True,
    "independent_gpu_replay_certified": True,
    "local_emst_witness_certified": True,
    "memcheck": "passed",
    "racecheck": "passed",
    "replay": replay,
}:
    raise SystemExit("contrôles Phase 5 incomplets")
environment = value.get("environment")
if not isinstance(environment, dict) or set(environment) != {
    "compute_capability",
    "driver_version",
    "gpu_name",
    "gpu_uuid",
    "gpu_vram_bytes",
} or environment.get("compute_capability") != "12.0":
    raise SystemExit("environnement Phase 5 absent ou incompatible")
logs = value.get("logs")
if not isinstance(logs, dict) or set(logs) != {
    "cuobjdump_elf",
    "cuobjdump_ptx",
    "cuobjdump_ptx_stderr",
    "full_replay",
    "memcheck",
    "racecheck",
}:
    raise SystemExit("journaux Phase 5 absents ou ouverts")
provenance = value.get("provenance")
expected_phase3_digest = hashlib.sha256(phase3_path.read_bytes()).hexdigest()
if provenance != {
    "environment_artifact_schema": "morsehgp3d.phase3.qualification.v1",
    "environment_artifact_sha256": expected_phase3_digest,
}:
    raise SystemExit("provenance Phase 3 de l'artefact Phase 5 incohérente")
if value.get("vm_lifecycle") != {
    "guest_shutdown_guard_verified": True,
    "stop_responsibility": "external_orchestrator",
    "worker_mutates_gcp": False,
}:
    raise SystemExit("contrat de cycle de vie Phase 5 absent")
PY
fi

if certify_target_stopped; then
    stop_status=0
else
    stop_status=$?
fi
if ((stop_status != 0)); then
    exit "${stop_status}"
fi

python3 - "${LOCAL_TEMP_RESULT}" "${LOCAL_RESULT}" \
    "${LOCAL_PHASE4_TEMP_RESULT}" "${LOCAL_PHASE4_RESULT}" \
    "${LOCAL_PHASE5_TEMP_RESULT}" "${LOCAL_PHASE5_RESULT}" \
    "${PROJECT_ID}" "${ZONE}" "${INSTANCE_NAME}" \
    "${GUEST_SHUTDOWN_MINUTES}" "${FINAL_STATUS}" \
    "${FINAL_STOP_VERIFIED_AT_UTC}" "${SESSION_LAST_START_TIMESTAMP}" <<'PY'
import json
import os
from pathlib import Path
import sys

pairs = [(Path(sys.argv[1]), Path(sys.argv[2]))]
if sys.argv[3] or sys.argv[4]:
    if not sys.argv[3] or not sys.argv[4]:
        raise SystemExit("paire de publication Phase 4 incomplète")
    pairs.append((Path(sys.argv[3]), Path(sys.argv[4])))
if sys.argv[5] or sys.argv[6]:
    if not sys.argv[5] or not sys.argv[6]:
        raise SystemExit("paire de publication Phase 5 incomplète")
    pairs.append((Path(sys.argv[5]), Path(sys.argv[6])))

for temporary, _ in pairs:
    with temporary.open(encoding="utf-8") as stream:
        value = json.load(stream)
    lifecycle = value["vm_lifecycle"]
    lifecycle["worker_status_before_targeted_stop"] = value["status"]
    lifecycle.update(
        {
            "final_status": sys.argv[11],
            "final_status_readback": "gcloud_compute_instances_describe",
            "final_status_verified_at_utc": sys.argv[12],
            "guest_shutdown_minutes": int(sys.argv[10]),
            "initial_status": "TERMINATED",
            "initial_status_basis": "start_and_verify_precondition",
            "instance": sys.argv[9],
            "last_start_timestamp": sys.argv[13],
            "project": sys.argv[7],
            "start_handoff_schema": "e-hgp.start-handoff.v3",
            "targeted_stop_verified": True,
            "zone": sys.argv[8],
        }
    )
    value["status"] = "passed"
    encoded = json.dumps(
        value, ensure_ascii=False, sort_keys=True, separators=(",", ":")
    )
    with temporary.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(encoded)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())

published = []
try:
    for temporary, target in pairs:
        os.link(temporary, target, follow_symlinks=False)
        published.append(str(target))
    for parent in {target.parent for _, target in pairs}:
        descriptor = os.open(parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)
except OSError as error:
    published_label = ",".join(published) if published else "aucun"
    raise SystemExit(
        "refus de remplacer un artefact créé concurremment "
        f"ou publication partielle : {error}; publiés={published_label}"
    )
PY
rm -f -- "${LOCAL_TEMP_RESULT}"
if [[ -n "${LOCAL_PHASE4_TEMP_RESULT}" ]]; then
    rm -f -- "${LOCAL_PHASE4_TEMP_RESULT}"
fi
if [[ -n "${LOCAL_PHASE5_TEMP_RESULT}" ]]; then
    rm -f -- "${LOCAL_PHASE5_TEMP_RESULT}"
fi
LOCAL_TEMP_RESULT=""
LOCAL_PHASE4_TEMP_RESULT=""
LOCAL_PHASE5_TEMP_RESULT=""
rm -f -- "${START_HANDOFF}"
START_HANDOFF=""
printf '[ARTEFACT] Résultat Phase 3 publié après certification TERMINATED : %s\n' "${LOCAL_RESULT}"
if ((PHASE4_SPATIAL_REFERENCE == 1)); then
    printf '[ARTEFACT] Résultat spatial Phase 4 publié après certification TERMINATED : %s\n' \
        "${LOCAL_PHASE4_RESULT}"
fi
if ((PHASE5_K1_BORUVKA == 1)); then
    printf '[ARTEFACT] Résultat K1 Boruvka Phase 5 publié après certification TERMINATED : %s\n' \
        "${LOCAL_PHASE5_RESULT}"
fi

if ! revoke_and_remove_session_ssh_key; then
    printf '[ERREUR] Qualification calculée et cible TERMINATED, mais le nettoyage local de la clé de session a échoué.\n' >&2
    trap - EXIT HUP INT TERM
    exit "${SSH_KEY_CLEANUP_FAILURE}"
fi
trap - EXIT HUP INT TERM
printf '[SUCCÈS] Qualification Phase 3 terminée; cible certifiée TERMINATED et artefact conservé : %s\n' \
    "${LOCAL_RESULT}"
if ((PHASE4_SPATIAL_REFERENCE == 1)); then
    printf '[SUCCÈS] Qualification spatiale Phase 4 compagnon conservée : %s\n' \
        "${LOCAL_PHASE4_RESULT}"
fi
if ((PHASE5_K1_BORUVKA == 1)); then
    printf '[SUCCÈS] Qualification K1 Boruvka Phase 5 compagnon conservée : %s\n' \
        "${LOCAL_PHASE5_RESULT}"
fi
