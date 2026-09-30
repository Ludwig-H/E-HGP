"""Private Euler extremal-LCA audit; imports read-only frozen audit contexts."""
from fractions import Fraction as F
from itertools import combinations
from pathlib import Path
import hashlib, json, random, sys

SOURCE = Path(__file__).resolve().parents[2]/'audit_independant_20260930'/'fixed_k_antichain'
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
if sha(SOURCE/"SHA256SUMS") != "9ab1e991f360331a9bcfe91a84920bb1708d8370c44ac6a603fd90d161f1806e":
    raise ValueError("wrong frozen archive")
inventory = {}
for line in (SOURCE/"SHA256SUMS").read_text().splitlines():
    digest, name = line.split("  ", 1)
    if name in inventory or sha(SOURCE/name) != digest:
        raise ValueError("wrong frozen dependency")
    inventory[name] = digest
if len(inventory) != 37:
    raise ValueError("incomplete frozen dependencies")
paths = [SOURCE/"check.py", SOURCE/"antichain.py"] + sorted((SOURCE/"source_snapshot").glob("*.py"))
before = {str(p.relative_to(SOURCE)): sha(p) for p in paths}
sys.path.insert(0, str(SOURCE))
import check as reference
import antichain
def need(ok, text):
    if not ok:
        raise RuntimeError(text)
def extrema(nodes, tin, tout):
    # No deduplication, sorting, or antichain storage in the algorithm.
    left = right = None
    for v in nodes:
        if left is None or (tout[v], -tin[v]) < (tout[left], -tin[left]):
            left = v
        if right is None or tin[v] > tin[right]:
            right = v
    if left is None:
        raise ValueError("empty selected set")
    return left, right
counts = {"contexts":0, "bands":0, "arbitrary_cases":0, "different_roots":0}
def verify(f, selected):
    tin, tout = antichain.euler(f)
    distinct = set(selected)  # Oracle only, not the scanned algorithm.
    minima = [v for v in distinct if not any(
        w != v and f.is_ancestor(v,w) for w in distinct)]
    ordered = sorted(minima, key=lambda v:tin[v])
    left, right = extrema(selected,tin,tout)
    need(left == ordered[0] and right == ordered[-1], "wrong antichain extrema")
    joint = ordered[0]
    for v in ordered[1:]:
        joint = f.lca(joint,v)
        if joint is None:
            break
    candidate = f.lca(left,right)
    need(candidate == joint, "wrong LCA")
    if candidate is None:
        counts["different_roots"] += 1
    return joint

shapes = [reference.Tree([1,2,4],[2,2,-1]),
          reference.Tree([1,1,4,6,9],[2,2,4,4,-1]),
          reference.Tree([1,1,4,4,9],[2,2,3,4,-1])]
etas = (F(0),F(1,4),F(1,2),F(1),F(2),F(4))
for f in shapes:
    dates = sorted(set(f.levels))
    dates = sorted(set(dates + [(a+b)/2 for a,b in zip(dates,dates[1:])] + [dates[-1]+1]))
    pool = [(i,beta,v) for i,(beta,v) in enumerate(
        (beta,v) for beta in dates for v in range(len(f)) if f.ancestor(v,beta) == v)]
    for size in (1,2,3):
        for entries in combinations(pool,size):
            counts["contexts"] += 1
            ctx = reference.Context(f,[min(beta for _,beta,_ in entries)],[entries])
            for eta in etas:
                selected = reference.selected(ctx,0,eta)
                joint = verify(f,selected)
                reduced = antichain.cover_band_antichain(ctx,eta)
                date = max(ctx.cover_level(0),f.level(joint))
                owner = f.ancestor(joint,date,True)
                need((date,owner) == (reduced.dates[0],reduced.nodes[0]), "attachment changed")
                verify(f,list(reversed(selected))+selected)
                counts["bands"] += 1
need((counts["contexts"], counts["bands"]) == (482,2892), "wrong inherited panel size")
# Exhaustive selection subsets on small nonbinary / unary / disconnected shapes.
for parents in ([1,2,3,4,-1],[4,4,5,5,6,6,-1],[-1,-1,-1],
                [3,3,4,-1,-1], [2,2,4,4,-1]):
    f = reference.Tree([1]*len(parents),parents)
    for mask in range(1,1<<len(parents)):
        selected = [v for v in range(len(parents)) if mask>>v&1]
        verify(f,selected+list(reversed(selected)))
        counts["arbitrary_cases"] += 1
rng = random.Random(9302026)
for _ in range(300):
    n = rng.randrange(1,31)
    parents = [-1 if v == n-1 or rng.randrange(5) == 0 else rng.randrange(v+1,n) for v in range(n)]
    f = reference.Tree([1]*n,parents)
    for _ in range(20):
        selected = [rng.randrange(n) for _ in range(rng.randrange(1,2*n+1))]
        verify(f,selected)
        counts["arbitrary_cases"] += 1
# Tie breaker is essential: a selected ancestor shares tout with its final child.
f = reference.Tree([1,1,1],[1,2,-1])
tin,tout = antichain.euler(f)
need(min([2,1,0],key=lambda v:tout[v]) == 2, "tie mutant fixture not causal")
need(extrema([2,1,0],tin,tout) == (0,0), "tie break failed")
need(f.lca(2,0) != f.lca(0,0), "tie mutant would be harmless")
after = {str(p.relative_to(SOURCE)):sha(p) for p in paths}
need(before == after, "read-only source inputs changed")
print(json.dumps({"status":"PASS","counts":counts,"pins_before":before,"pins_after":after,
    "native_executions":0,"GCP_used":False,"source_hash":sha(Path(__file__)),
    "limits":"Structural scan only; pairwise ancestry oracle; no FULL implementation, geometry, D bound or LCA-index performance qualification."},sort_keys=True))
