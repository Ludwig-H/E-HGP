#!/bin/bash
S=$1; m=$2
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -isystem /workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include -O2 $S/mut/$m/morsehgp3D_v7/tests/full_coverage_certificate_gate.cpp -o $S/bin/mut_$m 2> $S/mut/$m/compile.stderr
crc=$?
if [ $crc -ne 0 ]; then echo "$m COMPILE_FAIL rc=$crc $(head -c 200 $S/mut/$m/compile.stderr | tr '\n' ' ')"; exit 0; fi
timeout 60 $S/bin/mut_$m --selftest > $S/mut/$m/selftest.stdout 2> $S/mut/$m/selftest.stderr
rc=$?
echo "$m rc=$rc $(cat $S/mut/$m/selftest.stderr | head -1) $(cat $S/mut/$m/selftest.stdout | head -1)"
