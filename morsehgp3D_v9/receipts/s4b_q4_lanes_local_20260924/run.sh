#!/usr/bin/env bash
# Reçu local S4b (voie q4 par lots sans atlas), depuis la racine du dépôt :
#   bash morsehgp3D_v9/receipts/s4b_q4_lanes_local_20260924/run.sh <build> <out>
# <build> : build Release de morsehgp3D_v9 AU COMMIT DU REÇU (mhgp9_tower_probe,
# mhgp9_gpu_lanes_port_gate). Refus si les sources suivies de morsehgp3D_v9
# diffèrent de HEAD. Aucun GPU, aucune commande GCP. Trame sans sol 08/000000
# (v8, 39 885 sites), W8. Compteurs déterministes ; temps de mur indicatifs.
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
sha256sum "$frame" "$build/mhgp9_tower_probe" "$build/mhgp9_gpu_lanes_port_gate" > "$out/inputs.sha256"
# Voies q3 et q4 de l'hôte contre celles du moteur, arête par arête : toutes
# les arêtes demandées comparées (--all-asked), planchers positifs.
floors="--all-asked --min-q3-records=1 --min-q4-seeds=1 --min-q4-records=1"
for k in 5 10; do
  /usr/bin/time -f 'wall_s=%e max_rss_kb=%M' -o "$out/compare_k$k.time" \
    "$build/mhgp9_gpu_lanes_port_gate" --file="$frame" --k=$k --workers=8 --compare $floors > "$out/compare_k$k.txt"
done
# Chaîne : S4a (q3 par lots), S4a + S4b (q4 par lots), puis S4b jugé.
levers="--lever=q34_batch_filter=1 --lever=q34_batch_certificates=1 --lever=q34_batch_q3=1"
for k in 5 10; do
  "$build/mhgp9_tower_probe" "$frame" "$k" 8 --grid=1mm --catalogue-digest $levers > "$out/k${k}_s4a.json"
  "$build/mhgp9_tower_probe" "$frame" "$k" 8 --grid=1mm --catalogue-digest $levers --lever=q34_batch_q4=1 \
    > "$out/k${k}_s4b.json"
  "$build/mhgp9_tower_probe" "$frame" "$k" 8 --grid=1mm --catalogue-digest $levers --lever=q34_batch_q4=1 \
    --lanes-judge > "$out/k${k}_s4b_judged.json"
done
# Contrôle : statuts complets, condensés de tour et de catalogue égaux aux
# épingles de l'auditeur C, condensé des présentations et comptes émis égaux
# entre les trois bras, toutes les voies demandées décidées (et jugées pour
# le bras jugé), voies q4 émises sous S4b seulement.
python3 - "$out" <<'EOF'
import json, re, sys
out = sys.argv[1]
pins = {5: ('67450c64611075b1', '5ad1fe09354411ba'), 10: ('ac108f7f71096c3f', 'a6e959d227f3dafa')}
rows, compares = [], []
for k in (5, 10):
    line = open('%s/compare_k%d.txt' % (out, k)).readline().split()
    fields = dict(item.split('=', 1) for item in line[1:])
    if fields.get('equal') != '1' or fields.get('all_asked_compared') != '1' or int(fields['q4_records']) == 0:
        sys.exit('receipt check failed: compare K%d' % k)
    compares.append(dict(K=k, **{key: int(value) for key, value in fields.items() if key != 'K'}))
    values = {arm: json.load(open('%s/k%d_%s.json' % (out, k, arm))) for arm in ('s4a', 's4b', 's4b_judged')}
    counts = set()
    for arm, v in values.items():
        b, l, g = v['q34_batch'], v['ledger'], v['generator']
        q4 = arm != 's4a'
        ok = (v['status'] == 'complete_relative' and (v['tower_digest'], v['catalogue_digest']) == pins[k] and
              v['catalogue']['euler']['status'] == 'holds' and re.fullmatch('[0-9a-f]{16}', v['presentation_digest'])
              and b['lanes_decided'] == b['lanes_asked'] > 0 and b['lanes_deferred'] == 0 and
              b['lanes_judged'] == (b['lanes_decided'] if arm == 's4b_judged' else 0) and
              (l['lanes4_emitted'] > 0) == q4)
        if not ok:
            sys.exit('receipt check failed: K%d %s' % (k, arm))
        counts.add((v['presentation_digest'], g['q2_accepted_pairs'], g['q3_emitted'], g['q4_emitted'],
                    v['catalogue']['q3_presentations'], v['catalogue']['q4_presentations']))
        rows.append(dict(K=k, arm=arm, chain_ms=v['times_ms']['chain_total'], edges_ms=b['edges_ms'],
                         lanes_ms=b['lanes_ms'], asked=b['lanes_asked'], records=b['lanes_records'],
                         q4_emitted=g['q4_emitted'], lanes4_emitted=l['lanes4_emitted'],
                         lanes4_seeds=l['lanes4_seeds'], lanes4_certified=l['lanes4_certified'],
                         lanes4_pass_chunks=l['lanes4_pass_chunks'], lanes4_list_steps=l['lanes4_list_steps'],
                         lanes4_group_steps=l['lanes4_group_steps'], tower_digest=v['tower_digest'],
                         catalogue_digest=v['catalogue_digest'], presentation_digest=v['presentation_digest']))
    if len(counts) != 1:
        sys.exit('receipt check failed: K%d presentations differ between arms' % k)
json.dump(dict(schema='mhgp9_s4b_local_receipt_v1', compare=compares, rows=rows), open(out + '/SUMMARY.json', 'w'),
          indent=1)
print('receipt checks passed')
EOF
(cd "$out" && sha256sum commit.txt nproc.txt inputs.sha256 compare_*.txt compare_*.time k*.json SUMMARY.json \
  > SHA256SUMS)
