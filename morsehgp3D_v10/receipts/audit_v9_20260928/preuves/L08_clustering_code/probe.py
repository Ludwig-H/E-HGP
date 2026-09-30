"""Sonde de lecture : defauts de cluster.py mesures sur des scenes du banc (aucune ecriture au depot)."""
import sys, os, json, math, time, collections, argparse
import numpy as np
HERE = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, HERE + '/tower_clustering_20260928')
sys.path.insert(0, HERE + '/synthetic_bench_20260928')
import cluster as C, measure as M, run_tower as R
import baselines, bench_datasets as data, plan as bench_plan
BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
SCALES = tuple(os.environ.get('SCALES', 'lambda').split(','))
OUT = '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code/exports'
os.makedirs(OUT, exist_ok=True)

def reachable(nodes, roots):
    seen, stack = set(), list(roots)
    while stack:
        cur = stack.pop()
        if cur in seen: continue
        seen.add(cur)
        if cur in nodes: stack.extend(nodes[cur]['children'])
    return seen

def condense_hdbscan(nodes, roots, masses, births, mcm, mode, z, scale='lambda', drop_small_roots=True):
    """Variante conforme a HDBSCAN/HGP-old : scission en petits enfants seulement => tout tombe au niveau;
    racine sous le seuil => pas de cluster (bruit)."""
    mass = {}
    pending = list(roots)
    while pending:
        item = pending[-1]
        if item in mass: pending.pop(); continue
        if item not in nodes: mass[item] = float(masses.get(item, 0.0)); pending.pop(); continue
        missing = [c for c in nodes[item]['children'] if c not in mass]
        if missing: pending.extend(missing); continue
        mass[item] = sum(mass[c] for c in nodes[item]['children']); pending.pop()
    def members(x):
        return nodes[x]['members'] if x in nodes else {x}
    clusters, order = {}, []
    queue = []
    for r in roots:
        if drop_small_roots and mass[r] < mcm: continue
        lvl = nodes[r]['level'] if r in nodes else births[r]
        queue.append((r, None, C._lam(lvl, mode, z)))
    while queue:
        item, parent, bl = queue.pop()
        name = 'c%d' % len(order); order.append(name)
        clusters[name] = dict(parent=parent, birth=bl, falls=[], children=[], node=item)
        if parent is not None: clusters[parent]['children'].append(name)
        stack = [item]
        while stack:
            cur = stack.pop()
            if cur not in nodes:
                clusters[name]['falls'].append((cur, C._lam(births[cur], mode, z))); continue
            level = C._lam(nodes[cur]['level'], mode, z)
            big = [c for c in nodes[cur]['children'] if mass[c] >= mcm]
            if len(big) >= 2:
                for c in nodes[cur]['children']:
                    for f in members(c): clusters[name]['falls'].append((f, level))
                    if c in big: queue.append((c, name, level))
            elif len(big) == 1:
                for c in nodes[cur]['children']:
                    if c in big: stack.append(c)
                    else:
                        for f in members(c): clusters[name]['falls'].append((f, level))
            else:
                for c in nodes[cur]['children']:
                    for f in members(c): clusters[name]['falls'].append((f, level))
    for name, cl in clusters.items():
        b = cl['birth']
        if scale == 'lambda': span = lambda lam: max(0.0, lam - b)
        else: span = lambda lam: (math.log(lam / b) if b > 0 and lam > b else 0.0)
        cl['stability'] = sum(float(masses.get(f, 0.0)) * span(l) for f, l in cl['falls'])
        cl['mass'] = sum(float(masses.get(f, 0.0)) for f, l in cl['falls'])
    return clusters, order

