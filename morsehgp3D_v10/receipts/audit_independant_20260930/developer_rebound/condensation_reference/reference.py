"""Bounded exact API oracle and cohort expansion; no production implementation.

The oracle sweeps labelled closed-cut partitions, independently of cached
subtree masses. Expansion contracts equal geometric ranks first and flattens
equal cohort/parent events. All paid events use positive rational beta.
"""
from fractions import Fraction as F
from math import isqrt
from random import Random
import json


def require(ok, message):
    if not ok:
        raise ValueError(message)


def validate(d):
    n = len(d['birth'])
    require(n and len(d['children']) == n and len(d['parent']) == n, 'node arrays')
    require([v for v, p in enumerate(d['parent']) if p is None] == [n-1], 'one final root')
    seen = []
    for v, children in enumerate(d['children']):
        require(d['birth'][v] > 0, 'positive birth')
        for c in children:
            require(0 <= c < v and d['parent'][c] == v and
                    d['birth'][c] <= d['birth'][v], 'edge invariant')
            seen.append(c)
    require(sorted(seen) == list(range(n-1)), 'tree ownership')
    require(len(d['target']) == len(d['entry']) == len(d['weight']), 'point arrays')
    for x, v in enumerate(d['target']):
        p = d['parent'][v]
        require(d['weight'][x] >= 1 and d['birth'][v] <= d['entry'][x] and
                (p is None or d['entry'][x] <= d['birth'][p]), 'point lifetime')


def key_at(d, x, beta, closed):
    """Point must have entered; component in the closed/open cut at beta."""
    v = d['target'][x]
    while d['parent'][v] is not None:
        p = d['parent'][v]
        if d['birth'][p] > beta or (not closed and d['birth'][p] == beta):
            break
        v = p
    return v


def partition(d, beta, closed=True):
    groups = {}
    for x, e in enumerate(d['entry']):
        entered = e <= beta if closed else e < beta
        k = ('component', key_at(d, x, beta, closed)) if entered else ('singleton', x)
        groups.setdefault(k, []).append(x)
    return sorted(tuple(xs) for xs in groups.values())


def expand(d, flatten=True):
    """Event-tree adapter. Its node_cluster mapping is NOT a voting contract."""
    validate(d)
    n = len(d['birth'])
    rep = [None]*n
    for v in reversed(range(n)):
        p = d['parent'][v]
        rep[v] = rep[p] if p is not None and d['birth'][p] == d['birth'][v] else v
    kids, own = {v: [] for v in set(rep)}, {v: [] for v in set(rep)}
    for v, p in enumerate(d['parent']):
        if p is not None and rep[v] != rep[p]:
            kids[rep[p]].append(rep[v])
    for x, v in enumerate(d['target']):
        own[rep[v]].append(x)
    roots = {}
    def event(beta, children, points):
        cs, xs = [], list(points)
        for child in children:
            if child is None:
                continue
            if flatten and child['beta'] == beta:
                cs.extend(child['children'])
                xs.extend(child['points'])
            else:
                cs.append(child)
        if not cs and not xs:
            return None
        return dict(beta=beta, children=cs, points=xs)
    for v in sorted(kids):
        groups = {}
        for x in own[v]:
            groups.setdefault(d['entry'][x], []).append(x)
        base = d['birth'][v]
        current = event(base, [roots[c] for c in kids[v]], groups.pop(base, []))
        for beta, points in sorted(groups.items()):
            current = event(beta, [current], points)
        roots[v] = current
    result = dict(birth=[], children=[], parent=[], target=[None]*len(d['target']),
                  entry=list(d['entry']), weight=list(d['weight']))
    def emit(node):
        cs = [emit(c) for c in node['children']]
        v = len(result['birth'])
        result['birth'].append(node['beta'])
        result['children'].append(cs)
        result['parent'].append(None)
        for c in cs:
            result['parent'][c] = v
        for x in node['points']:
            require(result['target'][x] is None, 'point assigned twice')
            result['target'][x] = v
        return v
    emit(roots[n-1])
    validate(result)
    require(all(result['entry'][x] == result['birth'][v]
                for x, v in enumerate(result['target'])), 'event attachment')
    require(len(result['birth']) <= n + len(d['target']), 'linear node bound')
    return result


