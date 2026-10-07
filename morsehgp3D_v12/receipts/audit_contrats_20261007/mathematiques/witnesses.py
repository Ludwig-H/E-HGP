#!/usr/bin/env python3
"""Small exact models; no v12 engine, native geometry or performance claim.

T7 is compared with independent exhaustive convex-hull feasibility (Caratheodory).
T4--T6 compare an abstract chronological UF with batch connected components.
The only production import is the existing v11 strict FULL reader, to check its
actual semantic hash on a valid tiny synthetic payload. No assert statements.
"""
from fractions import Fraction as Q
from itertools import combinations, permutations
from pathlib import Path
import hashlib
import json
import struct
import sys
sys.dont_write_bytecode = True


def need(value, message):
    if not value:
        raise RuntimeError(message)


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])


def unique_solution(points):
    """Solve sum(w_i*p_i)=0, sum(w_i)=1 by rational row reduction."""
    n = len(points)
    a = [[Q(p[j]) for p in points]+[Q(0)] for j in range(3)]
    a.append([Q(1)]*n+[Q(1)])
    pivots = []
    for col in range(n):
        row = next((r for r in range(len(pivots), 4) if a[r][col]), None)
        if row is None:
            return None
        k = len(pivots)
        a[k], a[row] = a[row], a[k]
        scale = a[k][col]
        a[k] = [x/scale for x in a[k]]
        for r in range(4):
            if r != k:
                scale = a[r][col]
                a[r] = [x-scale*y for x, y in zip(a[r], a[k])]
        pivots.append(col)
    if any(all(x == 0 for x in row[:n]) and row[-1] for row in a):
        return None
    return [a[k][-1] for k in range(n)]


def blockers(points):
    result = []
    for q in range(1, min(4, len(points))+1):
        for ids in combinations(range(len(points)), q):
            w = unique_solution([points[i] for i in ids])
            if w is not None and all(x > 0 for x in w):
                result.append(sum(1 << i for i in ids))
    return result


def t7_windows(points):
    out, done = set(), set()
    m = len(points)
    for i, j in combinations(range(m), 2):
        normal = cross(points[i], points[j])
        if (i, j) in done or normal == (0, 0, 0):
            continue
        z = [x for x in range(m) if dot(normal, points[x]) == 0]
        done.update(combinations(z, 2))
        for sign in (1, -1):
            p = sum(1 << x for x in range(m) if sign*dot(normal, points[x]) > 0)
            for origin in z:
                w = 1 << origin
                for x in z:
                    if sign*dot(cross(points[origin], points[x]), normal) > 0:
                        w |= 1 << x
                out.add(p | w)
    if not out:
        need(m == 2 and points[0] == tuple(-x for x in points[1]), 'T7 fallback domain')
        out = {1, 2}
    return sorted(out)


def components(vertices, adjacent):
    remaining, result = set(vertices), []
    while remaining:
        seed = min(remaining)
        remaining.remove(seed)
        found, stack = {seed}, [seed]
        while stack:
            x = stack.pop()
            hit = {y for y in remaining if adjacent(x, y)}
            remaining -= hit
            found |= hit
            stack.extend(hit)
        result.append(frozenset(found))
    return set(result)


def quotient_checks():
    fixtures = {
        'antipodes': [(1,0,0),(-1,0,0)],
        'square': [(1,1,0),(-1,1,0),(-1,-1,0),(1,-1,0)],
        'tetrahedron': [(1,1,1),(1,-1,-1),(-1,1,-1),(-1,-1,1)],
        'octahedron': [tuple(s if a == j else 0 for a in range(3)) for j in range(3) for s in (-1,1)],
        'cube': [(x,y,z) for x in (-1,1) for y in (-1,1) for z in (-1,1)],
        'circle_nonmaximal': [(5,0,0),(4,3,0),(-3,4,0),(-3,-4,0)],
        'shared_plane_axis': [(5,0,0),(-5,0,0),(0,5,0),(0,-5,0),(0,0,5),(0,0,-5),(3,4,0),(0,3,4)],
    }
    result = []
    for name, source in fixtures.items():
        points = [tuple(p) for p in source]
        m = len(points)
        need(len(set(points)) == m and len({dot(p,p) for p in points}) == 1 and dot(points[0],points[0]) > 0, 'shell domain')
        forbidden = blockers(points)
        need(forbidden, 'critical shell')
        def sep(mask):
            return not any(mask & b == b for b in forbidden)
        windows = t7_windows(points)
        need(all(sep(w) for w in windows), 'T7 separability')
        need(all(not sep(a) or any(a & w == a for w in windows) for a in range(1, 1 << m)), 'T7 covering')
        orders = []
        for t in range(1, m+1):
            vertices = [a for a in range(1, 1 << m) if a.bit_count() == t and sep(a)]
            raw = components(vertices, lambda a,b: sep(a | b))
            active = [w for w in windows if w.bit_count() >= t]
            quotient = components(active, lambda a,b: (a & b).bit_count() >= t)
            expanded = {frozenset(a for a in vertices if any(a & w == a for w in cls)) for cls in quotient}
            need(raw == expanded, 'T7 quotient components')
            union_raw, union_windows = 0, 0
            for a in vertices:
                union_raw |= a
            for w in active:
                union_windows |= w
            need(union_raw == union_windows, 'T7 local coverage complement')
            orders.append({'t':t, 'strict_parts':len(vertices), 'pieces':len(raw)})
        nonmax = [w for w in windows if any(w != z and w & z == w for z in windows)]
        result.append({'name':name, 'points':points, 'windows':len(windows), 'nonmaximal_windows':nonmax, 'orders':orders})
    need(next(x for x in result if x['name'] == 'circle_nonmaximal')['nonmaximal_windows'], 'nonmaximal witness')
    return result


