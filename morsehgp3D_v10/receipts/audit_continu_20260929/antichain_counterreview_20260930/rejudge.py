"""Independent bounded read-only counter-review: no native or shared archive writes."""
from fractions import Fraction as F
import hashlib
import importlib.util
from itertools import combinations
import json
from pathlib import Path
import sys

sys.dont_write_bytecode=True
PACKET=Path(__file__).resolve().parents[2]/'audit_independant_20260930'/'fixed_k_antichain'
MANIFEST='9ab1e991f360331a9bcfe91a84920bb1708d8370c44ac6a603fd90d161f1806e'


def require(ok,message):
    if not ok:raise ValueError(message)


def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()


def hashes():
    require(sha(PACKET/'SHA256SUMS')==MANIFEST,'fixed archive manifest')
    pins={}
    for line in (PACKET/'SHA256SUMS').read_text().splitlines():
        digest,name=line.split('  ',1)
        require(name not in pins and sha(PACKET/name)==digest,'manifest hash')
        pins[name]=digest
    require(len(pins)==37,'manifest floor')
    return pins


before=hashes()
sys.path.insert(0,str(PACKET))
spec=importlib.util.spec_from_file_location('archived_antichain_check',PACKET/'check.py')
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
abstract=c.abstract_checks()
rows,cuts=c.native_checks()
for mode in ('normal','optimized'):
    r=json.loads((PACKET/(mode+'_receipt.json')).read_text())
    require(r['status']=='PASS' and r['abstract']==abstract and
            r['native_exports']==rows and r['native_cut_checks']==cuts,'archived consumer result')
    require(r['sources_before']==r['sources_after'] and
            all(before.get(name)==digest for name,digest in r['sources_before'].items()),
            'archived source pins')
pair_checks=changed_pairs=advances=0;triangle={}
etas=(F(0),F(1,64),F(1,32),F(1,8),F(1,4),F(1),F(4))
for path in sorted((PACKET/'fixtures').glob('*.json')):
    export=c.fc.load_export(path);ctx=c.arms.ArmContext(export)
    previous={}
    for eta in etas:
        reduced=c.reduced.cover_band_antichain(ctx,eta)
        original=c.original.cover_band_lca(ctx,eta)
        a=c.fc.Attachments(ctx.forest,reduced.dates,reduced.nodes)
        b=c.fc.Attachments(ctx.forest,original.dates,original.nodes)
        advances+=sum(x<y for x,y in zip(reduced.dates,original.dates))
        for x,y in combinations(range(ctx.n),2):
            # Literal ancestor sets: no Euler or native LCA used in this oracle.
            ancestors=set();node=reduced.nodes[x]
            while node!=-1:ancestors.add(node);node=ctx.forest.parent[node]
            node=reduced.nodes[y]
            while node not in ancestors:node=ctx.forest.parent[node]
            height=max(reduced.dates[x],reduced.dates[y],ctx.forest.level(node))
            require(a.merge_height(x,y)==height,'pair height oracle')
            require(height<=b.merge_height(x,y),'original height smaller than reduction')
            if (x,y) in previous:require(previous[x,y]<=height,'eta pair height decreased')
            previous[x,y]=height;pair_checks+=1;changed_pairs+=height<b.merge_height(x,y)
    if path.stem=='triangle_default':
        target=ctx.point_id.index(1);eta=F(1,8)
        full=ctx.witnesses()
        root=ctx.forest.root
        selected=[w for w in full[target] if w[1]<=(1+eta)**2*ctx.cover_level(target)]
        require([(str(beta),v) for _,beta,v in selected]==[('16',1),('73/4',2)],'causal witness identities')
        require(ctx.forest.is_ancestor(root,1) and c.fc.qmin_exact(export.sites,export.balls[2].U,
                export.balls[2].num,export.balls[2].den)==2,'strong ancestral shell')
        old=c.original.cover_band_lca(ctx,eta);new=c.reduced.cover_band_antichain(ctx,eta)
        ablated=[list(w) for w in full];ablated[target]=[w for w in ablated[target] if w[2]!=root]
        diagnostic=c.Context(ctx.forest,[ctx.cover_level(s) for s in range(ctx.n)],ablated)
        drop=c.original.cover_band_lca(diagnostic,eta)
        require(old.dates[target]==F(73,4) and new.dates[target]==drop.dates[target]==16,
                'root witness ablation not causal')
        method=c.reduced.minimal_nodes
        c.reduced.minimal_nodes=lambda nodes,tin,tout:tuple(sorted(set(nodes),key=lambda v:tin[v]))
        try:
            mutant=c.reduced.cover_band_antichain(ctx,eta)
            require(mutant.dates[target]==F(73,4),'keep-all mutant did not restore old delay')
        finally:c.reduced.minimal_nodes=method
        triangle=dict(K=2,eta='1/8',target_point_id=1,first='16',original='73/4',reduced='16',
                      diagnostic_omitted_root_witness='16',keep_all_mutant='73/4',
                      ablation_scope='in-memory incomplete diagnostic, not an alternate complete universe')
    if path.stem=='triangle':
        reduced=c.reduced.cover_band_antichain(ctx,F(1))
        origin=ctx.point_id.index(0)
        require(len(reduced.minima[origin])==2 and reduced.dates[origin]==F(13,4),
                'real first-cover ambiguity erased')
require(hashes()==before,'archive changed')
print(json.dumps(dict(status='READ_ONLY_COUNTERREVIEW_PASS',optimize=sys.flags.optimize,manifest_entries=len(before),
                      abstract_contexts=abstract['contexts'],abstract_bands=abstract['eta_cases'],
                      native_exports=len(rows),cuts=cuts,strong_incidences=sum(row['strong_incidences'] for row in rows),
                      pair_height_checks=pair_checks,strict_pair_height_advances=changed_pairs,
                      point_entry_advances_across_contexts=advances,triangle_causality=triangle,
                      native_invocations=0,sklearn_invocations=0,GCP_used=False),sort_keys=True))


