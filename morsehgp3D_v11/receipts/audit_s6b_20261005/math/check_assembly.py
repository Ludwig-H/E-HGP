#!/usr/bin/env python3
"""Python-only model of new S6b order/closure, checked against exact S1 and its new comparator.

This translates postorder and stable bucket logic; it does not execute the native product.
Synthetic JSON checks the comparator's response to important output faults.
"""
import contextlib
import copy
import hashlib
import io
import json
import pathlib
import sys
from fractions import Fraction
from itertools import combinations
from math import comb

sys.dont_write_bytecode = True
HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE/'snapshot/morsehgp3D_v11/tests/supports'))
import hierarchy_fraction as H  # noqa: E402


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def choose(n, k):
    return comb(n, k) if 0 <= k <= n else 0


def level_hex(value):
    f = Fraction(value)
    return [hex(f.numerator)[2:], hex(f.denominator)[2:]]


def stackless(nodes):
    n = len(nodes)
    root = next(v for v, node in enumerate(nodes) if node['parent'] is None)
    post, size = [None]*n, [None]*n
    post[root], size[root] = 0, 1
    v, rank, steps = root, 0, 0
    while True:
        require(steps < 2*n, 'postorder exceeded bound')
        steps += 1
        children = nodes[v]['children']
        c = post[v]
        if c < len(children):
            w = children[c]
            require(nodes[w]['parent'] == v and (c == 0 or children[c-1] < w), 'child invariant')
            post[v] = c+1
            post[w], size[w] = 0, 1
            v = w
            continue
        post[v], rank = rank, rank+1
        if v == root:
            break
        up = nodes[v]['parent']
        size[up] += size[v]
        v = up
    require(rank == n and post == [node['post'] for node in nodes], 'postorder differs from S1')
    return post, size


def synthetic(case, erase_late=False):
    name, points, ids, k, doc, oracle = case
    cloud = H.sample_judge.Cloud([tuple(p) for p in points], ids)
    post, sizes = stackless(doc['nodes'])
    src = []
    levels = sorted({Fraction(b['level']) for b in doc['balls']} | {Fraction(n['level']) for n in doc['nodes']})
    rank = {level: i for i, level in enumerate(levels)}
    for bi, ball in enumerate(doc['balls']):
        if erase_late and name == 'growth_abcz' and ball['level'] == '25':
            continue
        order = sorted(range(len(ball['supports'])), key=lambda j:
                       (len(ball['supports'][j]), sorted(cloud.rank[tuple(p)] for p in ball['supports'][j])))
        supports = [sorted(ball['supports'][j], key=lambda p: cloud.rank[tuple(p)]) for j in order]
        star = tuple(cloud.rank[tuple(p)] for p in supports[0])
        # Catalogue order at equal levels: S* padded with kNone. Check it against ordinary lex on actual supports.
        src.append((bi, ball, supports, star))
    src.sort(key=lambda x: (Fraction(x[1]['level']), x[3] + (0xFFFFFFFF,)*(4-len(x[3]))))
    for a, b in combinations(src, 2):
        if a[1]['level'] == b[1]['level']:
            require(not (a[3] == b[3][:len(a[3])] or b[3] == a[3][:len(b[3])]),
                    'distinct positive S* nested at equal level')
            require((a[3] < b[3]) ==
                    (a[3]+(0xFFFFFFFF,)*(4-len(a[3])) < b[3]+(0xFFFFFFFF,)*(4-len(b[3]))),
                    'padding reverses S* order')
    # Translate the new stable reverse fill of cumulative bucket ends.
    off = [0]*(len(post)+1)
    for _bi, ball, _supports, _star in src:
        off[post[ball['node']]+1] += 1
    for j in range(1, len(off)):
        off[j] += off[j-1]
    origins = [None]*len(src)
    for i in range(len(src)-1, -1, -1):
        at = post[src[i][1]['node']]+1
        off[at] -= 1
        origins[off[at]] = i
    off = off[1:]+[len(src)]
    direct = sorted(range(len(src)), key=lambda i: (post[src[i][1]['node']], i))
    require(origins == direct, 'stable bucket differs from direct sort')
    balls = []
    for key in origins:
        _bi, b, supports, _star = src[key]
        center, level = [Fraction(c) for c in b['center']], Fraction(b['level'])
        shell = sorted(tuple(p) for p in points if sum((Fraction(x)-c)**2 for x,c in zip(p,center)) == level)
        # Trace closure and all counts are recomputed from set containment; no native integer formula is called.
        masks = [sum(1 << shell.index(tuple(p)) for p in q) for q in supports]
        closure = [0]*(len(shell)+1)
        for subset in range(1 << len(shell)):
            if any(subset & q == q for q in masks):
                closure[subset.bit_count()] += 1
        p, m, t = b['p'], b['m'], k-b['p']
        require(m == len(shell), 'exact shell size differs')
        counts = dict(kparties_reliees=choose(p+m,k), compressed_parts=choose(m,t),
                      strict_traces=choose(m,t)-(closure[t] if t <= m else 0),
                      cofaces=sum(choose(p,k+1-j)*closure[j] for j in range(m+1)),
                      gabriel_cofaces=closure[t+1] if t+1 <= m else 0)
        require(all(counts[field] == b[field] for field in counts), 'closure/counts differ from S1')
        row = dict((field, copy.deepcopy(b[field])) for field in H.COMPARED_BALL if field in b)
        row.pop('center', None)
        row.update(key=key, rank=rank[Fraction(b['level'])], level=level_hex(b['level']), supports=supports,
                   sites=[[cloud.rank[tuple(p)] for p in q] for q in supports], **counts)
        row['cofaces_support'] = [choose(p+m-len(q),k+1-len(q)) for q in supports]
        row['gabriel_cofaces_support'] = [choose(m-len(q),t+1-len(q)) for q in supports]
        balls.append(row)
    nodes = []
    for v, node in enumerate(doc['nodes']):
        own = list(range(off[post[v]], off[post[v]+1]))
        birth_key = next((b['key'] for b in balls if b['node'] == v and b['role'] == 'naissance'), None)
        birth_site = [int(Fraction(c)) for c in node['birth_center']] if node['kind'] == 0 else None
        nodes.append(dict(level=level_hex(node['level']), parent=node['parent'], children=node['children'],
                          kind=node['kind'], post=post[v], size=sizes[v], balls=own,
                          birth_key=birth_key, birth_site=birth_site))
        lo = off[post[v]-sizes[v]+1]
        hi = off[post[v]+1]
        # A dated snapshot of a *living* node is a prefix: all strict descendants precede its own dated balls.
        parent_level = Fraction(doc['nodes'][node['parent']]['level']) if node['parent'] is not None else None
        for cut in levels:
            if cut < Fraction(node['level']) or parent_level is not None and cut >= parent_level:
                continue
            flags = [Fraction(src[origins[j]][1]['level']) <= cut for j in range(lo, hi)]
            require(flags == sorted(flags, reverse=True), 'living-node snapshot is not a prefix')
    return dict(format='hgp11_hierarchy_probe', version=1, check='', k=k, n=doc['n'], sites=doc['sites'],
                ids=doc['ids'], nodes=nodes, balls=balls)


