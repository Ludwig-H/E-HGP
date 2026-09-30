"""Rejoue le banc z1 : une exportation native par scene, plusieurs variantes de condensation/selection."""
import csv, importlib.util, json, math, os, subprocess, sys, tempfile, time
import numpy as np
WT = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, os.path.join(WT, 'synthetic_bench_20260928'))
sys.path.insert(0, os.path.join(WT, 'tower_clustering_20260928'))
import baselines, bench_datasets as data, plan as bench_plan
import measure as M
MODS = '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L09_F1_verif/mods'
def load(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(MODS, name + '.py'))
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m); return m
C = {n: load(n) for n in ('cl_cda', 'cl_head', 'cl_wt', 'cl_splitonly', 'cl_rootonly')}
BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
out_path = sys.argv[1]; only = sys.argv[2] if len(sys.argv) > 2 else None
specs = bench_plan.specifications(seeds=bench_plan.SEEDS, heavy=False)
LIMIT = int(os.environ.get('LIMIT', '0'))
if LIMIT:
    pass
if only:
    specs = [s for s in specs if s['family'] in set(only.split(','))]
def noroot(mod):
    def sel(clusters, order):
        best, chosen = {}, {}
        for name in reversed(order):
            ch = clusters[name]['children']; below = sum(best[c] for c in ch); own = clusters[name]['stability']
            if not ch or (own >= below and clusters[name]['parent'] is not None):
                best[name], chosen[name] = own, True
            else:
                best[name], chosen[name] = below, False
        sel, st = [], [n for n in order if clusters[n]['parent'] is None]
        while st:
            n = st.pop()
            if chosen[n]: sel.append(n)
            else: st.extend(clusters[n]['children'])
        return sorted(sel)
    return sel
VARIANTS = [
    ('cda_eom', 'cl_cda', lambda m: m.select_excess_of_mass),
    ('head_eom', 'cl_head', lambda m: m.select_excess_of_mass),
    ('splitonly_eom', 'cl_splitonly', lambda m: m.select_excess_of_mass),
    ('rootonly_eom', 'cl_rootonly', lambda m: m.select_excess_of_mass),
    ('head_eom_noroot', 'cl_wt', lambda m: (lambda c, o: m.select(c, o, 'eom'))),
    ('head_leaf', 'cl_wt', lambda m: (lambda c, o: m.select(c, o, 'leaf'))),
    ('cda_eom_noroot', 'cl_cda', lambda m: noroot(m)),
]
cols = ['scene', 'family', 'n', 'level', 'seed', 'digest', 'variant', 'ari', 'clusters', 'seconds']
with tempfile.TemporaryDirectory() as keep, open(out_path, 'w', newline='') as fh:
    w = csv.DictWriter(fh, fieldnames=cols); w.writeheader()
    START = int(os.environ.get('START', '0')); END = int(os.environ.get('END', str(len(specs))))
    for i, spec in list(enumerate(specs))[START:END]:
        points, truth, meta = data.generate({k: spec[k] for k in data.SPEC_KEYS})
        grid, _ = data.quantize(points)
        t0 = time.monotonic()
        h, path = tempfile.mkstemp(suffix='.u32le', dir=keep)
        with os.fdopen(h, 'wb') as s: s.write(np.ascontiguousarray(grid, dtype='<u4').tobytes())
        done = subprocess.run([BIN, '--input', path, '--k', '2', '--workers', os.environ.get('WORKERS', '2')], capture_output=True, text=True)
        os.unlink(path)
        if done.returncode != 0:
            print(i, spec['scene'], 'REFUS', done.stderr[:100], flush=True); continue
        report = json.loads(done.stdout)
        cofaces, gabriel, size = M.read_export(report)
        births = M.facet_births(cofaces, gabriel, 'gabriel')
        base = C['cl_head']
        facets, plateaus = base.facet_levels(cofaces, gabriel)
        nodes, roots = base.merge_tree(facets, plateaus, births)
        sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, 'gabriel')
        mass = math.sqrt(len(points))
        dt = time.monotonic() - t0
        line = []
        for vname, mname, selfn in VARIANTS:
            m = C[mname]
            clusters, order = m.condense(nodes, roots, masses, births, mass, 'radius', 1)
            selected = selfn(m)(clusters, order)
            labels, ties = m.vote(len(points), m.label_facets(clusters, selected), sums, totals, len(selected))
            sc = baselines.scores(truth, np.asarray(labels))
            w.writerow(dict(scene=spec['scene'], family=spec['family'], n=spec['n'], level=spec['level'],
                            seed=spec['seed'], digest=meta['digest'], variant=vname, ari=sc['ari'],
                            clusters=sc['clusters'], seconds=round(dt, 2)))
            line.append('%s=%.3f' % (vname, sc['ari']))
        fh.flush()
        print('%3d/%d %s seed=%d %s %.1fs' % (i + 1, len(specs), spec['scene'], spec['seed'], ' '.join(line), dt), flush=True)
