"""One geometric K2 case only: exhaustive Fraction Gamma, not a native export."""
from fractions import Fraction as F
import hashlib
import importlib.util
from itertools import combinations
import json
from pathlib import Path
import sys
sys.dont_write_bytecode=True
PACKET=Path(__file__).resolve().parents[2]/'audit_independant_20260930'/'fixed_k_antichain'
REFERENCE=Path(__file__).resolve().parent.parent/'point_condensation_cover_r2_20260930'/'reference'/'hgp10_ref.py'


def require(ok,message):
    if not ok:raise ValueError(message)


def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()


require(sha(PACKET/'SHA256SUMS')=='9ab1e991f360331a9bcfe91a84920bb1708d8370c44ac6a603fd90d161f1806e','fixed archive')
pins={}
for line in (PACKET/'SHA256SUMS').read_text().splitlines():
    digest,name=line.split('  ',1);require(sha(PACKET/name)==digest,'hash');pins[name]=digest
require(sha(REFERENCE)=='2cb84ad549b1f8982e71f7757794d97b5eadab323fd22d17461da18b76914104','MEB pin')
sys.path.insert(0,str(PACKET))
def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
c=load('frozen_antichain_geometry_context',PACKET/'check.py')
R=load('independent_fraction_meb',REFERENCE)
P=[(0,0,0),(10,0,0),(8,4,0),(4,8,0)]
K=2
ETA=F(1,8)
vertices={v:R.meb(P,v)[0] for v in combinations(range(4),K)}
edges={v:R.meb(P,v)[0] for v in combinations(range(4),K+1)}
levels=sorted(set(vertices.values())|set(edges.values()))
birth=[];parent=[];old={};snapshots={}
for beta in levels:
    active={v:v for v,t in vertices.items() if t<=beta}
    def find(v):
        while active[v]!=v:v=active[v]
        return v
    for e,t in edges.items():
        if t>beta:continue
        faces=list(combinations(e,K));anchor=find(faces[0])
        for face in faces[1:]:active[find(face)]=anchor
    groups={}
    for v in active:groups.setdefault(find(v),set()).add(v)
    new={}
    for component in sorted(groups.values(),key=lambda group:min(group)):
        predecessors=sorted({old[v] for v in component if v in old})
        if len(predecessors)==1:
            node=predecessors[0]
        else:
            node=len(birth);birth.append(beta);parent.append(-1)
            for prior in predecessors:
                require(parent[prior]==-1,'not currently alive')
                parent[prior]=node
        for v in component:new[v]=node
    old=new;snapshots[beta]=new
f=c.Tree(birth,parent)
# Exact critical catalogue, all positive supports and complete I/U, no native filters.
balls=R.critical_balls(P)
strong=sorted((b for b in balls if len(b.I)+len(b.U)>=K and b.p+b.qmin<=K),
              key=lambda b:(b.level,b.center))
alpha=[min(t for v,t in vertices.items() if x in v) for x in range(4)]
witnesses=[[] for _ in P];ball_rows=[]
for index,b in enumerate(strong):
    members=tuple(sorted(b.I+b.U))
    node=snapshots[b.level][tuple(members[:K])]
    require(all(snapshots[b.level][v]==node for v in combinations(members,K)),'sphere members span components')
    require(f.ancestor(node,b.level,True)==node,'witness not alive')
    for x in members:witnesses[x].append((index,b.level,node))
    ball_rows.append(dict(beta=str(b.level),center=list(map(str,b.center)),interior=list(b.I),
                          shell=list(b.U),qmin=b.qmin,node=node))
ctx=c.Context(f,alpha,witnesses)
old=c.original.cover_band_lca(ctx,ETA)
new=c.reduced.cover_band_antichain(ctx,ETA)
a=c.fc.Attachments(f,new.dates,new.nodes)
b=c.fc.Attachments(f,old.dates,old.nodes)
# Tree is derived above; pair oracle uses Gamma components directly, not its LCA.
beta=F(200,9)
require(edges[(0,2,3)]==beta and edges[(0,1,2)]==25 and beta<25,'geometric MEB witness')
require(new.dates[0]==beta and old.dates[0]==25,'entry difference absent')
require(new.dates[3]==old.dates[3]==8,'partner delayed elsewhere')
require(a.merge_height(0,3)==beta and b.merge_height(0,3)==25,'pair height difference absent')
cuts=sorted({F(0),beta,25}|set(levels)|{(x+y)/2 for x,y in zip(levels,levels[1:])})
partition_differences=[]
def blocks(value,cut):
    groups={}
    for x,(date,node) in enumerate(zip(value.dates,value.nodes)):
        label=('s',x) if cut<date else ('c',f.ancestor(node,cut,True))
        groups.setdefault(label,[]).append(x)
    return sorted(groups.values())
for cut in cuts:
    original,reduced=blocks(old,cut),blocks(new,cut)
    require(c.fc.nested(original,reduced),'refinement fails')
    if original!=reduced:
        partition_differences.append(dict(beta=str(cut),original=original,reduced=reduced))
require(partition_differences,'no partition difference')
require(snapshots[beta][(0,3)]==snapshots[beta][(0,2)] and
        snapshots[beta][(0,3)]!=snapshots[beta][(1,2)],'independent Gamma event mismatch')
for name,digest in pins.items():require(sha(PACKET/name)==digest,'archive changed')
require(sha(REFERENCE)=='2cb84ad549b1f8982e71f7757794d97b5eadab323fd22d17461da18b76914104','MEB changed')
require(blocks(old,beta)==[[0],[1,2],[3]] and blocks(new,beta)==[[0,3],[1,2]],'expected point partition')
require(alpha==[20,5,5,8] and (1+ETA)**2*alpha[0]==F(405,16),'default squared-radius threshold')
print(json.dumps(dict(status='EXACT_GEOMETRIC_PAIR_COUNTEREXAMPLE',optimize=sys.flags.optimize,K=2,eta=str(ETA),
                     points=P,vertices={str(v):str(t) for v,t in vertices.items()},
                     unions={str(v):str(t) for v,t in edges.items()},strong_balls=ball_rows,
                     forest_births=list(map(str,birth)),forest_parents=parent,
                     point_alpha=list(map(str,alpha)),original_entry=list(map(str,old.dates)),
                     reduced_entry=list(map(str,new.dates)),original_nodes=list(old.nodes),reduced_nodes=list(new.nodes),
                     minima=list(map(list,new.minima)),target_pair=[0,3],
                     original_pair_height='25',reduced_pair_height=str(beta),
                     partition_differences=partition_differences,
                     scope='one exhaustive Fraction geometry at default eta; no native export or statistics claim',
                     native_invocations=0,sklearn_invocations=0,GCP_used=False),sort_keys=True))

