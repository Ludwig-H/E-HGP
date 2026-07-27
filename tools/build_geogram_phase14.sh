#!/usr/bin/env bash
set -euo pipefail

readonly GEOGRAM_TAG="v1.10.0"
readonly GEOGRAM_SHA="c8529bb00838186938ab31d96008a59b6a892dee"

usage() {
  echo "usage: $0 --source-dir ABSOLUTE_PATH --prefix ABSOLUTE_PATH [--build-dir ABSOLUTE_PATH] [--jobs N]" >&2
}

source_dir=""
install_prefix=""
build_dir=""
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
    --build-dir)
      build_dir="${2:-}"
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

if [[ "$source_dir" != /* || "$install_prefix" != /* || ( -n "$build_dir" && "$build_dir" != /* ) ]]; then
  usage
  echo "source, install, and explicit build paths must be absolute" >&2
  exit 2
fi
source_dir="$(realpath -m -- "$source_dir")"
install_prefix="$(realpath -m -- "$install_prefix")"
if [[ -z "$build_dir" ]]; then
  build_dir="${source_dir}-build-phase14-low-order"
else
  build_dir="$(realpath -m -- "$build_dir")"
fi
if [[ "$source_dir" == "/" || "$install_prefix" == "/" || "$build_dir" == "/" ]]; then
  echo "refusing a root source, install, or build path" >&2
  exit 2
fi
if [[ ! "$jobs" =~ ^[1-9][0-9]*$ ]]; then
  echo "--jobs must be a positive integer" >&2
  exit 2
fi
if [[ "$build_dir" == "$source_dir" || "$build_dir" == "$source_dir"/* ]]; then
  echo "the Geogram build directory must be outside the source checkout" >&2
  exit 2
fi
if [[ "$install_prefix" == "$source_dir" || "$install_prefix" == "$source_dir"/* ]]; then
  echo "the Geogram install prefix must be outside the source checkout" >&2
  exit 2
fi
if [[ -e "$build_dir" ]]; then
  echo "refusing a non-fresh Geogram build path: $build_dir" >&2
  exit 1
fi
if [[ -e "$install_prefix" ]]; then
  if [[ ! -d "$install_prefix" || -n "$(find "$install_prefix" -mindepth 1 -print -quit)" ]]; then
    echo "refusing a non-empty Geogram install prefix: $install_prefix" >&2
    exit 1
  fi
fi

cxx_compiler="${CXX:-c++}"
if ! command -v "$cxx_compiler" >/dev/null 2>&1; then
  echo "C++ compiler not found: $cxx_compiler" >&2
  exit 1
fi
tbb_library="$($cxx_compiler -print-file-name=libtbb.so)"
if [[ "$tbb_library" == "libtbb.so" || ! -f "$tbb_library" ]]; then
  echo "Geogram v1.10.0 links libtbb even with GEOGRAM_WITH_TBB=OFF; install libtbb-dev" >&2
  exit 1
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
if [[ -n "$(git -C "$source_dir" status --porcelain --untracked-files=all)" ]]; then
  echo "the pinned Geogram checkout must contain no tracked or untracked changes" >&2
  exit 1
fi

cmake -S "$source_dir" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$install_prefix" \
  -DVORPALINE_PLATFORM=Linux64-gcc-dynamic \
  -DGEOGRAM_LIB_ONLY=ON \
  -DGEOGRAM_WITH_GRAPHICS=OFF \
  -DGEOGRAM_WITH_HLBFGS=OFF \
  -DGEOGRAM_WITH_LEGACY_NUMERICS=OFF \
  -DGEOGRAM_WITH_LUA=OFF \
  -DGEOGRAM_WITH_TETGEN=OFF \
  -DGEOGRAM_WITH_TRIANGLE=OFF \
  -DGEOGRAM_WITH_GARGANTUA=OFF \
  -DGEOGRAM_WITH_TBB=OFF
cmake --build "$build_dir" --parallel "$jobs"
cmake --install "$build_dir"

if [[ -n "$(git -C "$source_dir" status --porcelain --untracked-files=all)" ]]; then
  echo "the Geogram build modified its pinned source checkout" >&2
  exit 1
fi
for installed_file in \
  "$install_prefix/lib/libgeogram.so" \
  "$install_prefix/lib/cmake/modules/FindGeogram.cmake" \
  "$install_prefix/include/geogram1/geogram/basic/common.h"; do
  if [[ ! -e "$installed_file" ]]; then
    echo "missing installed Geogram artifact: $installed_file" >&2
    exit 1
  fi
done

echo "Geogram $GEOGRAM_TAG ($GEOGRAM_SHA) installed in $install_prefix"
