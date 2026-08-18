#!/usr/bin/env bash
# PIN DU PROTOCOLE scale_threads (meme discipline que v4_campaign_pin.sh,
# audits « pin du protocole » puis 9223888/b3a6eb4) : refuse un worktree
# qui differe de HEAD sur les chemins normatifs de CETTE session, puis
# materialise bundle et scripts epingles depuis le COMMIT.
#
# FERMETURE TRANSITIVE (audit bloquant 9d19ede) : le protocole REELLEMENT
# execute est plus grand que le moteur, le runner et le validateur. La
# session lit ses constantes de garde dans `start_and_verify.sh`, demarre
# la VM par `start_and_verify.sh`, pose le coupe-circuit GCE par
# `set_max_run_duration_and_verify.sh` et certifie l'arret par
# `stop_and_verify.sh`. Tant que ces trois fichiers ne sont ni dans les
# chemins normatifs ni dans le manifeste, une modification locale non
# commitee decide du demarrage et de l'arret sans laisser de trace — le
# cas le plus dangereux etant un `stop_and_verify.sh` mute qui rend zero
# et fait annoncer un arret certifie. Ils sont donc pinnes ET
# MATERIALISES : la session n'execute que la copie extraite du commit,
# ce qui ferme aussi la fenetre TOCTOU entre le pin et l'execution
# (`start_and_verify.sh` appelant `stop_and_verify.sh` relativement a son
# propre BASH_SOURCE, les deux dans le meme repertoire pinne ferment le
# chemin d'urgence).
# Usage : v4_scale_threads_pin.sh WORK_DIR
set -euo pipefail
WORK="${1:?repertoire de travail requis}"
mkdir -p "${WORK}"

# Gardes locales : lues et executees par la session, donc normatives.
LOCAL_GUARD_PATHS=(
  gcp-migration/set_max_run_duration_and_verify.sh
  gcp-migration/start_and_verify.sh
  gcp-migration/stop_and_verify.sh
)
PROTOCOL_PATHS=(
  morsehgp3D_v4
  gcp-migration/session_scale_threads_g4.sh
  gcp-migration/v4_scale_threads_remote.sh
  gcp-migration/validate_v4_scale_threads.py
  gcp-migration/v4_scale_threads_pin.sh
  "${LOCAL_GUARD_PATHS[@]}"
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
  gcp-migration/validate_v4_scale_threads.py \
  "${LOCAL_GUARD_PATHS[@]}"
SOURCE_PAYLOAD_SHA256="$(sha256sum "${BUNDLE}" | awk '{print $1}')"

mkdir -p "${WORK}/pinned"
tar xzf "${BUNDLE}" -C "${WORK}/pinned" \
  gcp-migration/v4_scale_threads_remote.sh \
  gcp-migration/validate_v4_scale_threads.py \
  "${LOCAL_GUARD_PATHS[@]}"
chmod +x "${WORK}/pinned/gcp-migration/"*.sh

# MANIFESTE A FRONTIERES EXPLICITES (audit 9d19ede § 3.1) : chaque entree
# serialise son CHEMIN et sa LONGUEUR avant son contenu. Une simple
# concatenation de contenus laisserait deux decoupages differents produire
# le meme digest.
MANIFEST_PATHS=(
  gcp-migration/session_scale_threads_g4.sh
  gcp-migration/v4_scale_threads_remote.sh
  gcp-migration/validate_v4_scale_threads.py
  "${LOCAL_GUARD_PATHS[@]}"
)
MANIFEST_BIN="${WORK}/protocol_manifest.bin"
MANIFEST_ENTRY="${WORK}/.protocol_manifest_entry"
: > "${MANIFEST_BIN}"
for p in "${MANIFEST_PATHS[@]}"; do
  git show "${SOURCE_COMMIT}:${p}" > "${MANIFEST_ENTRY}"
  printf '%s\n%s\n' "${p}" "$(wc -c < "${MANIFEST_ENTRY}")" >> "${MANIFEST_BIN}"
  cat "${MANIFEST_ENTRY}" >> "${MANIFEST_BIN}"
done
rm -f "${MANIFEST_ENTRY}"
PROTOCOL_MANIFEST_SHA256="$(sha256sum "${MANIFEST_BIN}" | awk '{print $1}')"

echo "source_commit=${SOURCE_COMMIT}"
echo "source_payload_sha256=${SOURCE_PAYLOAD_SHA256}"
echo "protocol_manifest_sha256=${PROTOCOL_MANIFEST_SHA256}"
echo "pinned_guard_dir=${WORK}/pinned/gcp-migration"
