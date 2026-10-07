#!/bin/bash
# Empreintes canoniques de la tour v10 (HEAD afb081774) sur les trames du contrat : ancres pour la v11.
cd /tmp/v11-audit/l06_code_tour/merkle
B=/tmp/v11-audit/l06_code_tour/build-release
D=/workspaces/E-HGP/build/v10-g4-data-s1
: > anchors.jsonl
for fr in lidar00 lidar01 lidar02; do
  $B/mhgp10_tower $D/${fr}_full.u32le --k=5 --threads=3 --dump=${fr}_k5.dump > ${fr}_k5.json
  echo "{\"entree\":\"${fr}_full.u32le\",\"sha256_entree\":\"$(sha256sum $D/${fr}_full.u32le | cut -d' ' -f1)\",\"K\":5,\"entree_points\":\"core\",\"sha256_dump\":\"$(sha256sum ${fr}_k5.dump | cut -d' ' -f1)\"}" >> anchors.jsonl
  PYTHONDONTWRITEBYTECODE=1 python3 tower_merkle.py ${fr}_k5.dump >> anchors.jsonl
  rm -f ${fr}_k5.dump
done
$B/mhgp10_tower $D/lidar00_full.u32le --k=10 --threads=3 --dump=lidar00_k10.dump > lidar00_k10.json
echo "{\"entree\":\"lidar00_full.u32le\",\"sha256_entree\":\"$(sha256sum $D/lidar00_full.u32le | cut -d' ' -f1)\",\"K\":10,\"entree_points\":\"core\",\"sha256_dump\":\"$(sha256sum lidar00_k10.dump | cut -d' ' -f1)\"}" >> anchors.jsonl
PYTHONDONTWRITEBYTECODE=1 python3 tower_merkle.py lidar00_k10.dump >> anchors.jsonl
rm -f lidar00_k10.dump
touch anchors.done
