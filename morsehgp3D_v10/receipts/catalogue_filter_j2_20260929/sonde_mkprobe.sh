#!/bin/bash
# Construit un binaire sonde (rdtsc autour de filter_node, 1 fil) a partir d'un arbre source v10.
# usage : mkprobe.sh SRC_V10_DIR OUT_DIR [CMAKE_EXTRA...]
set -e
SRC=$1; OUT=$2; shift 2
rm -rf $OUT/src; mkdir -p $OUT/src
cp -r $SRC/CMakeLists.txt $SRC/cmake $SRC/cli $SRC/src $SRC/tests $SRC/reference $OUT/src/ 2>/dev/null || true
G=$OUT/src/src/catalogue/generator.cpp
python3 - "$G" << 'PY'
import sys, re
p = sys.argv[1]
s = open(p).read()
s = s.replace('#include "catalogue/catalogue.hpp"', '#include "catalogue/catalogue.hpp"\n#include <x86intrin.h>\n#include <cstdio>\nnamespace mhgp10 { unsigned long long g_filter_cyc = 0; }', 1)
n = s.count('filter_node(C, L, Q, parent, cand);')
assert n == 1, n
s = s.replace('filter_node(C, L, Q, parent, cand);', '{ const unsigned long long t0_ = __rdtsc(); filter_node(C, L, Q, parent, cand); g_filter_cyc += __rdtsc() - t0_; }')
s = s.replace('  cat.t_boxes = since(t0);', '  cat.t_boxes = since(t0);\n  std::fprintf(stderr, "filter_gcyc %.4f\\n", g_filter_cyc / 1e9);', 1)
open(p, 'w').write(s)
PY
cmake -S $OUT/src -B $OUT/build -DCMAKE_BUILD_TYPE=Release "$@" > /dev/null
cmake --build $OUT/build --parallel 2 --target mhgp10_catalogue 2>&1 | grep -E "error|warning" || true
ls -la $OUT/build/mhgp10_catalogue
