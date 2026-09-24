#!/usr/bin/env bash
# Reçu local S3 (parcours et chargement par blocs du certificat), depuis la
# racine du dépôt :
#   bash morsehgp3D_v9/receipts/s3_certificate_chunks_local_20260924/run.sh <build> <out>
# <build> : build Release de morsehgp3D_v9 AU COMMIT DU REÇU
# (mhgp9_tower_probe, mhgp9_gpu_certificate_port_gate). Refus si les sources
# suivies de morsehgp3D_v9 diffèrent de HEAD. Aucun GPU, aucune commande GCP.
# Trame sans sol 08/000000 (v8, 39 885 sites), W4. Compteurs déterministes ;
# temps de mur indicatifs.
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
sha256sum "$frame" "$build/mhgp9_tower_probe" "$build/mhgp9_gpu_certificate_port_gate" > "$out/inputs.sha256"
# Chaque survivante : parcours par blocs contre build_cover (coeur et cover),
# chargement par fenêtres contre load_forms, certify_edge des deux chemins,
# prouveur produit ; compteurs avant/après de l'appel.
for k in 5 10; do
  /usr/bin/time -f 'wall_s=%e max_rss_kb=%M' -o "$out/port_k$k.time" \
    "$build/mhgp9_gpu_certificate_port_gate" --file="$frame" --k=$k --workers=4 --min-edges=2000000 \
    > "$out/port_k$k.txt"
done
# Chaîne (lots CPU, S2 + S3 + S4a + S4b) : condensés épinglés.
levers="--lever=q34_batch_filter=1 --lever=q34_batch_certificates=1 --lever=q34_batch_q3=1 --lever=q34_batch_q4=1"
for k in 5 10; do
  "$build/mhgp9_tower_probe" "$frame" "$k" 4 --grid=1mm --catalogue-digest $levers > "$out/k${k}_chain.json"
done
python3 - "$out" <<'PY'
import json, sys
out = sys.argv[1]
pins = {5: ('67450c64611075b1', '5ad1fe09354411ba', 'a2aa4b20ca392dfe'),
        10: ('ac108f7f71096c3f', 'a6e959d227f3dafa', '43ff64fb1c3846d9')}
edges = {5: 2043612, 10: 4507278}
port, chain = [], []
for k in (5, 10):
    line = open('%s/port_k%d.txt' % (out, k)).readline().split()
    if line[0] != 'certificate_port_file':
        sys.exit('receipt check failed: port K%d' % k)
    f = {key: int(value) for key, value in (item.split('=', 1) for item in line[1:]) if key != 'K'}
    if (f['survivors'] != edges[k] or f['decided'] != edges[k] or f['deferred'] != 0 or
            f['compared_walks'] != 2 * edges[k] or not 0 < f['call_chunks'] < f['call_visits'] or
            not 0 < f['load_passes_after'] < f['load_passes_before']):
        sys.exit('receipt check failed: port K%d counters' % k)
    port.append(dict(K=k, **f))
    v = json.load(open('%s/k%d_chain.json' % (out, k)))
    if v['status'] != 'complete_relative' or (v['tower_digest'], v['catalogue_digest'],
                                              v['presentation_digest']) != pins[k]:
        sys.exit('receipt check failed: chain K%d digests' % k)
    b = v['q34_batch']
    chain.append(dict(K=k, tower_digest=v['tower_digest'], catalogue_digest=v['catalogue_digest'],
                      presentation_digest=v['presentation_digest'], survivors=b['survivors'],
                      certificate_backend=b['certificate_backend'], certificate_ms=b['certificate_ms'],
                      chain_ms=v['times_ms']['chain_total']))
json.dump(dict(schema='mhgp9_s3_chunks_local_receipt_v1', port=port, chain=chain), open(out + '/SUMMARY.json', 'w'),
          indent=1)
print('receipt checks passed')
PY
(cd "$out" && sha256sum commit.txt nproc.txt inputs.sha256 port_*.txt port_*.time k*_chain.json SUMMARY.json \
  > SHA256SUMS)
