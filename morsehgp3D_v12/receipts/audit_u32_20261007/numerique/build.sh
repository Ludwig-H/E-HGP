#!/bin/sh
# Depuis la racine du worktree; seul l'executable temporaire indique est cree.
set -eu
c++ -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Werror \
  -DMHGP12_COORD_BITS=32 -I morsehgp3D_v12/src \
  morsehgp3D_v12/receipts/audit_u32_20261007/numerique/probe.cpp \
  morsehgp3D_v12/src/num/sphere.cpp morsehgp3D_v12/src/num/predicates.cpp \
  morsehgp3D_v12/src/num/centers.cpp morsehgp3D_v12/src/num/big.cpp -o "$1"
