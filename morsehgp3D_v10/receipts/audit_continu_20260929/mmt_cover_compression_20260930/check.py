from fractions import Fraction as F
from collections import defaultdict
import json, random, sys

MUTANT = sys.argv[1] if len(sys.argv) == 2 else ''

def need(ok, label, case=''):
    if not ok:
        print(json.dumps({'status':'MISMATCH','label':label,'case':case,'mutant':MUTANT},sort_keys=True))
        raise SystemExit(3)

class Tree:
    def __init__(self, parent, birth):
        self.parent, self.birth = parent, list(map(F,birth))
        self.n = len(parent)
        roots = [i for i,p in enumerate(parent) if p < 0]
        need(len(roots) == 1, 'one root')
        self.root = roots[0]
        self.children = [[] for _ in parent]
        for i,p in enumerate(parent):
            if p >= 0:
                need(self.birth[i] <= self.birth[p], 'monotone tree')
                self.children[p].append(i)
        self.depth = [0]*self.n
        self.tin, self.tout = [0]*self.n, [0]*self.n
        order = []
        def visit(v):
            self.tin[v] = len(order)
            order.append(v)
            for c in self.children[v]:
                self.depth[c] = self.depth[v]+1
                visit(c)
            self.tout[v] = len(order)
        visit(self.root)
        need(len(order) == self.n, 'connected tree')
        self.up = [[self.root if p < 0 else p for p in parent]]
        for _ in range(self.n.bit_length()-1):
            prev = self.up[-1]
            self.up.append([prev[prev[v]] for v in range(self.n)])
        self.lca_queries = 0

    def above(self, u, v):
        return self.tin[u] <= self.tin[v] < self.tout[u]

    def lca(self, a, b):
        self.lca_queries += 1
        if self.above(a,b):
            return a
        if self.above(b,a):
            return b
        for level in reversed(self.up):
            if not self.above(level[a],b):
                a = level[a]
        return self.up[0][a]

    def anc(self, v, s, closed=True):
        for level in reversed(self.up):
            u = level[v]
            admissible = self.birth[u] <= s if closed else self.birth[u] < s
            if MUTANT == 'open-instead-closed' and closed:
                admissible = self.birth[u] < s
            if admissible:
                v = u
        return v

    def ref_anc(self, v, s, closed):
        while self.parent[v] >= 0:
            p = self.parent[v]
            if not (self.birth[p] <= s if closed else self.birth[p] < s):
                break
            v = p
        return v

def seeds_validate(T, seeds):
    need(seeds, 'nonempty seeds')
    for v,c in seeds:
        p = T.parent[v]
        need(T.birth[v] <= c and (p < 0 or c < T.birth[p]), 'seed owner alive')

def reference(T, seeds, B):
    # Deliberately independent path-walking oracle, not the proposed port.
    cv = {}
    for v,c in seeds:
        while True:
            cv[v] = min(cv.get(v,c),c)
            if T.parent[v] < 0:
                break
            v = T.parent[v]
            c = T.birth[v]
    atoms = []
    for v,c in cv.items():
        end = min(B,T.birth[T.parent[v]]) if T.parent[v] >= 0 else B
        if end > c:
            atoms.append((v,c,end))
    return cv,atoms

