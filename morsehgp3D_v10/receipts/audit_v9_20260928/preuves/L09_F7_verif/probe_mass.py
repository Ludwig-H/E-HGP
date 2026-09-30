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
        m = importlib.util.module_from_spec(spec)
        sys.modules[name] = m if name == 'measure' else sys.modules.get(name, m)
        spec.loader.exec_module(m)
        mods[name] = m
    return mods['measure'], mods['cluster']

versions = {t: load(t) for t in ('cda', 'ce8', 'wt')}
fam = sys.argv[2] if len(sys.argv) > 2 else 'hierarchical'
print('family', fam, 'medium n=2000 g8 K=2 z=1 lambda scale, EOM')
print('%-12s %-10s %-10s %-10s %-10s %-10s %-10s %-10s' % ('seed', 'cda_sqrt', 'cda_100', 'ce8_sqrt', 'ce8_100', 'wt_eom_sq', 'wt_leaf_sq', 'wt_eom_100'))
for r in range(5):
    seed = 2026092800 + r
    pts, truth, _ = bd.generate(dict(family=fam, n=2000, groups=8, level='medium', seed=seed, noise_fraction=0.0))
    grid, _ = bd.quantize(pts)
    h, path = tempfile.mkstemp(suffix='.u32le', dir=S); os.close(h)
    open(path, 'wb').write(np.ascontiguousarray(grid, dtype='<u4').tobytes())
    rep = json.loads(subprocess.run(['nice', '-n', '19', D, '--input', path, '--k', '2', '--workers', '2'], capture_output=True, text=True).stdout)
    os.unlink(path)
    out = []
    for tag in ('cda', 'ce8', 'wt'):
        M, C = versions[tag]
        cof, gab, size = M.read_export(rep); births = M.facet_births(cof, gab, 'gabriel')
        facets, plateaus = C.facet_levels(cof, gab); nodes, roots = C.merge_tree(facets, plateaus, births)
        sums, totals, masses, covered = M.measure(cof, gab, 1, 'gabriel')
        if tag == 'wt':
            combos = [(math.sqrt(2000), 'eom'), (math.sqrt(2000), 'leaf'), (100.0, 'eom')]
        else:
            combos = [(math.sqrt(2000), 'eom'), (100.0, 'eom')]
        for mass, meth in combos:
            if tag == 'cda':
                cl, order = C.condense(nodes, roots, masses, births, mass, 'radius', 1)
                sel = C.select_excess_of_mass(cl, order)
            elif tag == 'ce8':
                cl, order = C.condense(nodes, roots, masses, births, mass, 'radius', 1, 'lambda')
                sel = C.select_excess_of_mass(cl, order)
            else:
                cl, order = C.condense(nodes, roots, masses, births, mass, 'radius', 1, 'lambda')
                sel = C.select(cl, order, meth)
            labels, _ = C.vote(len(pts), C.label_facets(cl, sel), sums, totals, len(sel))
            out.append('%.3f(%d)' % (ari(truth, labels), len(sel)))
    print('%-12d %s' % (seed, ' '.join('%-10s' % o for o in out)), flush=True)
