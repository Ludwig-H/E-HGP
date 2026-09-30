"""Five labelled collinear sites: intrinsic Gamma and partition lattice.
No native engine, ArmContext or developer projection imported.
"""
from fractions import Fraction as F
from itertools import combinations
import json

X = (0, 1, 2, 6, 9)
checks = 0

def require(ok, message):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(message)

def radius(face):
    return F(max(X[i] for i in face) - min(X[i] for i in face), 2)

def gamma(k, r):
    vertices = [f for f in combinations(range(len(X)), k) if radius(f) <= r]
    parent = {f: f for f in vertices}
    def find(f):
        while parent[f] != f:
            f = parent[f]
        return f
    for higher in combinations(range(len(X)), k + 1):
        if radius(higher) > r:
            continue
        faces = list(combinations(higher, k))
        require(all(f in parent for f in faces), 'inactive boundary face')
        for f in faces[1:]:
            a, b = find(faces[0]), find(f)
            parent[max(a, b)] = min(a, b)
    groups = {}
    for f in vertices:
        groups.setdefault(find(f), set()).add(f)
    ordered = sorted(groups.values(), key=lambda g: min(g))
    return ordered, {f: j for j, g in enumerate(ordered) for f in g}

def partition(k, r, eta):
    # Strong positive balls in distinct 1D data are exactly K consecutive sites:
    # p=K-2, q_min=m=2. Consecutive K+1 sites give the FULL merging events.
    windows = [tuple(range(i, i + k)) for i in range(len(X) - k + 1)]
    born = [radius(f) for f in windows]
    joins = [radius(tuple(range(i, i + k + 1))) for i in range(len(X) - k)]
    components, owner_at_cut = gamma(k, r)
    groups, rows = {}, []
    for s in range(len(X)):
        candidates = [j for j, f in enumerate(windows) if s in f]
        alpha = min(born[j] for j in candidates)
        chosen = [j for j in candidates if born[j] <= (1 + eta) * alpha]
        low, high = min(chosen), max(chosen)
        date = max([alpha] + joins[low:high])
        if r < date:
            owner = ('singleton', s)
        else:
            require(windows[low] in owner_at_cut, 'owner not active')
            ids = {owner_at_cut[windows[j]] for j in chosen}
            require(len(ids) == 1, 'selected witnesses not joined at entry')
            owner = ('component', owner_at_cut[windows[low]])
        groups.setdefault(owner, set()).add(s)
        rows.append({'site':s, 'first_radius':str(alpha), 'windows':chosen,
                     'entry_radius':str(date), 'owner':owner})
    return tuple(sorted((tuple(sorted(g)) for g in groups.values()))), rows, owner_at_cut

def refines(a, b):
    return all(any(set(x) <= set(y) for y in b) for x in a)

def meet(*partitions):
    signatures = [[] for _ in X]
    for p in partitions:
        for label, block in enumerate(p):
            for i in block:
                signatures[i].append(label)
    groups = {}
    for i, s in enumerate(signatures):
        groups.setdefault(tuple(s), []).append(i)
    return tuple(sorted(tuple(g) for g in groups.values()))

def join(*partitions):
    parent = list(range(len(X)))
    def find(a):
        while parent[a] != a:
            a = parent[a]
        return a
    for p in partitions:
        for block in p:
            for i in block[1:]:
                a, b = find(block[0]), find(i)
                parent[max(a,b)] = min(a,b)
    groups = {}
    for i in range(len(X)):
        groups.setdefault(find(i), []).append(i)
    return tuple(sorted(tuple(g) for g in groups.values()))

eta = F(1,8)
r = F(3)
p2, rows2, map2 = partition(2,r,eta)
p3, rows3, map3 = partition(3,r,eta)
p1, rows1, map1 = partition(1,r,eta)
require(p2 == ((0,1,2),(3,4)), 'unexpected K2 partition')
require(p3 == ((0,1,2,3),(4,)), 'unexpected K3 partition')
require(p1 == ((0,1,2,3,4),), 'unexpected K1 partition')
a, b = set(p2[1]), set(p3[0])
require(bool(a & b) and bool(a-b) and bool(b-a), 'no proper crossing')
g3, _ = gamma(3,r)
images = {map2[face] for f in g3[0] for face in combinations(f,2)}
require(len(images) == 1, 'vertical map not well-defined')
image = next(iter(images))
require(rows2[3]['owner'] != ('component', image), 'point owners commute unexpectedly')
m, j = meet(p2,p3), join(p2,p3)
require(m == ((0,1,2),(3,),(4,)), 'unexpected common refinement')
require(j == p1, 'unexpected coarsening')
require(refines(m,p2) and refines(m,p3), 'meet not a refinement')
require(refines(p2,j) and refines(p3,j), 'join not a coarsening')
# All event radii, plus open cuts and endpoints: check nestedness of each
# single-order projection and of the two lattice constructions on this fixture.
events = {F(0)}
for size in range(1,6):
    events.update(radius(f) for f in combinations(range(5),size))
ordered = sorted(events)
cuts = sorted(events | {(a+b)/2 for a,b in zip(ordered,ordered[1:])})
previous = None
for cut in cuts:
    p = [partition(k,cut,eta)[0] for k in (1,2,3)]
    current = p + [meet(p[1],p[2]), join(p[1],p[2])]
    require(refines(p[1],p[0]) and refines(p[2],p[0]), 'covered block escapes L1')
    require(join(*p) == p[0], 'join including K1 differs from K1')
    if previous is not None:
        for old, new in zip(previous,current):
            require(refines(old,new), 'non-nested cuts')
    previous = current
print(json.dumps({'status':'pass','sites':list(X),'eta':str(eta),
                  'radius':str(r),'beta':str(r*r),'partition_K1':p1,
                  'partition_K2':p2,'partition_K3':p3,
                  'crossing':{'K2':p2[1],'K3':p3[0]},
                  'point_index':3,'point_coordinate':X[3],
                  'vertical_component_image_K2':image,
                  'K2_point_owner':rows2[3]['owner'],
                  'K3_point_owner':rows3[3]['owner'],
                  'common_refinement':m,'common_coarsening':j,
                  'rows_K2':rows2,'rows_K3':rows3,
                  'cuts_checked':len(cuts),'checks':checks},sort_keys=True))