def run(spec, k, z, conv, workers=2, cache=True):
    points, truth, meta = data.generate({key: spec[key] for key in data.SPEC_KEYS})
    grid, scale = data.quantize(points)
    tag = '%s_%s_%d_%s_%s_s%d_k%d' % (spec['family'], spec['level'], spec['n'], spec['groups'], spec['noise_fraction'], spec['seed'], k)
    path = os.path.join(OUT, tag + '.json')
    t0 = time.monotonic()
    if cache and os.path.exists(path):
        report = json.load(open(path))
    else:
        report = R.export(BIN, grid, k, workers, OUT)
        json.dump(report, open(path, 'w'))
    t_export = time.monotonic() - t0
    n = len(points)
    mcm = math.sqrt(n)
    cofaces, gabriel, size = M.read_export(report)
    births = M.facet_births(cofaces, gabriel, conv)
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
    t1 = time.monotonic()
    nodes, roots = C.merge_tree(facets, plateaus, births)
    t_tree = time.monotonic() - t1
    reach = reachable(nodes, roots)
    lost = facets - reach
    multi = sum(1 for b, g in plateaus if len(g) > 1)
    sums, totals, masses, covered = M.measure(cofaces, gabriel, z, conv)
    members_total = sum(len(v['members']) for v in nodes.values())
    rootmass = {}
    for r in roots:
        rootmass[r] = sum(float(masses[f]) for f in (nodes[r]['members'] if r in nodes else {r}))
    small_roots = [r for r in roots if rootmass[r] < mcm]
    res = dict(tag=tag, n=n, conv=conv, k=k, z=z, cofaces=len(cofaces), facets=len(facets), nodes=len(nodes),
               roots=len(roots), small_roots=len(small_roots), small_root_mass=round(sum(rootmass[r] for r in small_roots), 3),
               multi_plateaus=multi, lost_facets=len(lost), members_total=members_total,
               t_export=round(t_export, 2), t_tree=round(t_tree, 2))
    t2 = time.monotonic()
    for variant in ('as_is', 'fall_only', 'hdbscan_conform'):
        for scale in SCALES:
            if variant == 'as_is':
                clusters, order = C.condense(nodes, roots, masses, births, mcm, 'radius', z, scale)
            elif variant == 'fall_only':
                clusters, order = condense_hdbscan(nodes, roots, masses, births, mcm, 'radius', z, scale, drop_small_roots=False)
            else:
                clusters, order = condense_hdbscan(nodes, roots, masses, births, mcm, 'radius', z, scale)
            for method in ('eom', 'leaf'):
                sel = C.select(clusters, order, method)
                lab, ties = C.vote(n, C.label_facets(clusters, sel), sums, totals, len(sel))
                sc = baselines.scores(truth, np.asarray(lab))
                tiny = sum(1 for s in sel if clusters[s]['parent'] is None and clusters[s]['mass'] < mcm)
                lab = np.asarray(lab)
                tiny_pts = int(sum((lab == i).sum() for i, s in enumerate(sel) if clusters[s]['parent'] is None and clusters[s]['mass'] < mcm))
                res['%s_%s_%s' % (variant, scale, method)] = dict(ari=round(sc['ari'], 4), clusters=sc['clusters'],
                    cov=round(sc['coverage'], 3), tiny_root_clusters=tiny, tiny_root_points=tiny_pts)
    res['t_condense_all'] = round(time.monotonic() - t2, 2)
    return res

if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--family', default='spherical'); ap.add_argument('--level', default='medium')
    ap.add_argument('--n', type=int, default=2000); ap.add_argument('--groups', type=int, default=8)
    ap.add_argument('--noise', type=float, default=0.0); ap.add_argument('--seed', type=int, default=0)
    ap.add_argument('--k', type=int, default=2); ap.add_argument('--z', type=int, default=1)
    ap.add_argument('--conv', default='gabriel')
    a = ap.parse_args()
    spec = dict(family=a.family, n=a.n, groups=a.groups, level=a.level, noise_fraction=a.noise, seed=bench_plan.BASE_SEED + a.seed)
    print(json.dumps(run(spec, a.k, a.z, a.conv)))
