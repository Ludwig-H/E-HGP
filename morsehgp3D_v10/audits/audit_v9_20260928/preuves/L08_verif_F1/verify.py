"""Verification adverse de L08-F1 (lecture seule du depot).

Variante `patched` = copie exacte de cluster.condense, SEULE modification :
la branche `not big` (aucun enfant au-dessus du seuil) fait tomber toutes les
facettes du noeud au niveau de la scission (HDBSCAN, sklearn _tree.pyx:204-216),
au lieu de continuer la descente jusqu'aux naissances.
Racines sous le seuil : conservees comme dans cluster.py (pas de drop).
"""
import sys, os, json, math
import numpy as np
HERE = '/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, HERE + '/tower_clustering_20260928')
sys.path.insert(0, HERE + '/synthetic_bench_20260928')
import cluster as C, measure as M, run_tower as R
import baselines, bench_datasets as data, plan as bench_plan
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari

BIN = '/workspaces/E-HGP/build/v9-weighted-native-20260927-r1/native_weighted_export'
OUT = os.path.dirname(os.path.abspath(__file__)) + '/exports'
os.makedirs(OUT, exist_ok=True)


def condense_patched(nodes, roots, masses, births, min_cluster_mass, mode, z, scale='lambda'):
    mass = {}
    pending = list(roots)
    while pending:
        item = pending[-1]
        if item in mass:
            pending.pop(); continue
        if item not in nodes:
            mass[item] = float(masses.get(item, 0.0)); pending.pop(); continue
        missing = [c for c in nodes[item]['children'] if c not in mass]
        if missing:
            pending.extend(missing); continue
        mass[item] = sum(mass[c] for c in nodes[item]['children']); pending.pop()
    clusters, order = {}, []

    def root_birth(root):
        level = nodes[root]['level'] if root in nodes else births[root]
        return C._lam(level, mode, z)
    queue = [(root, None, root_birth(root)) for root in roots]
    while queue:
        item, parent, birth_lambda = queue.pop()
        name = 'c%d' % len(order); order.append(name)
        clusters[name] = dict(parent=parent, birth=birth_lambda, falls=[], children=[], node=item)
        if parent is not None:
            clusters[parent]['children'].append(name)
        stack = [item]
        while stack:
            current = stack.pop()
            if current not in nodes:
                clusters[name]['falls'].append((current, C._lam(births[current], mode, z))); continue
            level = C._lam(nodes[current]['level'], mode, z)
            big = [c for c in nodes[current]['children'] if mass[c] >= min_cluster_mass]
            for child in nodes[current]['children']:
                if len(big) >= 2 and child in big:
                    for f in (nodes[child]['members'] if child in nodes else {child}):
                        clusters[name]['falls'].append((f, level))
                    queue.append((child, name, level))
                elif child in big:          # <-- seule difference : 'or not big' retire
                    stack.append(child)
                else:
                    for f in (nodes[child]['members'] if child in nodes else {child}):
                        clusters[name]['falls'].append((f, level))
    for name, cl in clusters.items():
        b = cl['birth']
        if scale == 'lambda':
            span = lambda lam: max(0.0, lam - b)
        else:
            span = lambda lam: (math.log(lam / b) if b > 0.0 and lam > b else 0.0)
        cl['stability'] = sum(float(masses.get(f, 0.0)) * span(l) for f, l in cl['falls'])
        cl['mass'] = sum(float(masses.get(f, 0.0)) for f, l in cl['falls'])
    return clusters, order


def count_nobig(nodes, roots, masses, mcm):
    """Nombre de clusters condenses dont la mort est une scission en petits morceaux,
    et masse de facettes qui tombent a leur naissance au lieu du niveau de scission."""
    mass = {}
    def tot(x):
        if x in mass: return mass[x]
        st = [x]
        while st:
            it = st[-1]
            if it in mass: st.pop(); continue
            if it not in nodes: mass[it] = float(masses.get(it, 0.0)); st.pop(); continue
            miss = [c for c in nodes[it]['children'] if c not in mass]
            if miss: st.extend(miss); continue
            mass[it] = sum(mass[c] for c in nodes[it]['children']); st.pop()
        return mass[x]
    for r in roots: tot(r)
    return mass


def run(family, n, k, conv, seed=0, z=1, level='medium'):
    spec = dict(family=family, n=n, groups=8, level=level, noise_fraction=0.0,
                seed=bench_plan.BASE_SEED + seed)
    points, truth, meta = data.generate(spec)
    grid, scale = data.quantize(points)
    tag = '%s_%s_%d_k%d_s%d' % (family, level, n, k, seed)
    path = os.path.join(OUT, tag + '.json')
    if os.path.exists(path):
        report = json.load(open(path))
    else:
        report = R.export(BIN, grid, k, 2, OUT)
        json.dump(report, open(path, 'w'))
    cofaces, gabriel, size = M.read_export(report)
    births = M.facet_births(cofaces, gabriel, conv)
    keep = None if conv == 'boundary' else gabriel
    facets, plateaus = C.facet_levels(cofaces, keep)
    nodes, roots = C.merge_tree(facets, plateaus, births)
    sums, totals, masses, covered = M.measure(cofaces, gabriel, z, conv)
    mcm = math.sqrt(n)
    res = dict(family=family, level=level, n=n, k=k, conv=conv, seed=seed, roots=len(roots))
    ref = {}
    if k == 1:
        X = np.asarray(grid, dtype=np.float64)
        ref['eom'] = HDBSCAN(min_cluster_size=math.ceil(mcm), min_samples=1,
                             cluster_selection_method='eom').fit(X).labels_
        ref['leaf'] = HDBSCAN(min_cluster_size=math.ceil(mcm), min_samples=1,
                              cluster_selection_method='leaf').fit(X).labels_
        res['hdb_ari_truth'] = {m: round(ari(truth, ref[m]), 4) for m in ref}
    for variant, fn in (('as_is', C.condense), ('patched', condense_patched)):
        for scale in ('lambda', 'log'):
            clusters, order = fn(nodes, roots, masses, births, mcm, 'radius', z, scale)
            for method in ('eom', 'leaf'):
                sel = C.select(clusters, order, method)
                lab, ties = C.vote(n, C.label_facets(clusters, sel), sums, totals, len(sel))
                lab = np.asarray(lab)
                sc = baselines.scores(truth, lab)
                d = dict(ari=round(sc['ari'], 4), clusters=int(sc['clusters']))
                if k == 1 and scale == 'lambda':
                    d['ari_vs_hdb'] = round(ari(ref[method], lab), 4)
                    d['identical_partition'] = bool(ari(ref[method], lab) == 1.0 and ((lab < 0) == (ref[method] < 0)).all())
                res['%s_%s_%s' % (variant, scale, method)] = d
    return res


if __name__ == '__main__':
    family, n, k, conv = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    level = sys.argv[5] if len(sys.argv) > 5 else 'medium'
    seed = int(sys.argv[6]) if len(sys.argv) > 6 else 0
    print(json.dumps(run(family, n, k, conv, seed=seed, level=level)), flush=True)
