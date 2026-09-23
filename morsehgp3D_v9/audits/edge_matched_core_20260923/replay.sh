#!/usr/bin/env bash
# Rebuild and rerun the audit outside the repository; needs the pinned v8
# LiDAR inputs, CMake, a C++20 compiler and Boost headers.
set -euo pipefail

if (($# != 1)); then
  echo 'usage: replay.sh /tmp/new-audit-directory' >&2
  exit 2
fi
run_root=$1
case "$run_root" in /tmp/*) ;; *) echo 'output must be under /tmp' >&2; exit 2 ;; esac
if [[ -e "$run_root" ]]; then
  echo 'refusing existing output directory' >&2
  exit 2
fi
audit_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo=$(cd "$audit_dir/../../.." && pwd)
mkdir -p "$run_root"
python3 -B "$repo/morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/generate.py" \
  --repo "$repo" --out "$run_root/inputs"

cmake_args=(-DCMAKE_BUILD_TYPE=Release -DMHGP9_ENABLE_CUDA=OFF)
if [[ -n "${MHGP9_AUDIT_BOOST_ROOT:-}" ]]; then
  cmake_args+=("-DBOOST_ROOT=$MHGP9_AUDIT_BOOST_ROOT")
elif [[ -d "$repo/build/v7_boost_gate/extracted/usr/include/boost" ]]; then
  cmake_args+=("-DBOOST_ROOT=$repo/build/v7_boost_gate/extracted/usr")
fi

python3 -B "$audit_dir/prepare_source.py" --repo "$repo" --out "$run_root/before"
cmake -S "$run_root/before/morsehgp3D_v9" -B "$run_root/before/build" "${cmake_args[@]}"
cmake --build "$run_root/before/build" --target mhgp9_tower_probe -j 4

sectors=(full quarter_x_neg_y_neg quarter_x_neg_y_nonneg
         quarter_x_nonneg_y_neg quarter_x_nonneg_y_nonneg)
for density in quarter half full; do
  case_root="$run_root/before/$density"
  if [[ "$density" == full ]]; then case_root="$run_root/before"; else mkdir -p "$case_root"; fi
  for sector in "${sectors[@]}"; do
    python3 -B "$audit_dir/run_case.py" \
      --binary "$run_root/before/build/mhgp9_tower_probe" \
      --inputs "$run_root/inputs" --out "$case_root/$sector" \
      --case "$sector" --density "$density"
  done
  python3 -B "$audit_dir/analyze.py" --inputs "$run_root/inputs" \
    --run-root "$case_root" --out "$run_root/before/DECOMPOSITION_${density^^}.json" \
    --density "$density" --patch-manifest "$run_root/before/PATCH_MANIFEST.json" \
    --binary "$run_root/before/build/mhgp9_tower_probe"
done

python3 -B "$audit_dir/prepare_source.py" --repo "$repo" --out "$run_root/after" --after-core
cmake -S "$run_root/after/morsehgp3D_v9" -B "$run_root/after/build" "${cmake_args[@]}"
cmake --build "$run_root/after/build" --target mhgp9_tower_probe -j 4
python3 -B "$audit_dir/run_case.py" \
  --binary "$run_root/after/build/mhgp9_tower_probe" \
  --inputs "$run_root/inputs" --out "$run_root/after/full" \
  --case full --density full --trace-format before_after
python3 -B "$audit_dir/analyze_after.py" --inputs "$run_root/inputs" \
  --before-run "$run_root/before/full" --after-run "$run_root/after/full" \
  --after-patch-manifest "$run_root/after/PATCH_MANIFEST.json" \
  --after-binary "$run_root/after/build/mhgp9_tower_probe" \
  --out "$run_root/after/POST_CORE_FULL.json"
python3 -B "$audit_dir/freeze.py" --before-root "$run_root/before" \
  --after-root "$run_root/after" --out "$run_root/compact"