def compress(T, seeds, B):
    direct = {}
    for v,c in seeds:
        direct[v] = min(direct.get(v,c),c)
    ordered = sorted(direct, key=lambda v:T.tin[v])
    nodes = set(ordered)
    before = T.lca_queries
    for a,b in zip(ordered,ordered[1:]):
        nodes.add(T.lca(a,b))
    nodes.add(T.root)
    ordered_nodes = sorted(nodes,key=lambda v:T.tin[v])
    parent,children = {},{v:[] for v in nodes}
    stack = []
    for v in ordered_nodes:
        while stack and not T.above(stack[-1],v):
            stack.pop()
        parent[v] = stack[-1] if stack else -1
        if stack:
            children[stack[-1]].append(v)
        stack.append(v)
    need(len(nodes) <= 2*len(direct), 'virtual size bound')
    need(T.lca_queries-before <= len(direct)-1, 'adjacent LCA count')
    atoms = []
    for v in ordered_nodes:
        start = T.birth[v] if children[v] else direct[v]
        if MUTANT == 'early-activation':
            start = T.birth[v]
        end = min(B,T.birth[parent[v]]) if parent[v] >= 0 else B
        if MUTANT == 'ignore-virtual-parent':
            end = B
        if end > start:
            atoms.append((v,start,end))
    return atoms, {'seeds_distinct':len(direct),'virtual_nodes':len(nodes),'adjacent_lca_queries':T.lca_queries-before}

def masses(T, atoms, s, closed, ref=False):
    out = defaultdict(F)
    for v,c,e in atoms:
        value = min(e,s)-c
        if value > 0:
            u = T.ref_anc(v,s,closed) if ref else T.anc(v,s,closed)
            out[u] += value
    return {v:w for v,w in out.items() if w > 0}

def median_events(T, atoms):
    W = sum((e-c for v,c,e in atoms),F(0))
    cumulative = F(0)
    m = None
    for v,c,e in sorted(atoms,key=lambda row:T.tin[row[0]]):
        cumulative += e-c
        if 2*cumulative >= W:
            m = v
            break
    need(m is not None, 'weighted median')
    events = defaultdict(lambda:[F(0),0])
    for v,c,e in atoms:
        h = T.lca(v,m)
        a = max(c,T.birth[h])
        if MUTANT == 'wrong-median-join':
            a = c
        events[a][0] += min(a,e)-c
        if a < e:
            events[a][1] += 1
            events[e][1] -= 1
    return W,m,sorted(events.items())

def line(events,s,closed):
    mass,slope,prev = F(0),0,None
    for e,(jump,delta) in events:
        if e > s or (e == s and not closed):
            break
        if prev is not None:
            mass += slope*(e-prev)
        mass += jump
        slope += delta
        need(slope in (0,1), 'valid covering-line slope')
        prev = e
    return mass+(slope*(s-prev) if prev is not None else 0)

def quotient(T,seeds):
    top = []
    for v in range(T.n):
        u = v
        while T.parent[u] >= 0 and T.birth[T.parent[u]] == T.birth[u]:
            u = T.parent[u]
        top.append(u)
    reps = sorted(set(top))
    ids = {v:i for i,v in enumerate(reps)}
    qparent = [-1 if T.parent[v] < 0 else ids[top[T.parent[v]]] for v in reps]
    Q = Tree(qparent,[T.birth[v] for v in reps])
    return Q, [(ids[top[v]],c) for v,c in seeds], [ids[top[v]] for v in range(T.n)]

def run_case(name,T,seeds,eta):
    seeds = [(v,F(c)) for v,c in seeds]
    seeds_validate(T,seeds)
    A = min(c for v,c in seeds)
    need(A > 0, 'positive alpha')
    B = (1+eta)*A
    cv,ref = reference(T,seeds,B)
    atoms,stats = compress(T,seeds,B)
    W = sum((e-c for v,c,e in ref),F(0))
    need(W == sum((e-c for v,c,e in atoms),F(0)), 'W compression',name)
    need(W > 0, 'positive W',name)
    wm,m,events = median_events(T,atoms)
    need(wm == W, 'median W',name)
    levels = sorted(set(T.birth)|{c for v,c in seeds}|{A,B}|{e for e,_ in events})
    cuts = levels+[F(a+b,2) for a,b in zip(levels,levels[1:])]+[levels[-1]+1]
    Q,qseeds,qids = quotient(T,seeds)
    seeds_validate(Q,qseeds)
    _qcv,qatoms = reference(Q,qseeds,B)
    comparisons = 0
    for s in cuts:
        for closed in (False,True):
            expected = masses(T,ref,s,closed,ref=True)
            actual = masses(T,atoms,s,closed)
            need(actual == expected, 'component masses compression',name)
            qm = masses(Q,qatoms,s,closed,ref=True)
            mapped = {qids[v]:w for v,w in expected.items()}
            need(qm == mapped, 'plateau quotient',name)
            mv = line(events,s,closed)
            born = T.birth[m] <= s if closed else T.birth[m] < s
            own = T.ref_anc(m,s,closed) if born else None
            need(mv == expected.get(own,F(0)), 'median line mass',name)
            G = max(expected.values(),default=F(0))
            need((2*G > W) == (2*mv > W), 'majority iff median',name)
            if 2*G > W:
                need(G == mv, 'post-majority maximum',name)
            comparisons += 5
    return dict(name=name,nodes=T.n,full_cover_nodes=len(cv),atoms=len(atoms),comparisons=comparisons,**stats)

