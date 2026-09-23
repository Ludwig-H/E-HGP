#!/usr/bin/env bash
set -euo pipefail

audit_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_dir=$(cd "$audit_dir/../../.." && pwd)
cd "$repo_dir"

(cd "$audit_dir" && sha256sum -c SHA256SUMS)
sha256sum -c "$audit_dir/INPUT_SHA256SUMS"
work_dir=$(mktemp -d /tmp/mhgp9-weighted-guard-replay.XXXXXX)
echo "Replay directory: $work_dir"

git show \
  3fd9f155a18dd950e8db46a06f5cb745220a365c:morsehgp3D_v9/audits/rect_pair_shadow_b_20260923/probe.cpp \
  > "$work_dir/base.cpp"
printf '%s  %s\n' \
  7f4f2c4e50d0b9e4e3261288ad582abaffa3d9e971466d30a9878bb56c6c589c \
  "$work_dir/base.cpp" | sha256sum -c -
patch -o "$work_dir/weighted.cpp" "$work_dir/base.cpp" "$audit_dir/probe.patch"
printf '%s  %s\n' \
  b6f96ddad461ab49d9780cd67b64a177542b06eb3b3f994d93ceb50d705e3143 \
  "$work_dir/weighted.cpp" | sha256sum -c -

g++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -I build/v9-open-worktree/morsehgp3D_v9/src \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
  "$work_dir/weighted.cpp" \
  build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a -pthread \
  -o "$work_dir/weighted_probe"

python3 -B "$audit_dir/verify_math.py" | cmp "$audit_dir/MATH.json" -
timeout 150s "$work_dir/weighted_probe" \
  /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.u32le \
  /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.raw_return_ids.u32le \
  /tmp/mhgp9-edge-core-audit-20260923/full/trace \
  > "$work_dir/weighted.stdout"
python3 -B "$audit_dir/analyze.py" "$work_dir/weighted.stdout" \
  "$audit_dir/RESULTS.json" > "$work_dir/RESULTS.json"
rg '^CERT' "$work_dir/weighted.stdout" > "$work_dir/CERTIFICATES.tsv"
cmp "$audit_dir/CERTIFICATES.tsv" "$work_dir/CERTIFICATES.tsv"
python3 -B "$audit_dir/verify_certs.py" "$work_dir/CERTIFICATES.tsv" \
  "$audit_dir/RESULTS.json" \
  /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.u32le \
  /tmp/mhgp9-s2-scaling-20260923-inputs/s00_full_full.raw_return_ids.u32le
echo "Non-timing counts match RESULTS.json; timings in $work_dir/RESULTS.json"
