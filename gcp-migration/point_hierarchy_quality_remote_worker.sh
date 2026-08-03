#!/usr/bin/env bash
set -Eeuo pipefail

# Worker invité de la campagne de qualité v2. Il ne pilote aucune ressource
# GCP : le cycle de vie reste exclusivement sous la responsabilité de
# run_phase3_qualification.sh. Le spool durable est volontairement conservé
# entre les sessions gardées; seuls les artefacts temporaires de cette session
# sont détruits par le trap local.

readonly WORKER_SCHEMA="morsehgp3d.point_hierarchy.quality_gcp_worker.v2"
readonly CAMPAIGN_SCRIPT_RELATIVE="morsehgp3d/tests/profiling/point_hierarchy_quality_campaign.py"
readonly CAMPAIGN_PLAN_RELATIVE="morsehgp3d/tests/profiling/point_hierarchy_quality_campaign_v2.json"
readonly PREFLIGHT_SCRIPT_RELATIVE="gcp-migration/check_point_hierarchy_quality_preflight.py"
readonly MINIMUM_WORK_WINDOW_SECONDS=600
readonly DEADLINE_RESERVE_SECONDS=120
readonly COMMAND_KILL_GRACE_SECONDS=10

ASSUME_YES=0
SOURCE_ROOT=""
OUTPUT_PATH=""
SPOOL_PATH=""
ENVIRONMENT_ARTIFACT=""
GCE_DEADLINE_EPOCH=""
CAMPAIGN_TEMP_OUTPUT=""
CAMPAIGN_TEMP_DIR=""
PRODUCER_PATH=""
VERIFIER_PATH=""

die() {
    printf '[ERREUR] %s\n' "$*" >&2
    exit 1
}

usage() {
    cat <<'EOF'
Usage : ./gcp-migration/point_hierarchy_quality_remote_worker.sh \
  --yes --source-root RÉPERTOIRE --gce-deadline-epoch EPOCH \
  --environment-artifact FICHIER --producer FICHIER --verifier FICHIER \
  --spool RÉPERTOIRE --output FICHIER

Vérifie la garde invitée, le plan v2 suivi et les handshakes distincts du
producteur et du vérificateur avant le premier benchmark. Le spool reprend au
prochain ordinal non committé et n'est jamais supprimé par ce worker.

Le worker ne contient et n'appelle aucune commande GCP. Il ne publie qu'un
artefact provisoire `*_pending_shutdown`; l'orchestrateur local peut le rendre
visible comme résumé complet ou `partial_resumable` uniquement après l'arrêt
ciblé et une relecture indépendante TERMINATED.
EOF
}

