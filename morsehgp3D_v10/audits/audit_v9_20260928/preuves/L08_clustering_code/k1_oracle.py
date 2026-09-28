"""Oracle differentiel a K=1 : la chaine v9 doit redonner HDBSCAN(min_samples=1) sur l'EMST.

A K=1, convention du bord : facettes = points, cofaces = aretes de Gabriel (EMST inclus),
m_x = 1 pour tout point, vote trivial. min_cluster_mass = sqrt(n) <=> min_cluster_size = ceil(sqrt(n)).
lambda v9 = 1/r = 2/d, lambda HDBSCAN = 1/d : facteur constant, meme selection EOM.
"""
import sys, os, json, math
import numpy as np
sys.path.insert(0, '/tmp/claude-1000/-workspaces-E-HGP/497c67a8-5e59-434d-a8a7-2dc2144d8af4/scratchpad/audit_v9/L08_clustering_code')
import probe as P
from sklearn.cluster import HDBSCAN
from sklearn.metrics import adjusted_rand_score as ari
C, M = P.C, P.M
fam = sys.argv[1]; n = int(sys.argv[2]); seed = int(sys.argv[3]) if len(sys.argv) > 3 else 0
spec = dict(family=fam, n=n, groups=8, level='medium', noise_fraction=0.0, seed=P.bench_plan.BASE_SEED + seed)
points, truth, meta = P.data.generate(spec)
grid, scale = P.data.quantize(points)
report = P.R.export(P.BIN, grid, 1, 2, P.OUT)
cofaces, gabriel, size = M.read_export(report)
conv = 'boundary'
births = M.facet_births(cofaces, gabriel, conv)
facets, plateaus = C.facet_levels(cofaces, None)
nodes, roots = C.merge_tree(facets, plateaus, births)
sums, totals, masses, covered = M.measure(cofaces, gabriel, 1, conv)
mcm = math.sqrt(n)
res = dict(scene=fam, n=n, seed=seed, roots=len(roots), native_roots=len(report['native']['roots']),
           max_mass=float(max(masses.values())), min_mass=float(min(masses.values())))
X = np.asarray(grid, dtype=np.float64)
hd = HDBSCAN(min_cluster_size=math.ceil(mcm), min_samples=1, cluster_selection_method='eom', allow_single_cluster=False).fit(X).labels_
hdl = HDBSCAN(min_cluster_size=math.ceil(mcm), min_samples=1, cluster_selection_method='leaf').fit(X).labels_
for variant in ('as_is', 'conform'):
    if variant == 'as_is':
        clusters, order = C.condense(nodes, roots, masses, births, mcm, 'radius', 1)
    else:
        clusters, order = P.condense_hdbscan(nodes, roots, masses, births, mcm, 'radius', 1)
    for method, ref in (('eom', hd), ('leaf', hdl)):
        sel = C.select(clusters, order, method)
        lab, ties = C.vote(n, C.label_facets(clusters, sel), sums, totals, len(sel))
        lab = np.asarray(lab)
        res['%s_%s' % (variant, method)] = dict(ari_vs_hdbscan=round(ari(ref, lab), 4), identical=bool((ari(ref, lab) == 1.0) and ((lab < 0) == (ref < 0)).all()),
            clusters=int(len(set(lab.tolist()) - {-1})), hdb_clusters=int(len(set(ref.tolist()) - {-1})),
            noise=int((lab < 0).sum()), hdb_noise=int((ref < 0).sum()), ari_truth=round(ari(truth, lab), 4), hdb_ari_truth=round(ari(truth, ref), 4))
print(json.dumps(res))
