import json, math, os, subprocess, sys, tempfile, importlib.util
import numpy as np
S = sys.argv[1]
W = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, W + '/synthetic_bench_20260928')
import bench_datasets as bd
from sklearn.metrics import adjusted_rand_score as ari
D = '/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad/nwe2/native_weighted_export'
def load(tag):
    mods = {}
    for name in ('measure', 'cluster'):
        spec = importlib.util.spec_from_file_location(name + '_' + tag, '%s/%s/%s.py' % (S, tag, name))
        m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m); mods[name] = m
    return mods['measure'], mods['cluster']
M, C = load('ce8')
for fam in sys.argv[2:]:
    row = []
    for r in range(5):
        seed = 2026092800 + r
        pts, truth, _ = bd.generate(dict(family=fam, n=2000, groups=8, level='medium', seed=seed, noise_fraction=0.0))
        grid, _ = bd.quantize(pts)
        h, path = tempfile.mkstemp(suffix='.u32le', dir=S); os.close(h)
        open(path, 'wb').write(np.ascontiguousarray(grid, dtype='<u4').tobytes())
        rep = json.loads(subprocess.run(['nice', '-n', '19', D, '--input', path, '--k', '2', '--workers', '2'], capture_output=True, text=True).stdout)
        os.unlink(path)
        cof, gab, size = M.read_export(rep); births = M.facet_births(cof, gab, 'gabriel')
        facets, plateaus = C.facet_levels(cof, gab); nodes, roots = C.merge_tree(facets, plateaus, births)
        sums, totals, masses, covered = M.measure(cof, gab, 1, 'gabriel')
        res = []
        for scale in ('lambda', 'log'):
            cl, order = C.condense(nodes, roots, masses, births, math.sqrt(2000), 'radius', 1, scale)
            sel = C.select_excess_of_mass(cl, order)
            labels, _ = C.vote(len(pts), C.label_facets(cl, sel), sums, totals, len(sel))
            res.append(ari(truth, labels))
        row.append(res)
    lam = [x[0] for x in row]; lg = [x[1] for x in row]
    print('%-16s lambda %s mean=%.3f | log %s mean=%.3f' % (fam, ' '.join('%.3f' % v for v in lam), sum(lam)/5, ' '.join('%.3f' % v for v in lg), sum(lg)/5), flush=True)