def compare(got, case):
    gate = H.mhgp11_gate.Gate('counter_hierarchy')
    stream = io.StringIO()
    with contextlib.redirect_stdout(stream):
        H.compare_one(gate, case[0]+' K'+str(case[3]), json.dumps(got), case)
        code = gate.finish(floor=1)
    return dict(code=code, checks=gate.checks, failures=gate.failures,
                first_failure=next((s for s in stream.getvalue().splitlines() if s.startswith('ECHEC ')), None))


fixtures = H.test_supports.FIXTURES
clouds = [(name, fixtures[name][0], ks) for name, ks in
          [('carre',(1,3)), ('passagere',(1,)), ('triangle_equilateral',(2,)), ('cube',(1,2)),
           ('growth_abcz',(3,)), ('cercle_bt_n3',(2,))]]
gate = H.mhgp11_gate.Gate('counter_oracle')
cases, compared, excluded = H.oracle_cases(gate, 21, clouds)
require(gate.failures == 0 and compared == 6 and excluded == 0 and len(cases) == 8, 'bounded cases differ')
docs = [synthetic(case) for case in cases]
controls = [compare(doc, case) for doc, case in zip(docs, cases)]
require(all(c['code'] == 0 for c in controls), 'model control rejected by the new comparator')


def at(name,k):
    return next(i for i,c in enumerate(cases) if c[0] == name and c[3] == k)


mutations = {}
for name in ['unstable_own_order', 'zero_coface_tetra_omitted', 'incidences_for_distinct_cofaces',
             'late_internal_ball_omitted', 'passenger_internal', 'subtree_size_wrong']:
    ci = at('carre',1)
    changed = copy.deepcopy(docs[ci])
    if name == 'unstable_own_order':
        changed['balls'].reverse()
    elif name == 'zero_coface_tetra_omitted':
        ci = at('cube',1); changed = copy.deepcopy(docs[ci])
        b = next(b for b in changed['balls'] if any(len(q)==4 for q in b['supports']))
        keep = [j for j,q in enumerate(b['supports']) if len(q)!=4]
        for field in ['supports','sites','cofaces_support','gabriel_cofaces_support']:
            b[field] = [b[field][j] for j in keep]
    elif name == 'incidences_for_distinct_cofaces':
        ci = at('carre',3); changed = copy.deepcopy(docs[ci]); b = changed['balls'][0]
        require(b['cofaces'] == 1 and sum(b['cofaces_support']) == 2, 'overlap witness absent')
        b['cofaces'] = 2
    elif name == 'late_internal_ball_omitted':
        ci = at('growth_abcz',3); changed = synthetic(cases[ci], erase_late=True)
    elif name == 'passenger_internal':
        ci = at('passagere',1); changed = copy.deepcopy(docs[ci])
        b = next(b for b in changed['balls'] if b['role']=='fusion' and b['components']==1)
        b['role'] = 'interne'
    elif name == 'subtree_size_wrong':
        changed['nodes'][-1]['size'] -= 1
    mutations[name] = compare(changed,cases[ci])
    require(mutations[name]['code'] == 1 and mutations[name]['failures'] > 0, name+' not rejected')

result = dict(scope='Python model + exact S1 + synthetic-output comparator, no native execution',
              native_executed=False, whole_963_order_gate_run=False, pin='9e7428995e3b359301d58d882610d9d4ee720fad',
              clouds=compared, orders=len(cases), counts=H.count(cases,compared,excluded), controls=controls,
              mutations=mutations, stable_bucket_equals_direct=True, living_snapshot_prefix=True,
              equal_level_padded_star_order=True,
              canonical_sha256=hashlib.sha256(json.dumps([c[4] for c in cases],sort_keys=True,
                                                        separators=(',',':')).encode()).hexdigest())
print(json.dumps(result,sort_keys=True,separators=(',',':')))
