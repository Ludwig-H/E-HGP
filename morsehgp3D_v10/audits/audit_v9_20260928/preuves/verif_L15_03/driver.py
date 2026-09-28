import csv, sys, os, types, tempfile
root = sys.argv[1]  # pre|post dir
sys.path.insert(0, os.path.join(root, 'synthetic_bench_20260928'))
sys.path.insert(0, os.path.join(root, 'tower_clustering_20260928'))
import run_tower as R
import plan as P
smoke = {(r['scene'], r['seed']) for r in csv.DictReader(open(sys.argv[2]))}
specs = [s for s in P.specifications(seeds=P.SEEDS, heavy=True) if (s['scene'], str(s['seed'])) in smoke]
print('specs', len(specs), 'smoke', len(smoke), flush=True)
args = types.SimpleNamespace(export=sys.argv[3], k=2, z=1, convention='gabriel', lambda_mode='radius', min_cluster_mass=0.0, workers=2)
out = open(sys.argv[4], 'w', newline='')
w = csv.DictWriter(out, fieldnames=list(R.COLUMNS) + ['roots', 'nodes', 'selected'])
w.writeheader()
with tempfile.TemporaryDirectory() as keep:
    for s in specs:
        row, detail = R.run_one(s, args, keep)
        row = {k: row[k] for k in R.COLUMNS}
        row.update(roots=detail['roots'], nodes=detail['nodes'], selected=detail['selected'])
        w.writerow(row); out.flush()
        print(s['scene'], s['seed'], round(row['ari'], 4), 'roots', detail['roots'], flush=True)