def lam(beta, z):
    if z == 2:
        return 1/beta
    a, b = isqrt(beta.numerator), isqrt(beta.denominator)
    require(a*a == beta.numerator and b*b == beta.denominator, 'rational sqrt scope')
    return F(b, a)


def oracle(d, mcs, z):
    """Decreasing-cut sweep: departures first, then surviving components."""
    validate(d)
    mass = lambda xs: sum(d['weight'][x] for x in xs)
    clusters = [dict(parent=None, birth=F(0), stability=F(0), mass=mass(range(len(d['target']))))]
    active = {0: set(range(len(d['target'])))}
    pc, pl = [None]*len(d['target']), [None]*len(d['target'])
    def drop(xs, c, value):
        for x in sorted(xs):
            require(pc[x] is None, 'point paid twice')
            pc[x], pl[x] = c, value
            clusters[c]['stability'] += d['weight'][x]*(value-clusters[c]['birth'])
    for beta in sorted(set(d['birth']) | set(d['entry']), reverse=True):
        value = lam(beta, z)
        for c, original in list(active.items()):
            points = set(original)
            cohort = {x for x in points if d['entry'][x] == beta}
            drop(cohort, c, value)
            points -= cohort
            del active[c]
            if mass(points) < mcs:
                drop(points, c, value)
                continue
            parts = {}
            for x in points:
                parts.setdefault(key_at(d, x, beta, False), set()).add(x)
            large = [xs for xs in parts.values() if mass(xs) >= mcs]
            if len(large) >= 2:
                for xs in sorted(parts.values(), key=lambda xs: min(xs)):
                    if mass(xs) < mcs:
                        drop(xs, c, value)
                        continue
                    clusters[c]['stability'] += mass(xs)*(value-clusters[c]['birth'])
                    child = len(clusters)
                    clusters.append(dict(parent=c, birth=value, stability=F(0), mass=mass(xs)))
                    active[child] = xs
            elif len(large) == 1:
                keep = large[0]
                drop(points-keep, c, value)
                active[c] = keep
            else:
                drop(points, c, value)
    require(not active and all(c is not None for c in pc), 'unfinished sweep')
    return dict(clusters=clusters, point_cluster=pc, point_lambda=pl)


def static_head(d, mcs, z):
    """Exact arithmetic transcription of published head; not the oracle."""
    own = [[] for _ in d['birth']]
    for x, v in enumerate(d['target']):
        own[v].append(x)
    mass = [0]*len(own)
    for v, cs in enumerate(d['children']):
        mass[v] = sum(d['weight'][x] for x in own[v]) + sum(mass[c] for c in cs)
    clusters = [dict(parent=None, birth=F(0), stability=F(0), mass=mass[-1])]
    pc, pl = [None]*len(d['target']), [None]*len(d['target'])
    def drop(v, c, value):
        for x in own[v]:
            require(pc[x] is None, 'static double payment')
            pc[x], pl[x] = c, value
            clusters[c]['stability'] += d['weight'][x]*(value-clusters[c]['birth'])
        for child in d['children'][v]:
            drop(child, c, value)
    work = [(len(own)-1, 0)]
    while work:
        v, c = work.pop()
        for x in own[v]:
            value = lam(d['entry'][x], z)
            pc[x], pl[x] = c, value
            clusters[c]['stability'] += d['weight'][x]*(value-clusters[c]['birth'])
        cs = d['children'][v]
        value = lam(d['birth'][v], z)
        big = [child for child in cs if mass[child] >= mcs]
        for child in cs:
            if len(big) >= 2 and child in big:
                clusters[c]['stability'] += mass[child]*(value-clusters[c]['birth'])
                k = len(clusters)
                clusters.append(dict(parent=c, birth=value, stability=F(0), mass=mass[child]))
                work.append((child, k))
            elif len(big) == 1 and child == big[0]:
                work.append((child, c))
            else:
                drop(child, c, value)
    return dict(clusters=clusters, point_cluster=pc, point_lambda=pl)


def canonical(out):
    clusters, pc = out['clusters'], out['point_cluster']
    members = [set() for _ in clusters]
    for x, c in enumerate(pc):
        while c is not None:
            members[c].add(x)
            c = clusters[c]['parent']
    keys = [tuple(sorted(xs)) for xs in members]
    require(len(set(keys)) == len(keys), 'nonunique cluster membership')
    rows = {}
    for c, row in enumerate(clusters):
        p = row['parent']
        rows[keys[c]] = (None if p is None else keys[p], row['birth'], row['mass'], row['stability'])
    return rows, [keys[c] for c in pc], out['point_lambda']


