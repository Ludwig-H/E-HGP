import sys, subprocess, importlib.util
from fractions import Fraction as F
W='/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/experiments'
sys.path.insert(0, W+'/weighted_clustering_20260927')
from full_attachment_oracle import E5, build_reference, tree_cut, reference_cut
# load committed (HEAD) cluster.py to avoid uncommitted diff
src = subprocess.run(['git','-C','/workspaces/E-HGP/build/v9-open-worktree','show','HEAD:morsehgp3D_v9/experiments/tower_clustering_20260928/cluster.py'],capture_output=True,text=True).stdout
spec = importlib.util.spec_from_loader('cluster_head', loader=None); C = importlib.util.module_from_spec(spec); exec(src, C.__dict__)
ref = build_reference(E5, 2)
cof = sorted(ref['gabriel_cofaces'].items(), key=lambda kv: kv[1])
print('gabriel cofaces', [(v, str(b)) for v,b in cof])
print('FULL leaf births', {f: str(b) for f,b in zip(ref['facets'], ref['leaf_birth_betas'])})
for conv in ('boundary','gabriel'):
    keep = None
    if conv=='gabriel':
        keep = set(ref['facets'])  # facets of the reference (FULL) catalogue
    facets, plateaus = C.facet_levels(cof, keep)
    births = {f: F(0) for f in facets}
    nodes, roots = C.merge_tree(facets, plateaus, births)
    print(conv, 'facets', sorted(facets), 'roots', len(roots))
    for name, nd in sorted(nodes.items(), key=lambda kv: kv[1]['level']):
        print('  node', name, 'level', nd['level'], float(nd['level']), 'members', sorted(nd['members']))
for beta in (F(33,2), F(83886,3563), F(24)):
    print('FULL tree_cut closed at', beta, float(beta), tree_cut(ref, beta))
