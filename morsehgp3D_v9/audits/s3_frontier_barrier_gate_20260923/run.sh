#!/usr/bin/env bash
set -euo pipefail

repo=$(git rev-parse --show-toplevel)
here="$repo/morsehgp3D_v9/audits/s3_frontier_barrier_gate_20260923"
tmp=$(mktemp -d /tmp/mhgp9-s3-frontier-barrier.XXXXXXXX)
trap 'rm -rf "$tmp"' EXIT

for variant in frozen mutable mutant; do
  mkdir -p "$tmp/$variant/morsehgp3D_v9/src/gpu"
done
git -C "$repo" show 50dabc0fa:morsehgp3D_v9/src/gpu/certificate.hpp \
  > "$tmp/frozen/morsehgp3D_v9/src/gpu/certificate.hpp"
git -C "$repo" show 50dabc0fa:morsehgp3D_v9/src/gpu/witness_filter.hpp \
  > "$tmp/frozen/morsehgp3D_v9/src/gpu/witness_filter.hpp"
for variant in mutable mutant; do
  cp "$tmp/frozen/morsehgp3D_v9/src/gpu/certificate.hpp" \
    "$tmp/$variant/morsehgp3D_v9/src/gpu/certificate.hpp"
  cp "$tmp/frozen/morsehgp3D_v9/src/gpu/witness_filter.hpp" \
    "$tmp/$variant/morsehgp3D_v9/src/gpu/witness_filter.hpp"
  patch --quiet --directory "$tmp/$variant" -p1 < "$here/mutable_certificate.patch"
done

python3 - "$tmp/mutant/morsehgp3D_v9/src/gpu/certificate.hpp" <<'PY'
from pathlib import Path
import sys

path = Path(sys.argv[1])
source = path.read_text()
old = "    group.sync();\n    const u32* frontier ="
new = "    // MUTANT: the entry barrier alone is removed.\n    const u32* frontier ="
if source.count(old) != 1:
    raise SystemExit("entry barrier anchor absent or ambiguous")
path.write_text(source.replace(old, new))
PY

(
  cd "$tmp"
  sha256sum --check "$here/HEADERS.sha256"
)

for variant in frozen mutable mutant; do
  g++ -std=c++20 -O2 -Wall -Wextra -Werror \
    -I "$tmp/$variant/morsehgp3D_v9/src/gpu" \
    "$here/gate.cpp" -o "$tmp/$variant/gate"
  expected=unsafe
  if [[ "$variant" == mutable ]]; then expected=safe; fi
  "$tmp/$variant/gate" "$expected" > "$tmp/$variant.stdout"
  diff -u "$here/$variant.stdout" "$tmp/$variant.stdout"
done
echo 's3_frontier_barrier_gate: PASS (frozen unsafe, mutable safe, barrier mutant unsafe)'
