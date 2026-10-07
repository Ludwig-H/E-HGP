#!/bin/bash
# Audit L12 (2 octobre 2026) : rejeu, sur le code v10 PUBLIE (copie /tmp des sources du worktree de lecture,
# moteur inchange depuis 4b7d70422), des defauts releves par les auditeurs. Lecture seule sur le depot.
# Usage : rejeux_code_publie.sh BUILD_RELEASE BUILD_ASAN SOURCES_COPIE  (repertoire courant = dossier de travail /tmp)
set -u
B=$1; BA=$2; SRC=$3
ulimit -c 0
run() { echo "--- $(echo "$*" | sed "s|$B/||g")"; timeout 20 "$@" 2>&1 | cut -c1-150 | tail -2; echo "[code=${PIPESTATUS[0]}]"; }
echo "== sources : $(cd $SRC && sha256sum src/sched/pool.cpp src/head/head.cpp src/cloud/site_tree.cpp src/tower/tower.cpp cli/mhgp10_tower.cpp cli/mhgp10_catalogue.cpp cli/mhgp10_cluster.cpp | cut -c1-16,65- | tr '\n' ';')"
echo "== 1. CLI tour : export sans attaches (auditeur continu C3, independant I2)"
run $B/mhgp10_tower three.u32le --k=2 --threads=1 --no-points --dump=o_nopoints.txt
run $B/mhgp10_tower four.u32le --k=2 --threads=1 --repeat=0
echo "== 2. Fins de fichier partielles acceptees (I1, C6)"
for e in 1 4 8 11; do run $B/mhgp10_catalogue two_plus_$e.u32le --k=2 --threads=1; done
run $B/mhgp10_tower two_plus_11.u32le --k=2 --threads=1
echo "== 3. Options numeriques (CL1)"
run $B/mhgp10_catalogue four.u32le --k=abc --threads=1
run $B/mhgp10_catalogue four.u32le --k=2not_an_integer --threads=1
run $B/mhgp10_catalogue four.u32le --k=2 --threads=4294967297
run $B/mhgp10_catalogue four.u32le --k=2 --threads=1 --leaf=4294967304
echo "== 4. Feuille plus petite que K (C9) : huit coins du cube u18, K5"
run $B/mhgp10_catalogue corners.u32le --k=5 --threads=1
echo "--- mhgp10_catalogue corners.u32le --k=5 --threads=1 --leaf=2 (delai 10 s)"; timeout 10 $B/mhgp10_catalogue corners.u32le --k=5 --threads=1 --leaf=2 > /dev/null 2>&1; echo "[code=$?]"
echo "== 5. CLI cluster : un point a K2 ; etiquettes et arbre vers le meme fichier ; z = 0 ; K1/mcs1"
run $B/mhgp10_cluster one.u32le o_one.lab --k=2 --mcs=2 --threads=1
run $B/mhgp10_cluster five.u32le o_ctrl.lab --k=2 --mcs=2 --threads=1
run $B/mhgp10_cluster five.u32le o_same.out --k=2 --mcs=2 --threads=1 --tree=o_same.out
echo "tailles : etiquettes de controle $(stat -c %s o_ctrl.lab) octets ; fichier commun $(stat -c %s o_same.out) octets ; debut : $(head -c 9 o_same.out | tr '\n' ' ')"
run $B/mhgp10_cluster five.u32le o_z0.lab --k=2 --mcs=2 --threads=1 --z=0
run $B/mhgp10_cluster five.u32le o_k1.lab --k=1 --mcs=1 --threads=1
echo "== 6. Pool : exception de l'appelant (ASan) et d'un ouvrier (C2, P1)"
ASAN_OPTIONS=detect_stack_use_after_return=1:halt_on_error=1 timeout 30 ./pool_throw_asan > o_pool.out 2> o_pool.err; echo "[code=$?] $(cat o_pool.out) ; $(grep -m1 -o 'ERROR: AddressSanitizer: [a-z-]*' o_pool.err)"
timeout 20 ./pool_worker_throw > o_poolw.out 2>&1; echo "[code=$?] $(tail -1 o_poolw.out)"
echo "== 7. Tete : departs de points, H3, zero (TT1, H3, addendum zero)"
./head_probe; echo "[code=$?]"
echo "== 8. Tete : validate trop faible (H1) sous ASan"
for m in rank missing-edge nan-level nan-z negative-z; do timeout 30 ./head_validate_asan $m > o_hv.out 2> o_hv.err; echo "mode=$m code=$? :: $(tr '\n' ' ' < o_hv.out):: $(grep -m1 -o 'ERROR: AddressSanitizer: [a-z-]*' o_hv.err)"; done
echo "== 9. Tete : peigne (H4)"
./head_ladder; echo "[code=$?]"
echo "== 10. SiteTree, centre hors enveloppe (G1)"
./geometry_site_tree; echo "[code=$?]"
echo "== 11. Juge vertical publie : mutant survivant"
rm -rf o_vertical && PYTHONDONTWRITEBYTECODE=1 timeout 120 python3 -B probe_vertical.py $B/mhgp10_tower $SRC o_vertical | grep -E '"(baseline_gate|mutated_gate|outcome)"' -A2 | tr -d '\n' | tr -s ' '; echo
echo "== 12. Rangs exacts coalesces dans le dendrogramme de points (trois sites)"
timeout 20 $B/mhgp10_tower coalesce.u32le --k=2 --threads=1 --dump=o_coal.txt > /dev/null; grep -A4 "^order 2" o_coal.txt | tr '\n' ';'; echo
timeout 20 $B/mhgp10_cluster coalesce.u32le o_coal.lab --k=2 --mcs=2 --threads=1 --entry=cover --tree=o_coal.tree > /dev/null; tr '\n' ';' < o_coal.tree; echo
echo "== 13. Tete R2 privee (head.cpp a513aebd) : meme defaut des departs de points"
./head_probe_r2
echo "== fin $(date -u +%Y-%m-%dT%H:%M:%SZ)"
