#!/usr/bin/env bash
# PIN DU PROTOCOLE scale_threads (meme discipline que v4_campaign_pin.sh,
# audits « pin du protocole » puis 9223888/b3a6eb4) : refuse un worktree
# qui differe de HEAD sur les chemins normatifs de CETTE session, puis
# materialise bundle et scripts epingles depuis le COMMIT.
# Usage : v4_scale_threads_pin.sh WORK_DIR
set -euo pipefail
WORK="${1:?repertoire de travail requis}"

PROTOCOL_PATHS=(
  morsehgp3D_v4
  gcp-migration/session_scale_threads_g4.sh
  gcp-migration/v4_scale_threads_remote.sh
  gcp-migration/validate_v4_scale_threads.py
  gcp-migration/v4_scale_threads_pin.sh
)
SOURCE_COMMIT="$(git rev-parse HEAD)"
git diff --quiet -- "${PROTOCOL_PATHS[@]}" || {
  echo "REFUS : chemins normatifs modifies dans le worktree — committer d'abord" >&2
  exit 2
}
git diff --cached --quiet -- "${PROTOCOL_PATHS[@]}" || {
  echo "REFUS : chemins normatifs modifies dans l'index — committer d'abord" >&2
  exit 2
}
if [ -n "$(git ls-files --others --exclude-standard -- "${PROTOCOL_PATHS[@]}")" ]; then
  echo "REFUS : fichiers non suivis dans les chemins normatifs" >&2
  exit 2
fi

BUNDLE="${WORK}/bundle.tgz"
git archive --format=tar.gz -o "${BUNDLE}" "${SOURCE_COMMIT}" \
  morsehgp3D_v4 \
  gcp-migration/v4_scale_threads_remote.sh \
  gcp-migration/validate_v4_scale_threads.py
SOURCE_PAYLOAD_SHA256="$(sha256sum "${BUNDLE}" | awk '{print $1}')"

mkdir -p "${WORK}/pinned"
tar xzf "${BUNDLE}" -C "${WORK}/pinned" \
  gcp-migration/v4_scale_threads_remote.sh \
  gcp-migration/validate_v4_scale_threads.py

PROTOCOL_MANIFEST_SHA256="$({
  git show "${SOURCE_COMMIT}:gcp-migration/session_scale_threads_g4.sh"
  git show "${SOURCE_COMMIT}:gcp-migration/v4_scale_threads_remote.sh"
  git show "${SOURCE_COMMIT}:gcp-migration/validate_v4_scale_threads.py"
} | sha256sum | awk '{print $1}')"

echo "source_commit=${SOURCE_COMMIT}"
echo "source_payload_sha256=${SOURCE_PAYLOAD_SHA256}"
echo "protocol_manifest_sha256=${PROTOCOL_MANIFEST_SHA256}"
