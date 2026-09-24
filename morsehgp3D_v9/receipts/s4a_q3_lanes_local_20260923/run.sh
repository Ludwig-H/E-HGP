#!/usr/bin/env bash
# Reçu local S4a (voie q3 par lots sans atlas), depuis la racine du dépôt :
#   bash morsehgp3D_v9/receipts/s4a_q3_lanes_local_20260923/run.sh <build> <out>
# <build> : build Release de morsehgp3D_v9 AU COMMIT DU REÇU (mhgp9_tower_probe,
# mhgp9_gpu_lanes_port_gate, libmhgp9_gen.a). Refus si les sources suivies de
# morsehgp3D_v9 diffèrent de HEAD. Aucun GPU, aucune commande GCP. Trame sans
# sol 08/000000 (v8, 39 885 sites), W8. Les compteurs sont déterministes ; les
# temps de mur sont indicatifs (hôte partagé).
set -euo pipefail
build=$1
out=$2
frame=morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le
git diff --quiet HEAD -- morsehgp3D_v9/src morsehgp3D_v9/bench morsehgp3D_v9/tests morsehgp3D_v9/CMakeLists.txt \
  morsehgp3D_v9/cmake || { echo "sources differ from HEAD: refused" >&2; exit 2; }
[ ! -e "$out" ] || { echo "output exists: refused" >&2; exit 2; }
mkdir -p "$out"
git rev-parse HEAD > "$out/commit.txt"
nproc > "$out/nproc.txt"
sha256sum "$frame" "$build/mhgp9_tower_probe" "$build/mhgp9_gpu_lanes_port_gate" "$build/libmhgp9_gen.a" \
  > "$out/inputs.sha256"
# Statistiques de travail par arête : anneaux par défaut (8) à K5 et K10,
# ordre de rang (1 anneau, binaire de mesure) à K5.
"$build/mhgp9_gpu_lanes_port_gate" --file="$frame" --k=5 --workers=8 > "$out/stats_k5_rings8.txt"
"$build/mhgp9_gpu_lanes_port_gate" --file="$frame" --k=10 --workers=8 > "$out/stats_k10_rings8.txt"
g++ -std=c++20 -O2 -DNDEBUG -DMHGP9_LANES_SCAN_RINGS=1 -I morsehgp3D_v9/src/gen -I morsehgp3D_v9/tests \
  morsehgp3D_v9/tests/gpu/lanes_port_gate.cpp "$build/libmhgp9_gen.a" -pthread -o "$out/rank_order_stats"
sha256sum "$out/rank_order_stats" | sed "s|$out/||" >> "$out/inputs.sha256"
"$out/rank_order_stats" --file="$frame" --k=5 --workers=8 > "$out/stats_k5_rings1.txt"
rm -f "$out/rank_order_stats"
# Chaîne : S3 CPU seul, puis S3 + voie q3 CPU (sans juge), puis jugée.
levers="--lever=q34_batch_filter=1 --lever=q34_batch_certificates=1"
for k in 5 10; do
  "$build/mhgp9_tower_probe" "$frame" "$k" 8 --grid=1mm --catalogue-digest $levers > "$out/k${k}_s3.json"
  "$build/mhgp9_tower_probe" "$frame" "$k" 8 --grid=1mm --catalogue-digest $levers --lever=q34_batch_q3=1 \
    > "$out/k${k}_q3.json"
  "$build/mhgp9_tower_probe" "$frame" "$k" 8 --grid=1mm --catalogue-digest $levers --lever=q34_batch_q3=1 \
    --lanes-judge > "$out/k${k}_q3_judged.json"
done
# Contrôle : statuts complets, condensés égaux entre les trois bras et aux
# épingles de l'auditeur C (tower_worker_v9.PINNED_DIGESTS), toutes les voies
# demandées décidées (et jugées pour le bras jugé).
python3 - "$out" <<'EOF'
import json, sys
out = sys.argv[1]
pins = {5: ('67450c64611075b1', '5ad1fe09354411ba'), 10: ('ac108f7f71096c3f', 'a6e959d227f3dafa')}
rows = []
for k in (5, 10):
    values = {arm: json.load(open('%s/k%d_%s.json' % (out, k, arm))) for arm in ('s3', 'q3', 'q3_judged')}
    for arm, v in values.items():
        b = v['q34_batch']
        ok = (v['status'] == 'complete_relative' and (v['tower_digest'], v['catalogue_digest']) == pins[k] and
              v['catalogue']['euler']['status'] == 'holds' and
              (arm == 's3' or (b['lanes_decided'] == b['lanes_asked'] > 0 and
                               b['lanes_judged'] == (b['lanes_decided'] if arm == 'q3_judged' else 0))))
        if not ok:
            sys.exit('receipt check failed: K%d %s' % (k, arm))
        rows.append(dict(K=k, arm=arm, chain_ms=v['times_ms']['chain_total'], edges_ms=b['edges_ms'],
                         lanes_ms=b['lanes_ms'], asked=b['lanes_asked'], records=b['lanes_records'],
                         census_point_tests=v['ledger']['lanes_census_point_tests'],
                         tower_digest=v['tower_digest'], catalogue_digest=v['catalogue_digest']))
json.dump(dict(schema='mhgp9_s4a_local_receipt_v1', rows=rows), open(out + '/SUMMARY.json', 'w'), indent=1)
print('receipt checks passed')
EOF
(cd "$out" && sha256sum commit.txt nproc.txt inputs.sha256 stats_*.txt k*.json SUMMARY.json > SHA256SUMS)