def uf_case(junctions, n=8):
    up, size = list(range(n)), [1]*n
    top = [('b',i) for i in range(n)]
    attach, wins, events, jt = {}, [[] for _ in range(n)], [], []
    def find(x):
        while up[x] != x:
            x = up[x]
        return x
    for rank, members in junctions:
        x = find(members[0])
        for leaf in members[1:]:
            y = find(leaf)
            if x == y:
                continue
            a, b = top[x], top[y]
            if size[x] < size[y]:
                x, y = y, x
            eid = len(events)
            events.append((rank,a,b,x))
            up[y] = x
            attach[y] = (x,rank)
            size[x] += size[y]
            top[x] = ('e',eid)
            wins[x].append(eid)
        jt.append(top[x])
    def leaves(symbol):
        if symbol[0] == 'b':
            return frozenset((symbol[1],))
        event = events[symbol[1]]
        return leaves(event[1]) | leaves(event[2])
    maxrank = max(r for r,_ in junctions)
    classes = []
    for r in range(1, maxrank+1):
        ids = [i for i,e in enumerate(events) if e[0] == r]
        def linked(i,j):
            return ('e',i) in events[j][1:3] or ('e',j) in events[i][1:3]
        classes.extend(components(ids, linked))
    node_of, parents, nodes = {}, {}, set()
    for cls in classes:
        reach = frozenset().union(*(leaves(('e',i)) for i in cls))
        node = (events[next(iter(cls))][0], tuple(sorted(reach)))
        nodes.add(node)
        for i in cls:
            node_of[('e',i)] = node
    for i in range(n):
        node_of[('b',i)] = (0,(i,))
    for eid, event in enumerate(events):
        for child in event[1:3]:
            child_node, node = node_of[child], node_of[('e',eid)]
            if child_node != node:
                need(child_node not in parents or parents[child_node] == node, 'unique parent')
                parents[child_node] = node
    def closed(cut):
        edges = [set(m) for r,m in junctions if r <= cut]
        return components(range(n), lambda a,b: any(a in edge and b in edge for edge in edges))
    expected = set()
    for r in range(1,maxrank+1):
        old = closed(r-1)
        for cls in closed(r):
            if sum(part <= cls for part in old) >= 2:
                node = (r,tuple(sorted(cls)))
                expected.add(node)
                actual_children = {frozenset(child[1]) for child,parent in parents.items() if parent == node}
                need(actual_children == {part for part in old if part <= cls}, 'T4 exact children')
    need(nodes == expected, 'T4 batch equality')
    queries, maxdepth = 0, 0
    for leaf in range(n):
        x, depth = leaf, 0
        while x in attach:
            x = attach[x][0]
            depth += 1
        maxdepth = max(maxdepth,depth)
        need(depth <= n.bit_length()-1, 'T5 logarithmic history')
        for cut in range(maxrank+1):
            x = leaf
            while x in attach and attach[x][1] <= cut:
                x = attach[x][0]
            eligible = [i for i in wins[x] if events[i][0] <= cut]
            answer = node_of[('e',eligible[-1])] if eligible else (0,(x,))
            cls = next(c for c in closed(cut) if leaf in c)
            need(set(answer[1]) == cls, 'T5 exact closed component')
            need(answer[0] <= cut and (answer not in parents or parents[answer][0] > cut), 'T5 living node')
            queries += 1
    climbed = 0
    for (r,members), symbol in zip(junctions,jt):
        answer = node_of[symbol]
        if answer in parents and parents[answer][0] == r:
            answer = parents[answer]
            climbed += 1
        cls = next(c for c in closed(r) if members[0] in c)
        need(set(answer[1]) == cls, 'T6 one contracted parent')
    return queries,maxdepth,climbed


