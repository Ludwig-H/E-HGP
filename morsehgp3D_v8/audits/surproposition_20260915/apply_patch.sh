#!/bin/sh
# Auditeur B — reconstruit la copie d'audit du moteur 2741d614 avec l'extension de fenêtre de Front::filter.
# usage : apply_patch.sh <repo> <destination> ; puis cmake -S <destination>/morsehgp3D_v8 -B <destination>/build -DCMAKE_BUILD_TYPE=Release
set -e
repo="$1"; dest="$2"; here="$(cd "$(dirname "$0")" && pwd)"
mkdir -p "$dest"
git -C "$repo" archive 2741d614 morsehgp3D_v8/src morsehgp3D_v8/bench morsehgp3D_v8/tests morsehgp3D_v8/CMakeLists.txt morsehgp3D_v8/cmake | tar -x -C "$dest"
patch -p1 -d "$dest" < "$here/front_filter.patch"
sha256sum "$dest/morsehgp3D_v8/src/wspd/front.cpp" "$dest/morsehgp3D_v8/src/wspd/front.hpp"