while (($# > 0)); do
    case "$1" in
        --yes)
            ((ASSUME_YES == 0)) || die "--yes ne peut être fourni qu'une fois."
            ASSUME_YES=1
            shift
            ;;
        --source-root)
            (($# >= 2)) || die "Valeur manquante après --source-root."
            [[ -z "${SOURCE_ROOT}" ]] || die "--source-root ne peut être fourni qu'une fois."
            SOURCE_ROOT="$2"
            shift 2
            ;;
        --gce-deadline-epoch)
            (($# >= 2)) || die "Valeur manquante après --gce-deadline-epoch."
            [[ -z "${GCE_DEADLINE_EPOCH}" ]] || die "--gce-deadline-epoch ne peut être fourni qu'une fois."
            GCE_DEADLINE_EPOCH="$2"
            shift 2
            ;;
        --environment-artifact)
            (($# >= 2)) || die "Valeur manquante après --environment-artifact."
            [[ -z "${ENVIRONMENT_ARTIFACT}" ]] || die "--environment-artifact ne peut être fourni qu'une fois."
            ENVIRONMENT_ARTIFACT="$2"
            shift 2
            ;;
        --producer)
            (($# >= 2)) || die "Valeur manquante après --producer."
            [[ -z "${PRODUCER_PATH}" ]] || die "--producer ne peut être fourni qu'une fois."
            PRODUCER_PATH="$2"
            shift 2
            ;;
        --verifier)
            (($# >= 2)) || die "Valeur manquante après --verifier."
            [[ -z "${VERIFIER_PATH}" ]] || die "--verifier ne peut être fourni qu'une fois."
            VERIFIER_PATH="$2"
            shift 2
            ;;
        --spool)
            (($# >= 2)) || die "Valeur manquante après --spool."
            [[ -z "${SPOOL_PATH}" ]] || die "--spool ne peut être fourni qu'une fois."
            SPOOL_PATH="$2"
            shift 2
            ;;
        --output)
            (($# >= 2)) || die "Valeur manquante après --output."
            [[ -z "${OUTPUT_PATH}" ]] || die "--output ne peut être fourni qu'une fois."
            OUTPUT_PATH="$2"
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

((ASSUME_YES == 1)) || die "--yes est obligatoire pour ce worker déjà placé sous garde."
[[ "${GCE_DEADLINE_EPOCH}" =~ ^[1-9][0-9]*$ ]] || die "L'échéance GCE doit être un epoch positif canonique."
(( ${#GCE_DEADLINE_EPOCH} <= 12 )) || die "L'échéance GCE est hors domaine."
GCE_DEADLINE_EPOCH=$((10#${GCE_DEADLINE_EPOCH}))
readonly GCE_DEADLINE_EPOCH

for required_path in SOURCE_ROOT ENVIRONMENT_ARTIFACT PRODUCER_PATH VERIFIER_PATH SPOOL_PATH OUTPUT_PATH; do
    value="${!required_path}"
    [[ "${value}" == /* && "${value}" != *$'\n'* && "${value}" != *$'\r'* ]] || \
        die "${required_path} doit être un chemin absolu sans saut de ligne."
done

command -v git >/dev/null 2>&1 || die "git est introuvable."
command -v python3 >/dev/null 2>&1 || die "python3 est introuvable."
command -v timeout >/dev/null 2>&1 || die "GNU timeout est introuvable."
timeout_version="$(timeout --version 2>/dev/null | sed -n '1p')" || die "Impossible d'identifier timeout."
[[ "${timeout_version}" == timeout\ \(GNU\ coreutils\)* ]] || die "timeout doit être GNU coreutils."

SOURCE_ROOT="$(cd -- "${SOURCE_ROOT}" && pwd -P)" || die "Source root illisible."
readonly SOURCE_ROOT
[[ -d "${SOURCE_ROOT}/.git" ]] || die "Le source root n'est pas un checkout Git."

canonical_regular_file() {
    local path="$1"
    local label="$2"
    [[ -f "${path}" && ! -L "${path}" ]] || die "${label} absent, non régulier ou symbolique : ${path}."
    local parent base
    parent="$(cd -- "$(dirname -- "${path}")" && pwd -P)" || die "Parent de ${label} illisible."
    base="$(basename -- "${path}")"
    printf '%s/%s\n' "${parent}" "${base}"
}

CAMPAIGN_SCRIPT="$(canonical_regular_file \
    "${SOURCE_ROOT}/${CAMPAIGN_SCRIPT_RELATIVE}" "Harnais de campagne")"
CAMPAIGN_PLAN="$(canonical_regular_file \
    "${SOURCE_ROOT}/${CAMPAIGN_PLAN_RELATIVE}" "Plan de campagne")"
PREFLIGHT_SCRIPT="$(canonical_regular_file \
    "${SOURCE_ROOT}/${PREFLIGHT_SCRIPT_RELATIVE}" "Checker de préflight")"
ENVIRONMENT_ARTIFACT="$(canonical_regular_file \
    "${ENVIRONMENT_ARTIFACT}" "Artefact d'environnement Phase 3")"
PRODUCER_PATH="$(canonical_regular_file "${PRODUCER_PATH}" "Producteur qualité")"
VERIFIER_PATH="$(canonical_regular_file "${VERIFIER_PATH}" "Vérificateur scientifique")"
[[ -x "${PRODUCER_PATH}" ]] || die "Le producteur qualité n'est pas exécutable : ${PRODUCER_PATH}."
[[ -x "${VERIFIER_PATH}" ]] || die "Le vérificateur scientifique n'est pas exécutable : ${VERIFIER_PATH}."
[[ "${PRODUCER_PATH}" != "${VERIFIER_PATH}" ]] || die "Producteur et vérificateur doivent être des fichiers distincts."
readonly CAMPAIGN_SCRIPT CAMPAIGN_PLAN PREFLIGHT_SCRIPT ENVIRONMENT_ARTIFACT PRODUCER_PATH VERIFIER_PATH

output_parent="$(dirname -- "${OUTPUT_PATH}")"
output_base="$(basename -- "${OUTPUT_PATH}")"
[[ -n "${output_base}" && "${output_base}" != "." && "${output_base}" != ".." ]] || die "Nom d'artefact de sortie invalide."
[[ -d "${output_parent}" && ! -L "${output_parent}" ]] || die "Parent de sortie absent ou symbolique : ${output_parent}."
output_parent="$(cd -- "${output_parent}" && pwd -P)" || die "Parent de sortie illisible."
OUTPUT_PATH="${output_parent}/${output_base}"
readonly OUTPUT_PATH
[[ ! -e "${OUTPUT_PATH}" && ! -L "${OUTPUT_PATH}" ]] || die "La sortie doit être inexistante : ${OUTPUT_PATH}."

spool_parent="$(dirname -- "${SPOOL_PATH}")"
spool_base="$(basename -- "${SPOOL_PATH}")"
[[ -n "${spool_base}" && "${spool_base}" != "." && "${spool_base}" != ".." ]] || die "Nom de spool invalide."
[[ -d "${spool_parent}" && ! -L "${spool_parent}" ]] || die "Parent du spool absent ou symbolique : ${spool_parent}."
spool_parent="$(cd -- "${spool_parent}" && pwd -P)" || die "Parent du spool illisible."
SPOOL_PATH="${spool_parent}/${spool_base}"
if [[ -e "${SPOOL_PATH}" || -L "${SPOOL_PATH}" ]]; then
    [[ -d "${SPOOL_PATH}" && ! -L "${SPOOL_PATH}" ]] || die "Le spool existant doit être un répertoire non symbolique."
    [[ "$(cd -- "${SPOOL_PATH}" && pwd -P)" == "${SPOOL_PATH}" ]] || die "Le spool existant n'est pas canonique."
fi
readonly SPOOL_PATH

now_epoch="$(date +%s)" || die "Horloge invitée illisible."
[[ "${now_epoch}" =~ ^[1-9][0-9]*$ ]] || die "Horloge invitée invalide."
((GCE_DEADLINE_EPOCH - now_epoch >= MINIMUM_WORK_WINDOW_SECONDS + DEADLINE_RESERVE_SECONDS)) || \
    die "Fenêtre de travail insuffisante avant l'échéance GCE sûre."

readonly SCHEDULED_SHUTDOWN_FILE="/run/systemd/shutdown/scheduled"
[[ -r "${SCHEDULED_SHUTDOWN_FILE}" ]] || die "Coupe-circuit invité absent ou illisible."
shutdown_mode="$(sed -n 's/^MODE=\(.*\)$/\1/p' "${SCHEDULED_SHUTDOWN_FILE}")"
shutdown_usec="$(sed -n 's/^USEC=\([0-9][0-9]*\)$/\1/p' "${SCHEDULED_SHUTDOWN_FILE}")"
[[ "${shutdown_mode}" == "poweroff" ]] || die "Le coupe-circuit invité n'est pas un poweroff."
[[ "${shutdown_usec}" =~ ^[0-9]{1,18}$ ]] || die "Échéance du coupe-circuit invité invalide."
shutdown_epoch=$((10#${shutdown_usec} / 1000000))
((shutdown_epoch > now_epoch)) || die "Le coupe-circuit invité n'est plus futur."
((shutdown_epoch <= GCE_DEADLINE_EPOCH)) || die "Le coupe-circuit invité dépasse l'échéance GCE sûre."
WORK_DEADLINE_EPOCH="${GCE_DEADLINE_EPOCH}"
if ((shutdown_epoch < WORK_DEADLINE_EPOCH)); then
    WORK_DEADLINE_EPOCH="${shutdown_epoch}"
fi
readonly WORK_DEADLINE_EPOCH
CAMPAIGN_STOP_BEFORE_EPOCH=$((WORK_DEADLINE_EPOCH - DEADLINE_RESERVE_SECONDS))
readonly CAMPAIGN_STOP_BEFORE_EPOCH
((CAMPAIGN_STOP_BEFORE_EPOCH - now_epoch >= MINIMUM_WORK_WINDOW_SECONDS)) || \
    die "Fenêtre de travail insuffisante avant le premier coupe-circuit."

git_sha="$(git -C "${SOURCE_ROOT}" rev-parse HEAD)" || die "SHA Git illisible."
[[ "${git_sha}" =~ ^[0-9a-f]{40}$ ]] || die "SHA Git non canonique."
worktree_status="$(GIT_OPTIONAL_LOCKS=0 git -C "${SOURCE_ROOT}" status --porcelain --untracked-files=normal)" || die "État Git illisible."
[[ -z "${worktree_status}" ]] || die "Le checkout distant doit être propre avant la campagne."

# La porte scientifique est testée dans le préflight avant que les deux
# exécutables ne soient invoqués et avant la première création de spool ou de
# temporaire de campagne.
python3 -B "${PREFLIGHT_SCRIPT}" --plan "${CAMPAIGN_PLAN}" \
    --producer "${PRODUCER_PATH}" --verifier "${VERIFIER_PATH}" \
    --expected-git-sha "${git_sha}"

CAMPAIGN_TEMP_DIR="$(mktemp -d /tmp/morsehgp3d-point-quality-campaign.XXXXXXXX)" || \
    die "Impossible de créer le répertoire temporaire de campagne."
chmod 700 "${CAMPAIGN_TEMP_DIR}" || die "Impossible de protéger le temporaire de campagne."
CAMPAIGN_TEMP_OUTPUT="${CAMPAIGN_TEMP_DIR}/campaign-summary.json"

cleanup() {
    local status=$?
    trap - EXIT HUP INT TERM
    if [[ -n "${CAMPAIGN_TEMP_OUTPUT}" && -e "${CAMPAIGN_TEMP_OUTPUT}" ]]; then
        rm -f -- "${CAMPAIGN_TEMP_OUTPUT}" || true
    fi
    if [[ -n "${CAMPAIGN_TEMP_DIR}" && -d "${CAMPAIGN_TEMP_DIR}" ]]; then
        rmdir -- "${CAMPAIGN_TEMP_DIR}" || true
    fi
    exit "${status}"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

remaining_seconds=$((CAMPAIGN_STOP_BEFORE_EPOCH - $(date +%s)))
((remaining_seconds >= MINIMUM_WORK_WINDOW_SECONDS)) || die "Fenêtre insuffisante avant la campagne."
timeout --kill-after="${COMMAND_KILL_GRACE_SECONDS}s" "${remaining_seconds}s" \
    python3 -B "${CAMPAIGN_SCRIPT}" \
        --producer "${PRODUCER_PATH}" \
        --verifier "${VERIFIER_PATH}" \
        --spool "${SPOOL_PATH}" \
        --output "${CAMPAIGN_TEMP_OUTPUT}" \
        --plan "${CAMPAIGN_PLAN}" \
        --stop-before-epoch "${CAMPAIGN_STOP_BEFORE_EPOCH}"

[[ -s "${CAMPAIGN_TEMP_OUTPUT}" && ! -L "${CAMPAIGN_TEMP_OUTPUT}" ]] || \
    die "La campagne n'a pas produit un résumé provisoire régulier non vide."

python3 -B - \
    "${CAMPAIGN_TEMP_OUTPUT}" "${OUTPUT_PATH}" "${ENVIRONMENT_ARTIFACT}" \
    "${CAMPAIGN_PLAN}" "${CAMPAIGN_SCRIPT}" "${PRODUCER_PATH}" \
    "${VERIFIER_PATH}" "${git_sha}" "${WORKER_SCHEMA}" \
    "${shutdown_epoch}" "${GCE_DEADLINE_EPOCH}" \
    "${CAMPAIGN_STOP_BEFORE_EPOCH}" <<'PY'
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import tempfile

campaign_path = Path(sys.argv[1])
output_path = Path(sys.argv[2])
environment_path = Path(sys.argv[3])
plan_path = Path(sys.argv[4])
script_path = Path(sys.argv[5])
generator_path = script_path.with_name("point_hierarchy_quality_generator.py")
producer_path = Path(sys.argv[6])
verifier_path = Path(sys.argv[7])
git_sha = sys.argv[8]
schema = sys.argv[9]
shutdown_epoch = int(sys.argv[10])
gce_deadline_epoch = int(sys.argv[11])
stop_before_epoch = int(sys.argv[12])


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


with campaign_path.open(encoding="utf-8") as stream:
    campaign = json.load(stream)
if not isinstance(campaign, dict) or campaign.get("schema") != "morsehgp3d.point_hierarchy.quality_campaign_summary.v2":
    raise SystemExit("résumé de campagne v2 incompatible")
status = campaign.get("status")
if status not in {"complete", "partial_resumable"}:
    raise SystemExit("statut de campagne v2 incompatible")
completed = campaign.get("completed_transaction_count")
next_ordinal = campaign.get("next_transaction_ordinal")
total = campaign.get("total_transaction_count")
receipts = campaign.get("receipts")
if (
    type(completed) is not int
    or type(next_ordinal) is not int
    or type(total) is not int
    or not isinstance(receipts, list)
    or completed != next_ordinal
    or completed != len(receipts)
    or not (0 <= completed <= total == 84)
):
    raise SystemExit("préfixe transactionnel de campagne invalide")
if status == "complete":
    if completed != total or campaign.get("exact_source_build_count") != 6 or campaign.get("stop_reason") != "all_transactions_complete":
        raise SystemExit("campagne complète sans fermeture des 84 transactions")
elif completed >= total or campaign.get("stop_reason") != "insufficient_time_for_next_transaction":
    raise SystemExit("résumé partiel non reprenable")
if campaign.get("session_stop_before_epoch") != stop_before_epoch:
    raise SystemExit("échéance du résumé de campagne différente de la garde invitée")
if campaign.get("claims") != {
    "deployment_status": "architecture_only",
    "public_exact_status_claimed": False,
    "quality_campaign_promotes_exact": False,
}:
    raise SystemExit("le résumé de campagne surestime son statut")
if re.fullmatch(r"[0-9a-f]{64}", str(campaign.get("campaign_id"))) is None:
    raise SystemExit("identité de campagne invalide")
if re.fullmatch(r"[0-9a-f]{64}", str(campaign.get("ordered_receipt_chain_sha256"))) is None:
    raise SystemExit("chaîne de reçus de campagne invalide")

with environment_path.open(encoding="utf-8") as stream:
    environment = json.load(stream)
if not isinstance(environment, dict) or environment.get("schema") != "morsehgp3d.phase3.qualification.v1":
    raise SystemExit("artefact d'environnement Phase 3 incompatible")

plan_sha256 = sha256(plan_path)
if campaign.get("plan_sha256") != plan_sha256:
    raise SystemExit("le résumé de campagne n'est pas lié au plan v2")

pending_status = (
    "worker_complete_pending_shutdown"
    if status == "complete"
    else "worker_partial_resumable_pending_shutdown"
)
document = {
    "campaign": campaign,
    "provenance": {
        "campaign_script_sha256": sha256(script_path),
        "environment_artifact_schema": environment["schema"],
        "environment_artifact_sha256": sha256(environment_path),
        "generator_sha256": sha256(generator_path),
        "git_sha": git_sha,
        "plan_sha256": plan_sha256,
        "producer_sha256": sha256(producer_path),
        "verifier_sha256": sha256(verifier_path),
    },
    "schema": schema,
    "spool": {
        "campaign_id": campaign["campaign_id"],
        "next_transaction_ordinal": next_ordinal,
        "persistent_across_guarded_sessions": True,
        "resume_from_next_uncommitted_ordinal": True,
        "total_transaction_count": total,
    },
    "status": pending_status,
    "vm_lifecycle": {
        "gce_safe_deadline_epoch": gce_deadline_epoch,
        "guest_shutdown_epoch": shutdown_epoch,
        "worker_stop_before_epoch": stop_before_epoch,
        "guest_shutdown_guard_verified": True,
        "stop_responsibility": "external_orchestrator",
        "worker_mutates_gcp": False,
    },
}
encoded = (json.dumps(document, sort_keys=True, separators=(",", ":")) + "\n").encode()
descriptor, temporary_name = tempfile.mkstemp(
    prefix=f".{output_path.name}.", suffix=".partial", dir=output_path.parent
)
temporary = Path(temporary_name)
try:
    offset = 0
    while offset < len(encoded):
        offset += os.write(descriptor, encoded[offset:])
    os.fsync(descriptor)
    os.close(descriptor)
    descriptor = -1
    os.link(temporary, output_path, follow_symlinks=False)
    os.unlink(temporary)
    directory = os.open(output_path.parent, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
    try:
        os.fsync(directory)
    finally:
        os.close(directory)
finally:
    if descriptor >= 0:
        os.close(descriptor)
    temporary.unlink(missing_ok=True)
PY

printf '[SUCCÈS] Session qualité v2 terminée; résumé provisoire en attente de l’arrêt ciblé : %s\n' \
    "${OUTPUT_PATH}"
