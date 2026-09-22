#!/bin/bash
set -u
cd /workspaces/E-HGP
S=/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/gprof
B=build/v8-dev-gprof
cmake -S morsehgp3D_v8 -B $B -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-pg -fno-omit-frame-pointer" -DCMAKE_EXE_LINKER_FLAGS="-pg" -DBOOST_ROOT=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr > $S/configure.log 2>&1 || { echo "configure failed"; exit 1; }
cmake --build $B --parallel 3 --target mhgp8_wspd_q34_probe > $S/build.log 2>&1 || { echo "build failed"; exit 1; }
echo "built $(date -u +%H:%M:%S)"
IN=morsehgp3D_v8/audits/lidar08_20260914/prepared/ground_u16/scene_00/full.u16le
cd $S
/usr/bin/time -v -o $S/time_K5_W1.txt /workspaces/E-HGP/$B/mhgp8_wspd_q34_probe /workspaces/E-HGP/$IN 39815 5 8 6 28 1 samples digest rectangle-pair boxes affine live 64 > $S/probe_K5_W1.json 2> $S/probe_K5_W1.stderr
echo "run exit $? $(date -u +%H:%M:%S)"
gprof -b -p /workspaces/E-HGP/$B/mhgp8_wspd_q34_probe $S/gmon.out > $S/gprof_flat_K5_W1.txt 2> $S/gprof.err
gprof -b -q /workspaces/E-HGP/$B/mhgp8_wspd_q34_probe $S/gmon.out > $S/gprof_call_K5_W1.txt 2>> $S/gprof.err
head -n 40 $S/gprof_flat_K5_W1.txt
touch $S/DONE
