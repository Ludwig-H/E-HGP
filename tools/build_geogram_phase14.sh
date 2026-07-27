#!/usr/bin/env bash
set -euo pipefail

readonly GEOGRAM_TAG="v1.10.0"
readonly GEOGRAM_SHA="c8529bb00838186938ab31d96008a59b6a892dee"

usage() {
  echo "usage: $0 --source-dir ABSOLUTE_PATH --prefix ABSOLUTE_PATH [--jobs N]" >&2
}

source_dir=""
install_prefix=""
jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
while (($#)); do
  case "$1" in
    --source-dir)
      source_dir="${2:-}"
      shift 2
      ;;
    --prefix)
      install_prefix="${2:-}"
      shift 2
      ;;
    --jobs)
      jobs="${2:-}"
      shift 2
      ;;
    *)
      usage
      exit 2
      ;;
  esac
done

if [[ "$source_dir" != /* || "$install_prefix" != /* ]]; then
  usage
  echo "source and install paths must be absolute" >&2
  exit 2
fi
if [[ "$source_dir" == "/" || "$install_prefix" == "/" ]]; then
  echo "refusing a root source or install path" >&2
  exit 2
fi
if [[ ! "$jobs" =~ ^[1-9][0-9]*$ ]]; then
  echo "--jobs must be a positive integer" >&2
  exit 2
fi

if [[ ! -d "$source_dir/.git" ]]; then
  if [[ -e "$source_dir" ]]; then
    echo "source path exists but is not a Geogram Git checkout: $source_dir" >&2
    exit 1
  fi
  git clone --recursive --branch "$GEOGRAM_TAG" \
    https://github.com/BrunoLevy/geogram.git "$source_dir"
fi

observed_sha="$(git -C "$source_dir" rev-parse HEAD)"
if [[ "$observed_sha" != "$GEOGRAM_SHA" ]]; then
  echo "unexpected Geogram SHA: $observed_sha (expected $GEOGRAM_SHA)" >&2
  exit 1
fi
if [[ -n "$(git -C "$source_dir" status --porcelain --untracked-files=no)" ]]; then
  echo "the pinned Geogram checkout has tracked modifications" >&2
  exit 1
fi

build_dir="$source_dir/build-phase14-low-order"
cmake -S "$source_dir" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE=Release \
  -DVORPALINE_PLATFORM=Linux64-gcc-dynamic \
  -DGEOGRAM_LIB_ONLY=ON \
  -DGEOGRAM_WITH_GRAPHICS=OFF \
  -DGEOGRAM_WITH_LUA=OFF \
  -DGEOGRAM_WITH_TETGEN=OFF \
  -DGEOGRAM_WITH_TRIANGLE=OFF \
  -DGEOGRAM_WITH_GARGANTUA=OFF \
  -DGEOGRAM_WITH_TBB=OFF
cmake --build "$build_dir" --parallel "$jobs"
cmake --install "$build_dir" --prefix "$install_prefix"

echo "Geogram $GEOGRAM_TAG ($GEOGRAM_SHA) installed in $install_prefix"