def selected(out, single, method):
    cs, pc = out['clusters'], out['point_cluster']
    kids = [[] for _ in cs]
    for c, row in enumerate(cs):
        if row['parent'] is not None:
            kids[row['parent']].append(c)
    best, choose = [F(0)]*len(cs), set()
    for c in reversed(range(len(cs))):
        sub = sum((best[k] for k in kids[c]), F(0))
        take = not kids[c] or (method == 'eom' and (c != 0 or single) and sub <= cs[c]['stability'])
        best[c] = cs[c]['stability'] if take else sub
        if take:
            choose.add(c)
    if not single:
        choose.discard(0)
    for c in sorted(tuple(choose)):
        p = cs[c]['parent']
        while p is not None:
            if p in choose:
                choose.discard(c)
                break
            p = cs[p]['parent']
    blocks, noise = {}, []
    for x, c in enumerate(pc):
        while c is not None and c not in choose:
            c = cs[c]['parent']
        if c is None:
            noise.append(x)
        else:
            blocks.setdefault(c, []).append(x)
    return sorted(tuple(xs) for xs in blocks.values()), tuple(noise)


def fixtures():
    base = dict(birth=list(map(F, [1, 16, 25])), children=[[], [], [0, 1]], parent=[2, 2, None],
                target=[0, 0, 0, 1, 1], entry=list(map(F, [1, 4, 9, 16, 16])), weight=[1]*5)
    yield 'staggered_API', base, [2], [1, 2]
    # Two nominal children born at the root plateau must not become clusters
    # of zero lifetime when the root is excluded.
    equal = dict(birth=list(map(F, [25, 25, 25])), children=[[], [], [0, 1]], parent=[2, 2, None],
                 target=[0, 0, 1, 1], entry=[F(25)]*4, weight=[1]*4)
    yield 'equal_geometry_plateau_API', equal, [2], [1, 2]
    # Child cohort at its parent's split: four direct departures, two groups
    # of mass 1 remain. Counting the old mass 3 of each child is erroneous.
    plateau = dict(birth=list(map(F, [1, 4, 25])), children=[[], [], [0, 1]], parent=[2, 2, None],
                   target=[0, 0, 0, 1, 1, 1], entry=list(map(F, [1, 25, 25, 4, 25, 25])), weight=[1]*6)
    yield 'cohort_at_geometric_split_API', plateau, [2], [1, 2]
    raw = json.loads(open(__file__.replace('reference.py', 'historical_k3.json')).read())
    order = raw['orders'][0]
    lv = [F(int(a), int(b)) for a, b in raw['levels']]
    historical = dict(birth=[lv[v[0]] for v in order['nodes']],
                      children=[v[3] for v in order['nodes']],
                      parent=[None if v[1] == -1 else v[1] for v in order['nodes']],
                      target=order['cover_node'], entry=[lv[r] for r in order['cover_lv']], weight=[1]*6)
    yield 'historical_K3_cover', historical, [2, 6], [2]
    rng = Random(20260930)
    for case in range(96):
        birth, kids, parent, target, entry, weight = [], [], [], [], [], []
        def make(depth):
            cs = [make(depth-1) for _ in range(rng.randrange(2, 4))] if depth and rng.random() < .7 else []
            rank = (max(birth[c] for c in cs) if cs else 0) + rng.randrange(3)
            v = len(birth)
            birth.append(rank); kids.append(cs); parent.append(None)
            for c in cs:
                parent[c] = v
            return v
        make(3)
        for v, cs in enumerate(kids):
            for _ in range(rng.randrange(0 if cs else 1, 4)):
                target.append(v)
                hi = birth[parent[v]] if parent[v] is not None else birth[v]+3
                entry.append(F((rng.randint(birth[v], hi)+1)**2))
                weight.append(rng.randrange(1, 4))
        d = dict(birth=[F((r+1)**2) for r in birth], children=kids, parent=parent,
                 target=target, entry=entry, weight=weight)
        total = sum(weight)
        yield 'random_API_%02d' % case, d, sorted(set([2, min(5, total), total])), [1, 2]