cases = []
cases.append(('late_single_chain',Tree([-1,0,1,2,3],[12,8,5,2,0]),[(4,F(3,2))],F(10)))
cases.append(('late_two_branches',Tree([-1,0,0],[5,0,0]),[(1,F(1,2)),(2,F(9,2))],F(10)))
cases.append(('redundant_internal_seed',Tree([-1,0,0,1,1],[9,5,0,0,0]),[(3,F(1,2)),(1,F(11,2)),(2,F(2))],F(10)))
cases.append(('simultaneous_plateau',Tree([-1,0,0,1,1],[3,3,0,0,0]),[(3,F(1)),(4,F(1)),(2,F(2))],F(3)))
cases.append(('plateau_chain',Tree([-1,0,1,2,2],[4,4,4,0,0]),[(3,F(1)),(4,F(2))],F(4)))
cases.append(('root_late_seed',Tree([-1,0,0],[4,0,0]),[(0,F(9,2)),(1,F(1,2)),(1,F(1))],F(12)))
cases.append(('late_internal_no_descendant',Tree([-1,0,0,1,1],[9,5,0,0,0]),[(1,F(11,2)),(2,F(2))],F(10)))
cases.append(('long_unary_chain',Tree([-1]+list(range(30)),[F(3*(30-i)) for i in range(31)]),[(30,F(1,2))],F(200)))
rng = random.Random(20260930)
for index in range(100):
    height = rng.choice([2,3,4])
    n = 2**height-1
    parent = [-1]+[(i-1)//2 for i in range(1,n)]
    depth = [0]*n
    for i in range(1,n):
        depth[i] = depth[parent[i]]+1
    birth = [F(3*(height-1-d)) for d in depth]
    if index%7 == 0 and n >= 7:
        birth[1] = birth[0]
    T = Tree(parent,birth)
    candidates = [i for i,p in enumerate(parent) if p < 0 or birth[i] < birth[p]]
    chosen = rng.sample(candidates,rng.randint(1,min(6,len(candidates))))
    seeds = []
    for v in chosen:
        end = birth[parent[v]] if parent[v] >= 0 else birth[v]+6
        start = birth[v]+(end-birth[v])*F(rng.randint(1,4),5)
        seeds.append((v,start))
    if index%3 == 0:
        v,c = seeds[0]
        end = birth[parent[v]] if parent[v] >= 0 else c+2
        seeds.append((v,(c+end)/2))
    cases.append(('generated_%03d'%index,T,seeds,rng.choice([F(1,8),F(2,3),F(3),F(10)])))

rows = [run_case(*case) for case in cases]
print(json.dumps({'status':'PASS','scope':'abstract_rational_cover_trees_not_native_geometry',
    'cases':len(rows),'comparisons':sum(r['comparisons'] for r in rows),
    'per_seed_path_walks_in_compressor':0,'rows':rows},sort_keys=True))
