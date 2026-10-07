#!/bin/bash
# Differentiel d'echelle : une descente ALTERNATIVE valide (saut vers k interieurs quelconques, representant du dernier
# morceau, t derniers sites de la coquille) doit rendre la meme tour, a l'octet pres.
cd /tmp/v11-audit/l06_code_tour/mutants
D=/workspaces/E-HGP/build/v10-g4-data-s1
: > alt_results.txt
./build_ALT/mhgp10_tower $D/lidar00_full.u32le --k=5 --threads=3 --dump=alt_l00_k5.dump > alt_l00_k5.json
echo "lidar00 K=5 core descente alternative rc=$? sha256=$(sha256sum alt_l00_k5.dump | cut -d' ' -f1) temoin=$(grep 'lidar00 k5 core t1' ../determinism.txt | awk '{print $6}')" >> alt_results.txt
rm -f alt_l00_k5.dump
./build_ALT/mhgp10_tower $D/lidar01_full.u32le --k=10 --threads=3 --dump=alt_l01_k10.dump > alt_l01_k10.json
echo "lidar01 K=10 core descente alternative rc=$? sha256=$(sha256sum alt_l01_k10.dump | cut -d' ' -f1) temoin=$(grep 'lidar01 k10 core t1' ../determinism.txt | awk '{print $6}')" >> alt_results.txt
rm -f alt_l01_k10.dump
touch alt.done
