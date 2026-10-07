#!/bin/bash
# Contre-epreuve du generateur v10 (HEAD afb081774) contre l'oracle brut independant : chemins non couverts par la
# porte du depot (K = 10 et 12, masques a plusieurs mots m > 64, m > 128, m > 256, poids, bords du domaine u18).
C=${1:-/tmp/v11-audit/l05_code_catalogue/build-rel/mhgp10_catalogue}
O=/tmp/v11-audit/l05_code_catalogue/oracle
run() { # fichier K options...
  local f=$1 k=$2; shift 2
  local t0=$SECONDS
  out=$(python3 $O/compare.py $C $O/brute_oracle $O/$f $k "$@" 2>&1); rc=$?
  printf '%-18s K=%-2s %-40s code=%d %3ds %s\n' "$f" "$k" "$*" "$rc" "$((SECONDS - t0))" "$(echo "$out" | head -1 | cut -c1-105)"
}
date -u
for k in 1 2 3 5 10 12; do run sph26_plus.u32le $k --threads=2; done
for k in 1 2 3 5 10; do run sph26_dup.u32le $k --threads=2; done
for k in 2 5 10 12; do run rnd90_s12.u32le $k --threads=2; done
for k in 2 5 10 12; do run rnd110_s40.u32le $k --threads=2; done
for k in 3 10; do run rnd140_s100.u32le $k --threads=2; done
for k in 1 2 5 10 12; do run rnd70_w.u32le $k --threads=2; done
for k in 1 2 5 10; do run plan120.u32le $k --threads=2; done
for k in 1 3; do run droite40.u32le $k --threads=2; done
# feuilles larges forcees : masques a 2, 4 mots et nombre de mots a l'execution
run rnd110_s40.u32le 5 --threads=2 --leaf=120
run rnd110_s40.u32le 10 --threads=2 --leaf=120
run rnd200_s60.u32le 5 --threads=2 --leaf=250
run rnd200_s60.u32le 10 --threads=2 --leaf=250
run rnd280_s100.u32le 3 --threads=2 --leaf=300 --max-leaf=1000
run rnd280_s100.u32le 5 --threads=2 --leaf=300 --max-leaf=1000
run rnd280_s100.u32le 10 --threads=2 --leaf=300 --max-leaf=1000
run rnd280_s100.u32le 10 --threads=2
# spheres entieres : feuilles bloquees a m = 144 (4 mots) et m = 312 (mots a l'execution)
run sph89.u32le 2 --threads=2
run sph89_plus.u32le 3 --threads=2
run sph314.u32le 2 --threads=2 --max-leaf=1000
run sph314.u32le 2 --threads=2
date -u