def history_checks():
    plateau = [(0,), (0,2), (4,5), (5,6), (3,4), (1,6)]
    counts = [0,0,0,0]
    for perm in permutations(plateau):
        junctions = [(1,(0,1)),(1,(2,3))]+[(2,p) for p in perm]+[(3,(6,7))]
        queries,depth,climbs = uf_case(junctions)
        counts[0] += 1
        counts[1] += queries
        counts[2] = max(counts[2],depth)
        counts[3] += climbs
    need(counts[3] > 0, 'T6 parent-climb witness exercised')
    return dict(cases=counts[0], history_queries=counts[1], max_history_depth=counts[2], delayed_jtop_climbs=counts[3], geometric_realization_claimed=False)


def morton(p):
    return sum(((c >> bit) & 1) << (3*bit+axis) for axis,c in enumerate(p) for bit in range(32))


def spanning(points, lex):
    order = sorted(range(3),key=lambda i: points[i] if lex else morton(points[i]))
    rank = {i:j for j,i in enumerate(order)}
    edges = sorted(combinations(range(3),2),key=lambda e:tuple(sorted(rank[i] for i in e)))
    # All three empty diametral balls have radius squared 1/2; any first two connect.
    for i,j in edges:
        center = tuple(Q(a+b,2) for a,b in zip(points[i],points[j]))
        radius = sum((Q(a)-c)**2 for a,c in zip(points[i],center))
        need(radius == Q(1,2), 'translation fixture level')
        k = 3-i-j
        need(sum((Q(a)-c)**2 for a,c in zip(points[k],center)) > radius, 'empty diametral ball')
    return sorted(tuple(sorted(e)) for e in edges[:2])


def full_payload(points):
    def word(x):
        return struct.pack('<Q',x)
    def integer(x):
        return word(int(x<0))+word(1)+word(abs(x))
    def level(x):
        return integer(x.numerator)+integer(x.denominator)
    out = bytearray(b'MHGP11FUL1')
    for x in (21,1,3,3):
        out += word(x)
    for i in sorted(range(3),key=lambda i:morton(points[i])):
        for x in (*points[i],1,i):
            out += word(x)
    for x in (1,3,4,3,3):
        out += word(x)
    for p in sorted(points):
        out += word(3)+word(0)+word(0)+level(Q(0))
        out += b''.join(integer(x) for x in p)+integer(1)
    out += word((1<<32)-1)+word(0)+word(3)+level(Q(1,2))
    out += b''.join(word(x) for x in range(3))
    return bytes(out)


def translation_checks():
    root = Path(__file__).resolve().parents[4]
    sys.path.insert(0,str(root/'morsehgp3D_v11'/'bench'))
    from full_semantic import decode
    base = [(0,1,1),(1,0,1),(1,1,0)]
    result = []
    for shift in [(0,0,0),(1,0,0),(0,1,0),(0,0,1),(5,11,17)]:
        points = [tuple(x+t for x,t in zip(p,shift)) for p in base]
        semantic = decode(full_payload(points),21,1,3)
        result.append(dict(translation=shift,morton_edges=spanning(points,False),lex_edges=spanning(points,True),v11_full_semantic_sha256=semantic['sha256']))
    need(len({str(x['morton_edges']) for x in result}) > 1,'old Morton selection must change')
    need(len({str(x['lex_edges']) for x in result}) == 1,'lex equivariance')
    need(len({x['v11_full_semantic_sha256'] for x in result}) == len(result),'existing semantic hash contains absolute geometry')
    return result


def main():
    t1 = {'points':[0,1,2], 'K':2, 'support':[0,2], 'F':[0,1], 'ball_level':'1', 'meb_F_level':'1/4'}
    need(Q((max(t1['F'])-min(t1['F']))**2,4) == Q(1,4),'T1 counterexample')
    result = dict(schema='v12.math_contracts.independent.v1',pin='13c52bc602a4e7ea90964ee23e155a8ad2cfd301',native_engine_executed=False,t1=t1,t4_t5_t6=history_checks(),t7=quotient_checks(),translation=translation_checks())
    print(json.dumps(result,sort_keys=True,indent=2))


if __name__ == '__main__':
    main()
